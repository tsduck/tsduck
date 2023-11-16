//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEITGenerator.h"
#include "tsDuckContext.h"
#include "tsEIT.h"
#include "tsMJD.h"
#include "tsBCD.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITGenerator::EITGenerator(DuckContext& duck, PID pid, EITOptions options, const EITRepetitionProfile& profile) :
    _duck(duck),
    _eit_pid(pid),
    _options(options),
    _profile(profile),
    _demux(_duck, nullptr, this),
    _packetizer(_duck, _eit_pid, this)
{
    // We need the PAT as long as the TS id is not known.
    _demux.addPID(PID_PAT);

    // We always get TDT/TOT.
    _demux.addPID(PID_TDT);

    // We need to analyze input EIT's only if they feed the EPG.
    if (bool(_options & EITOptions::LOAD_INPUT)) {
        _demux.addPID(_eit_pid);
    }
}


//----------------------------------------------------------------------------
// Reset the EIT generator to default state.
//----------------------------------------------------------------------------

void ts::EITGenerator::reset()
{
    _actual_ts_id = 0;
    _actual_ts_id_set = false;
    _regenerate = false;
    _packet_index = 0;
    _max_bitrate = 0;
    _ts_bitrate = 0;
    _ref_time.clear();
    _ref_time_pkt = 0;
    _eit_inter_pkt = 0;
    _last_eit_pkt = 0;
    _demux.reset();
    _demux.addPID(PID_PAT);
    _packetizer.reset();
    _services.clear();
    for (size_t i = 0; i < _injects.size(); ++i) {
        _injects[i].clear();
    }
    _last_tid = TID_NULL;
    _obsolete_count = 0;
    _versions.clear();
}


//----------------------------------------------------------------------------
// Event: Constructor of the structure containing binary events.
//----------------------------------------------------------------------------

ts::EITGenerator::Event::Event(const uint8_t*& data, size_t& size)
{
    size_t event_size = size;

    if (size >= EIT::EIT_EVENT_FIXED_SIZE) {
        event_size = std::min(size, EIT::EIT_EVENT_FIXED_SIZE + (GetUInt16(data + 10) & 0x0FFF));
        event_id = GetUInt16(data);
        DecodeMJD(data + 2, 5, start_time);
        end_time = start_time + (MilliSecPerHour * DecodeBCD(data[7])) + (MilliSecPerMin * DecodeBCD(data[8])) + (MilliSecPerSec * DecodeBCD(data[9]));
        event_data.copy(data, event_size);
    }

    data += event_size;
    size -= event_size;
}


//----------------------------------------------------------------------------
// ESection: Constructor of the structure for a section, ready to inject.
//----------------------------------------------------------------------------

ts::EITGenerator::ESection::ESection(EITGenerator* gen, const ServiceIdTriplet& srv, TID tid, uint8_t section_number, uint8_t last_section_number)
{
    // Build section data.
    ByteBlockPtr section_data(new ByteBlock(LONG_SECTION_HEADER_SIZE + EIT::EIT_PAYLOAD_FIXED_SIZE + SECTION_CRC32_SIZE));
    CheckNonNull(section_data.pointer());
    uint8_t* data = section_data->data();

    // Section header
    PutUInt8(data, tid);
    PutUInt16(data + 1, 0xF000 | uint16_t(section_data->size() - 3));
    PutUInt16(data + 3, srv.service_id);       // table id extension
    PutUInt8(data + 5, 0xC1);                  // version = 0 for now, updated below
    PutUInt8(data + 6, section_number);
    PutUInt8(data + 7, last_section_number);

    // EIT section payload, without event.
    PutUInt16(data + 8, srv.transport_stream_id);
    PutUInt16(data + 10, srv.original_network_id);
    PutUInt8(data + 12, last_section_number);  // last section number in this segment
    PutUInt8(data + 13, tid);                  // last table id in this service

    // Build a section from the binary data.
    section = new Section(section_data, PID_NULL, CRC32::IGNORE);
    updateVersion(gen, false);
    CheckNonNull(section.pointer());
}


//----------------------------------------------------------------------------
// ESection: Indicate that a section will be modified.
//----------------------------------------------------------------------------

void ts::EITGenerator::ESection::startModifying()
{
    // Do something only if the section is maybe still used in a packetizer.
    if (injected && !section.isNull()) {
        // Duplicate the section. The previous section data is maybe still
        // referenced inside the packetizer and will be deleted later.
        section = new Section(*section, ShareMode::COPY);
    }
    // Mark the new section data as no longer used by a packetizer.
    injected = false;
}


//----------------------------------------------------------------------------
// ESection: Toogle the actual/other status for a section.
//----------------------------------------------------------------------------

void ts::EITGenerator::ESection::toggleActual(bool actual)
{
    if (!section.isNull() && EIT::IsActual(section->tableId()) != actual) {
        startModifying();
        section->setTableId(EIT::ToggleActual(section->tableId(), actual), true);
    }
}


//----------------------------------------------------------------------------
// ESection: Increment version of section.
//----------------------------------------------------------------------------

void ts::EITGenerator::ESection::updateVersion(EITGenerator* gen, bool recompute_crc)
{
    // Does nothing when option SYNC_VERSIONS is set (versions are separately updated later).
    if (!section.isNull() && !(gen->_options & EITOptions::SYNC_VERSIONS)) {
        assert(section->payloadSize() >= EIT::EIT_PAYLOAD_FIXED_SIZE);
        startModifying();
        // ServiceIdTriplet: svid, tsid, onetid
        const ServiceIdTriplet sid(section->tableIdExtension(), GetUInt16(section->payload()), GetUInt16(section->payload() + 2));
        const uint8_t version = gen->nextVersion(sid, section->tableId(), section->sectionNumber());
        section->setVersion(version, recompute_crc);
    }
}


//----------------------------------------------------------------------------
// Compute the next version for a table. If option SYNC_VERSIONS is set, the section number is ignored.
//----------------------------------------------------------------------------

uint8_t ts::EITGenerator::nextVersion(const ServiceIdTriplet& service_id, TID table_id, uint8_t section_number)
{
    // Build a unique section identifier on 64 bits.
    const uint64_t index =
        (uint64_t(table_id) << 56) |
        (uint64_t(service_id.original_network_id) << 40) |
        (uint64_t(service_id.transport_stream_id) << 24) |
        (uint64_t(service_id.service_id) << 8) |
        (bool(_options & EITOptions::SYNC_VERSIONS) ? 0 : section_number);

    // Locate previous version for this section and compute new version.
    const auto iter = _versions.find(index);
    if (iter == _versions.end()) {
        // Section did not exist, use 0 as first version.
        return _versions[index] = 0;
    }
    else {
        // Update version.
        return iter->second = (iter->second + 1) & SVERSION_MASK;
    }
}


//----------------------------------------------------------------------------
// Load EPG data from binary events descriptions.
//----------------------------------------------------------------------------

bool ts::EITGenerator::loadEvents(const ServiceIdTriplet& service_id, const uint8_t* data, size_t size)
{
    bool success = true;

    // Description of the service.
    EService* srv = nullptr;

    // Number of loaded event.
    size_t ev_count = 0;

    // Current time according to the transport stream. Can be "Epoch" (undefined).
    const Time now(getCurrentTime());
    const Time ref_midnight(now.thisDay());

    // Loop on all event descriptions.
    while (size >= EIT::EIT_EVENT_FIXED_SIZE) {

        // Get the next binary event.
        const EventPtr ev(new Event(data, size));
        if (ev->event_data.empty()) {
            _duck.report().error(u"error loading EPG event, truncated data");
            success = false;
            break;
        }

        // Discard events in the past.
        if (now != Time::Epoch && ev->end_time <= now) {
            _duck.report().verbose(u"discard obsolete event id 0x%X (%<d), %s, ending %s", {ev->event_id, service_id, ev->end_time});
            continue;
        }

        // Discard events too far in the future.
        if (now != Time::Epoch && ev->start_time >= ref_midnight + EIT::TOTAL_DAYS * MilliSecPerDay) {
            _duck.report().verbose(u"discard event id 0x%X (%<d), %s, starting %s, too far in the future", {ev->event_id, service_id, ev->start_time});
            continue;
        }

        // Create the service only when we know we have some event to insert.
        if (srv == nullptr) {
            srv = &_services[service_id];
        }

        // Locate or allocate the segment for that event. At this stage, we only create this
        // segment if necessary. This is the minimum to store an event. We do not try to create
        // empty intermediate segments. This will be done in regenerateSchedule().

        const Time seg_start_time(EIT::SegmentStartTime(ev->start_time));
        auto seg_iter = srv->segments.begin();
        while (seg_iter != srv->segments.end() && (*seg_iter)->start_time < seg_start_time) {
            ++seg_iter;
        }
        if (seg_iter == srv->segments.end() || (*seg_iter)->start_time != seg_start_time) {
            // The segment does not exist, create it.
            _duck.report().debug(u"creating EIT segment starting at %s for %s", {seg_start_time, service_id});
            const ESegmentPtr seg(new ESegment(seg_start_time));
            CheckNonNull(seg.pointer());
            seg_iter = srv->segments.insert(seg_iter, seg);
        }
        ESegment& seg(**seg_iter);

        // Insert the binary event in the list of events for that segment.
        auto ev_iter = seg.events.begin();
        while (ev_iter != seg.events.end() && (*ev_iter)->start_time < ev->start_time) {
            ++ev_iter;
        }
        if (ev_iter != seg.events.end() && (*ev_iter)->event_id == ev->event_id && (*ev_iter)->event_data == ev->event_data) {
            // Duplicate event, ignore it.
            continue;
        }
        _duck.report().log(2, u"loaded event id 0x%X (%<d), %s, starting %s", {ev->event_id, service_id, ev->start_time});
        seg.events.insert(ev_iter, ev);
        ev_count++;

        // Mark all EIT schedule in this segment as to be regenerated.
        _regenerate = srv->regenerate = seg.regenerate = true;
    }

    // If some events were added, it may be necessary to regenerate the EIT p/f in this service.
    if (ev_count > 0) {
        assert(srv != nullptr);
        regeneratePresentFollowing(service_id, *srv, now);
    }
    return success;
}


//----------------------------------------------------------------------------
// Load EPG data from an EIT section.
//----------------------------------------------------------------------------

bool ts::EITGenerator::loadEvents(const Section& section, bool get_actual_ts)
{
    const uint8_t* const pl_data = section.payload();
    const size_t pl_size = section.payloadSize();

    // Load all events in the EIT payload.
    bool success = section.isValid() && EIT::IsEIT(section.tableId()) && pl_size >= EIT::EIT_PAYLOAD_FIXED_SIZE;
    if (success) {
        if (get_actual_ts && !_actual_ts_id_set && EIT::IsActual(section.tableId())) {
            // Use the EIT actual TS id as current TS id.
            setTransportStreamId(GetUInt16(pl_data));
        }
        success = loadEvents(EIT::GetService(section), pl_data + EIT::EIT_PAYLOAD_FIXED_SIZE, pl_size - EIT::EIT_PAYLOAD_FIXED_SIZE);
    }
    return success;
}


//----------------------------------------------------------------------------
// Load EPG data from all EIT sections in a section file.
//----------------------------------------------------------------------------

bool ts::EITGenerator::loadEvents(const SectionPtrVector& sections, bool get_actual_ts)
{
    bool success = true;
    for (size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].isNull()) {
            success = loadEvents(*sections[i], get_actual_ts) && success;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Save all current EIT sections.
//----------------------------------------------------------------------------

void ts::EITGenerator::saveEITs(SectionFile& secfile)
{
    SectionPtrVector sections;
    saveEITs(sections);
    secfile.add(sections);
}

void ts::EITGenerator::saveEITs(SectionPtrVector& sections)
{
    // If the reference time is not set, force it to the start time of the oldest event in the database.
    if (_ref_time == Time::Epoch) {
        for (const auto& it1 : _services) {
            // Get first event of first non-empty segment in the service.
            for (const auto& it2 : it1.second.segments) {
                if (!it2->events.empty()) {
                    const Time& start_time(it2->events.front()->start_time);
                    if (_ref_time == Time::Epoch || start_time < _ref_time) {
                        _ref_time = start_time;
                        _ref_time_pkt = _packet_index;
                    }
                    break; // found oldest in this service, move to next service
                }
            }
        }
        if (_ref_time != Time::Epoch) {
            _duck.report().debug(u"forcing TS time to %s (oldest event start time) at packet index %'d", {_ref_time, _ref_time_pkt});
        }
    }

    // Ensure all EIT sections are correctly regenerated.
    const Time now(getCurrentTime());
    updateForNewTime(now);
    regenerateSchedule(now);

    size_t pf_count = 0;
    size_t sched_count = 0;

    // Loop on all services, saving all EIT p/f.
    for (const auto& it1 : _services) {
        for (size_t i = 0; i < it1.second.pf.size(); ++i) {
            if (!it1.second.pf[i].isNull()) {
                sections.push_back(it1.second.pf[i]->section);
                pf_count++;
            }
        }
    }

    // Loop on all services again, saving all EIT schedule.
    for (const auto& it1 : _services) {
        for (const auto& it2 : it1.second.segments) {
            for (const auto& it3 : it2->sections) {
                sections.push_back(it3->section);
                sched_count++;
            }
        }
    }

    _duck.report().debug(u"saved %'d EIT (%'d p/f, %'d sched)", {pf_count + sched_count, pf_count, sched_count});
}


//----------------------------------------------------------------------------
// Define the "actual" transport stream id for generated EIT's.
//----------------------------------------------------------------------------

void ts::EITGenerator::setTransportStreamId(uint16_t new_ts_id)
{
    // Do nothing if this is a new TS id.
    if (_actual_ts_id_set && _actual_ts_id == new_ts_id) {
        return;
    }
    _duck.report().debug(u"setting EIT generator TS id to 0x%X (%<d)", {new_ts_id});

    // Set new TS id.
    const uint16_t old_ts_id = _actual_ts_id_set ? _actual_ts_id : 0xFFFF;
    _actual_ts_id = new_ts_id;
    _actual_ts_id_set = true;

    // No longer need the PAT when the TS id is known.
    _demux.removePID(PID_PAT);

    // Current time according to the transport stream. Can be "Epoch" (undefined).
    const Time now(getCurrentTime());
    if (now == Time::Epoch) {
        // Cannot regenerate EIT's without reference time.
        return;
    }

    // Update all EIT's which switch between actual and other.
    for (auto& srv_iter : _services) {
        EService& srv(srv_iter.second);

        // Does this service changes between actual and other?
        const bool new_actual = srv_iter.first.transport_stream_id == new_ts_id;
        const bool new_other = srv_iter.first.transport_stream_id == old_ts_id;
        const bool need_eit = (new_actual && bool(_options & EITOptions::GEN_ACTUAL)) || (new_other && bool(_options & EITOptions::GEN_OTHER));

        // Test if this service shall switch between actual and other.
        if (new_other || new_actual) {

            // Process EIT p/f.
            if ((new_actual && bool(_options & EITOptions::GEN_ACTUAL_PF)) || (new_other && bool(_options & EITOptions::GEN_OTHER_PF))) {
                if (need_eit && (srv.pf[0].isNull() || srv.pf[1].isNull())) {
                    // At least one EIT p/f shall be rebuilt.
                    regeneratePresentFollowing(srv_iter.first, srv_iter.second, now);
                }
                else {
                    // Loop on EIT p & f sections.
                    for (size_t i = 0; i < srv.pf.size(); ++i) {
                        if (need_eit) {
                            assert(!srv.pf[i].isNull());
                            srv.pf[i]->toggleActual(new_actual);
                        }
                        else if (!srv.pf[i].isNull()) {
                            // The existing section is no longer needed.
                            markObsoleteSection(*srv.pf[i]);
                            srv.pf[i].clear();
                        }
                    }
                }
            }

            // Process EIT schedule (all segments, all sections).
            if (bool(_options & EITOptions::GEN_SCHED)) {
                if ((_options & (EITOptions::GEN_ACTUAL | EITOptions::GEN_OTHER)) == (EITOptions::GEN_ACTUAL | EITOptions::GEN_OTHER)) {
                    // Actual and others are both requested. Toggle the state of existing sections.
                    for (const auto& seg_iter : srv.segments) {
                        const ESegment& seg(*seg_iter);
                        for (const auto& sec_iter : seg.sections) {
                            sec_iter->toggleActual(new_actual);
                        }
                    }
                }
                else if (need_eit) {
                    // The EIT schedule for that service were not there, we need them now, regenerate later.
                    _regenerate = srv.regenerate = true;
                    for (const auto& seg_iter : srv.segments) {
                        seg_iter->regenerate = true;
                    }
                }
                else {
                    // We no longer need the EIT schedule.
                    for (auto& seg_iter : srv.segments) {
                        ESegment& seg(*seg_iter);
                        for (auto& sec_iter : seg.sections) {
                            markObsoleteSection(*sec_iter);
                        }
                        seg.sections.clear();
                        seg.regenerate = false;
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Set new EIT generation options.
//----------------------------------------------------------------------------

void ts::EITGenerator::setOptions(EITOptions options)
{
    // Update the options.
    const EITOptions old_options = _options;
    _options = options;

    // If the new options request to load events from input EIT's, demux the EIT PID.
    if (bool(options & EITOptions::LOAD_INPUT)) {
        _demux.addPID(_eit_pid);
    }
    else {
        _demux.removePID(_eit_pid);
    }

    // Current time according to the transport stream. Can be "Epoch" (undefined).
    const Time now(getCurrentTime());

    // Check the configuration has changed for EIT p/f and EIT schedule, respectively.
    const bool pf_changed = (_options & EITOptions::GEN_PF) != (old_options & EITOptions::GEN_PF);
    const bool sched_changed = (_options & EITOptions::GEN_SCHED) != (old_options & EITOptions::GEN_SCHED);

    // If the combination of EIT to generate has changed, regenerate EIT.
    if ((pf_changed || sched_changed) && _actual_ts_id_set && now != Time::Epoch) {

        // Loop on all services.
        for (auto& srv_iter : _services) {

            const ServiceIdTriplet& service_id(srv_iter.first);
            EService& srv(srv_iter.second);

            const bool actual = service_id.transport_stream_id == _actual_ts_id;
            const bool need_eit = (actual && bool(_options & EITOptions::GEN_ACTUAL)) || (!actual && bool(_options & EITOptions::GEN_OTHER));
            const auto GEN_PF = actual ? EITOptions::GEN_ACTUAL_PF : EITOptions::GEN_OTHER_PF;
            const auto GEN_SCHED = actual ? EITOptions::GEN_ACTUAL_SCHED : EITOptions::GEN_OTHER_SCHED;

            // Process EIT p/f.
            if (pf_changed) {
                if (!need_eit || !(_options & GEN_PF)) {
                    // Remove existing EIT p/f sections.
                    for (size_t i = 0; i < srv.pf.size(); ++i) {
                        if (!srv.pf[i].isNull()) {
                            markObsoleteSection(*srv.pf[i]);
                            srv.pf[i].clear();
                        }
                    }
                }
                else if (srv.pf[0].isNull() || srv.pf[1].isNull()) {
                    // At least one EIT p/f shall be rebuilt.
                    regeneratePresentFollowing(service_id, srv, now);
                }
            }

            // Process EIT schedule (all segments, all sections).
            if (sched_changed) {
                if (!need_eit || !(_options & GEN_SCHED)) {
                    // We no longer need the EIT schedule.
                    for (auto& seg_iter : srv.segments) {
                        ESegment& seg(*seg_iter);
                        for (auto& sec_iter : seg.sections) {
                            markObsoleteSection(*sec_iter);
                        }
                        seg.sections.clear();
                        seg.regenerate = false;
                    }
                }
                else {
                    // The EIT schedule for that service were not there, we need them now, regenerate later.
                    _regenerate = srv.regenerate = true;
                    for (const auto& seg_iter : srv.segments) {
                        seg_iter->regenerate = true;
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Set TS bitrate and maximum EIT bitrate of the EIT PID.
//----------------------------------------------------------------------------

void ts::EITGenerator::setBitRateField(BitRate EITGenerator::* field, const BitRate& bitrate)
{
    // Update the target field (_ts_bitrate or _max_bitrate) if modified.
    if (this->*field != bitrate) {
        this->*field = bitrate;
        if (_ts_bitrate == 0 || _max_bitrate == 0) {
            // Cannot compute EIT inter-packet, use free insertion based on cycle time.
            _eit_inter_pkt = 0;
        }
        else {
            // Both bitrates are known, compute the minimum interval between two EIT packets.
            _eit_inter_pkt = (_ts_bitrate / _max_bitrate).toInt();
        }
    }
}


//----------------------------------------------------------------------------
// Get the current time in the stream processing.
//----------------------------------------------------------------------------

ts::Time ts::EITGenerator::getCurrentTime()
{
    return _ref_time == Time::Epoch ? Time::Epoch : _ref_time + PacketInterval(_ts_bitrate, _packet_index - _ref_time_pkt);
}


//----------------------------------------------------------------------------
// Set the current time in the stream processing.
//----------------------------------------------------------------------------

void ts::EITGenerator::setCurrentTime(Time current_utc)
{
    // Store the current time.
    _ref_time = current_utc;
    _ref_time_pkt = _packet_index;
    _duck.report().debug(u"setting TS time to %s at packet index %'d", {_ref_time, _ref_time_pkt});

    // Update EIT database if necessary.
    updateForNewTime(_ref_time);
}


//----------------------------------------------------------------------------
// Mark a segment or section as obsolete, garbage collect obsolete sections
//----------------------------------------------------------------------------

void ts::EITGenerator::markObsoleteSegment(ESegment &seg)
{
    for (const auto& it : seg.sections) {
        markObsoleteSection(*it);
    }
}

void ts::EITGenerator::markObsoleteSection(ESection& sec)
{
    // Don't do anything if the section is already obsolete.
    if (!sec.obsolete) {

        // Mark the section as obsolete.
        sec.obsolete = true;
        _obsolete_count++;

        // If too many obsolete sections were not naturally discarded, they probably
        // accumulate because the EIT bandwidth is not large enough and low-priority
        // EIT schedule never get a chance to get selected (and discarded when marked
        // as obsolete). Do some garbage collecting to avoid infinite accumulation.
        if (_obsolete_count > 100) {
            // Loop on all injection queues.
            for (size_t index = 0; index < _injects.size(); ++index) {
                // Loop on all sections in the queue.
                ESectionList& list(_injects[index]);
                auto it = list.begin();
                while (it != list.end()) {
                    if ((*it)->obsolete) {
                        it = list.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
            _obsolete_count = 0;
        }
    }
}


//----------------------------------------------------------------------------
// Enqueue a section for injection.
//----------------------------------------------------------------------------

void ts::EITGenerator::enqueueInjectSection(const ESectionPtr& sec, const Time& next_inject, bool try_front)
{
    // Update section injection time.
    sec->next_inject = next_inject;

    // Compute which injection queue to use.
    ESectionList& list(_injects[size_t(_profile.sectionToProfile(*sec->section))]);

    // Even start at from or back of the queue (possible optimization).
    if (try_front) {
        auto it = list.begin();
        while (it != list.end() && (*it)->next_inject <= next_inject) {
            ++it;
        }
        list.insert(it, sec);
    }
    else {
        auto it = list.end();
        while (it != list.begin() && (*--it)->next_inject > next_inject) {
        }
        if (it != list.end()) {
            // it points to a section before sec, must insert after.
            ++it;
        }
        list.insert(it, sec);
    }
}


//----------------------------------------------------------------------------
// Regenerate, if necessary, the EIT p/f in a service.
//----------------------------------------------------------------------------

void ts::EITGenerator::regeneratePresentFollowing(const ServiceIdTriplet& service_id, EService& srv, const Time& now)
{
    // Don't know what to generate if the actual TS or current time are unknown.
    if (!_actual_ts_id_set || now == Time::Epoch) {
        return;
    }

    const bool actual = _actual_ts_id == service_id.transport_stream_id;
    const auto GEN_PF = actual ? EITOptions::GEN_ACTUAL_PF : EITOptions::GEN_OTHER_PF;

    if (!(_options & GEN_PF)) {
        // This type of EIT cannot be (no time ref) or shall not be (excluded) generated. If sections exist, delete them.
        for (size_t i = 0; i < srv.pf.size(); ++i) {
            if (!srv.pf[i].isNull()) {
                markObsoleteSection(*srv.pf[i]);
                srv.pf[i].clear();
            }
        }
    }
    else {
        // Find first and second event in the service. Can be null if none is found.
        std::array<EventPtr, 2> events;
        size_t next_event = 0;
        for (auto seg_iter = srv.segments.begin(); next_event < events.size() && seg_iter != srv.segments.end(); ++seg_iter) {
            const ESegment& seg(**seg_iter);
            for (auto ev_iter = seg.events.begin(); next_event < events.size() && ev_iter != seg.events.end(); ++ev_iter) {
                events[next_event++] = *ev_iter;
            }
        }

        // If the first event is not yet current, make it the "following" one.
        if (!events[0].isNull() && now < events[0]->start_time) {
            events[1] = events[0];
            events[0].clear();
        }

        // Rebuild the two sections when necessary.
        const TID tid = actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        const bool modp = regeneratePresentFollowingSection(service_id, srv.pf[0], tid, 0, events[0], now);
        const bool modf = regeneratePresentFollowingSection(service_id, srv.pf[1], tid, 1, events[1], now);

        // With SYNC_VERIONS, if any section is modified, update both versions.
        if ((modp || modf) && bool(_options & EITOptions::SYNC_VERSIONS)) {
            const uint8_t version = nextVersion(service_id, tid, 0);
            srv.pf[0]->section->setVersion(version, true);
            srv.pf[1]->section->setVersion(version, true);
        }
    }
}


//----------------------------------------------------------------------------
// Regenerate, if necessary, one EIT present or following.
//----------------------------------------------------------------------------

bool ts::EITGenerator::regeneratePresentFollowingSection(const ServiceIdTriplet& service_id,
                                                         ESectionPtr& sec,
                                                         TID tid,
                                                         bool section_number,
                                                         const EventPtr& event,
                                                         const Time&inject_time)
{
    if (sec.isNull()) {
        // The section did not exist, create it.
        sec = new ESection(this, service_id, tid, section_number, 1);
        CheckNonNull(sec.pointer());
        // The initial state of the section is: no event, no CRC.
        if (!event.isNull()) {
            // Append the event in the payload.
            sec->section->appendPayload(event->event_data, false);
        }
        if (!(_options & EITOptions::SYNC_VERSIONS)) {
            sec->section->recomputeCRC();
        }
        // Place the section in the inject queue.
        enqueueInjectSection(sec, inject_time, true);
        // Section was modified.
        return true;
    }
    else if (event.isNull()) {
        // The section already exists. It must be already in an injection queue.
        // There is no more event, truncate the section payload to remove the event (if any is present).
        if (sec->section->tableId() != tid || sec->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE) {
            sec->startModifying();
            sec->section->setTableId(tid, false);
            sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, false);
            sec->updateVersion(this, true);
            // Section was modified.
            return true;
        }
    }
    else if (sec->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE + event->event_data.size() ||
             std::memcmp(sec->section->payload() + EIT::EIT_PAYLOAD_FIXED_SIZE, event->event_data.data(), event->event_data.size()) != 0)
    {
        // The section already exists. It must be already in an injection queue.
        // The event is not the same as the one in the section, update the section.
        sec->startModifying();
        sec->section->setTableId(tid, false);
        sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, false);
        sec->section->appendPayload(event->event_data, false);
        sec->updateVersion(this, true);
        // Section was modified.
        return true;
    }
    else if (sec->section->tableId() != tid) {
        // The same event is already in the section but the table id changed (because the TS id changed).
        sec->startModifying();
        sec->section->setTableId(tid, false);
        sec->updateVersion(this, true);
        // Section was modified.
        return true;
    }

    // Section not modified.
    return false;
}


//----------------------------------------------------------------------------
// Regenerate all EIT schedule, create missing segments and sections.
//----------------------------------------------------------------------------

void ts::EITGenerator::regenerateSchedule(const Time& now)
{
    // We cannot regenerate EIT if the TS id or the current time is unknown.
    if (!_regenerate || !_actual_ts_id_set || now == Time::Epoch) {
        return;
    }

    // Reference time for EIT schedule.
    const Time last_midnight(now.thisDay());

    // Check if all sections of a sub-table must have the same version number.
    const bool sync_versions = bool(_options & EITOptions::SYNC_VERSIONS);

    // Loop on all services, regenerating those which are marked for regeneration.
    for (auto& srv_iter : _services) {
        if (srv_iter.second.regenerate) {

            const ServiceIdTriplet& service_id(srv_iter.first);
            EService& srv(srv_iter.second);
            const bool actual = service_id.transport_stream_id == _actual_ts_id;
            const auto GEN_SCHED = actual ? EITOptions::GEN_ACTUAL_SCHED : EITOptions::GEN_OTHER_SCHED;
            _duck.report().debug(u"regenerating events for service 0x%X (%<d)", {service_id});

            // Set of subtables to globally update their version (SYNC_VERSIONS only).
            std::set<TID> sync_tids;

            // Check if EIT schedule are needed for the service.
            const bool need_eits = bool(_options & GEN_SCHED);

            // Remove initial segments before last midnight.
            while (!srv.segments.empty() && srv.segments.front()->start_time < last_midnight) {
                markObsoleteSegment(*srv.segments.front());
                srv.segments.pop_front();
            }

            // Remove final empty segments (no events). Keep at least one segment for last midnight, even if empty.
            while (!srv.segments.empty() && srv.segments.back()->events.empty() && srv.segments.back()->start_time > last_midnight) {
                markObsoleteSegment(*srv.segments.back());
                srv.segments.pop_back();
            }

            // Make sure that the first segment exists for last midnight.
            if (srv.segments.empty() || srv.segments.front()->start_time != last_midnight) {
                _duck.report().debug(u"creating EIT segment starting at %s for %s", {last_midnight, service_id});
                const ESegmentPtr seg(new ESegment(last_midnight));
                CheckNonNull(seg.pointer());
                srv.segments.push_front(seg);
            }

            // Loop on all segments. The first segment must be at last midnight.
            Time segment_start_time(last_midnight);
            size_t segment_number = 0;
            for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {

                // Enforce the existence of contiguous segments. Create missing segments when necessary.
                if ((*seg_iter)->start_time != segment_start_time) {
                    _duck.report().debug(u"creating EIT segment starting at %s for %s", {segment_start_time, service_id});
                    assert((*seg_iter)->start_time > segment_start_time);
                    const ESegmentPtr seg(new ESegment(segment_start_time));
                    CheckNonNull(seg.pointer());
                    seg_iter = srv.segments.insert(seg_iter, seg);
                }
                ESegment& seg(**seg_iter);

                if (!need_eits) {
                    // We do not need EIT schedule here, delete all sections.
                    markObsoleteSegment(seg);
                    seg.sections.clear();
                }
                else if (seg.regenerate) {
                    // Regenerate EIT schedule in the segment.

                    // Table id and first section number in that segment.
                    const TID table_id = EIT::SegmentToTableId(actual, segment_number);
                    const uint8_t first_section_number = EIT::SegmentToSection(segment_number);
                    uint8_t section_number = first_section_number;

                    // Update or generate all sections.
                    auto ev_iter = seg.events.begin();
                    auto sec_iter = seg.sections.begin();
                    while (ev_iter != seg.events.end()) {

                        // Check if the current section is still valid, meaning it exactly contains the next events.
                        const auto saved_ev_iter = ev_iter;
                        bool section_still_valid = sec_iter != seg.sections.end() && (*sec_iter)->section->payloadSize() >= EIT::EIT_PAYLOAD_FIXED_SIZE;
                        const uint8_t* pl = section_still_valid ? (*sec_iter)->section->payload() + EIT::EIT_PAYLOAD_FIXED_SIZE : nullptr;
                        size_t pl_size = section_still_valid ? (*sec_iter)->section->payloadSize() - EIT::EIT_PAYLOAD_FIXED_SIZE : 0;

                        while (section_still_valid && pl_size > 0 && ev_iter != seg.events.end()) {
                            const uint8_t* ev = (*ev_iter)->event_data.data();
                            const size_t ev_size = (*ev_iter)->event_data.size();
                            section_still_valid = pl_size >= ev_size && std::memcmp(pl, ev, ev_size) == 0;
                            if (section_still_valid) {
                                ++ev_iter;
                                pl += ev_size;
                                pl_size -= ev_size;
                            }
                        }
                        if (section_still_valid) {
                            // If the next event exists and could fit in the section, then the section is no longer valid.
                            section_still_valid = ev_iter == seg.events.end() ||
                                (*sec_iter)->section->payloadSize() + (*ev_iter)->event_data.size() > MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
                        }

                        // If the current section is still valid, skip those events and move to next section.
                        if (section_still_valid) {
                            ++sec_iter;
                            ++section_number;
                            continue;
                        }

                        // The section is no longer valid or does not exist, rebuild it.
                        const ESectionPtr sec(new ESection(this, service_id, table_id, section_number, section_number));
                        CheckNonNull(sec.pointer());
                        if (sec_iter != seg.sections.end()) {
                            // Existing section, invalidate it and replace it.
                            markObsoleteSection(**sec_iter);
                            *sec_iter = sec;
                        }
                        else if (seg.sections.size() >= EIT::SECTIONS_PER_SEGMENT) {
                            // Too many sections for that segment, skip the last events.
                            break;
                        }
                        else {
                            // Insert a new section for that segment.
                            sec_iter = seg.sections.insert(sec_iter, sec);
                        }

                        // Restart exploring events at the beginning of the section.
                        ev_iter = saved_ev_iter;

                        // Insert events in the section, as long as they fit.
                        while (ev_iter != seg.events.end() && sec->section->payloadSize() + (*ev_iter)->event_data.size() <= MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE) {
                            // Append the event to the section payload.
                            sec->section->appendPayload((*ev_iter)->event_data, false);
                            ++ev_iter;
                        }

                        // Section complete.
                        if (sync_versions) {
                            // Will adjust version for all sections of this sub-table.
                            sync_tids.insert(table_id);
                        }
                        else {
                            // Sections are independently versioned, this one is complete.
                            sec->section->recomputeCRC();
                        }
                        enqueueInjectSection(sec, getCurrentTime(), true);

                        // Move to next section (if it exists).
                        ++sec_iter;
                        ++section_number;
                    }

                    // Deallocate remaining sections, if any.
                    while (sec_iter != seg.sections.end()) {
                        markObsoleteSection(**sec_iter);
                        sec_iter = seg.sections.erase(sec_iter);
                    }

                    // We need at least one section, possibly empty, in each segment.
                    if (seg.sections.empty()) {
                        const ESectionPtr sec(new ESection(this, service_id, table_id, first_section_number, first_section_number));
                        CheckNonNull(sec.pointer());
                        seg.sections.push_back(sec);
                        enqueueInjectSection(sec, getCurrentTime(), true);
                    }
                }

                // Clear segment regeneration flag.
                seg.regenerate = false;

                // Time and index of next expected segment:
                segment_start_time += EIT::SEGMENT_DURATION;
                segment_number++;
            }

            // Fix synthetic fields in all EIT-schedule sections: last_section_number, segment_last_section_number, last_table_id.
            if (need_eits) {
                assert(!srv.segments.empty());
                assert(!srv.segments.back()->sections.empty());

                segment_number = srv.segments.size();
                TID previous_table_id = TID_NULL;
                TID last_table_id = TID_NULL;
                uint8_t last_section_number = 0;

                // Loop on segments from last to first.
                for (auto seg_iter = srv.segments.rbegin(); seg_iter != srv.segments.rend(); ++seg_iter) {
                    ESegment& seg(**seg_iter);
                    assert(!seg.sections.empty());
                    assert(segment_number > 0);

                    const TID table_id = EIT::SegmentToTableId(actual, --segment_number);
                    uint8_t section_number = EIT::SegmentToSection(segment_number);
                    const uint8_t segment_last_section_number = uint8_t(section_number + seg.sections.size() - 1);

                    if (table_id != previous_table_id) {
                        // Changed table. We are on the last segment of the previous table.
                        last_section_number = segment_last_section_number;
                        previous_table_id = table_id;
                    }
                    if (seg_iter == srv.segments.rbegin()) {
                        // Last segment.
                        last_table_id = table_id;
                    }
                    for (const auto& sec_iter : seg.sections) {
                        ESection& sec(*sec_iter);
                        const uint8_t* pl = sec.section->payload();
                        if (sec.section->sectionNumber() != section_number ||
                            sec.section->lastSectionNumber() != last_section_number ||
                            pl[4] != segment_last_section_number ||
                            pl[5] != last_table_id)
                        {
                            if (sec.section->sectionNumber() != section_number) {
                                sec.section->setSectionNumber(section_number, false);
                                sec.updateVersion(this, true);
                            }
                            sec.section->setLastSectionNumber(last_section_number, false);
                            sec.section->setUInt8(4, segment_last_section_number, false);
                            sec.section->setUInt8(5, last_table_id, !sync_versions);
                            if (sync_versions) {
                                sync_tids.insert(table_id);
                            }
                            assert(sec.section->sectionNumber() <= sec.section->lastSectionNumber());
                        }
                        section_number++;
                    }
                }
            }

            // Regenerate synchronous new versions for all sections of updated subtables (only with SYNC_VERSIONS).
            if (!sync_tids.empty()) {
                // Each sub-table uses 32 segments (SEGMENTS_PER_TABLE). We loop over segments in this service,
                // 32 per 32. When a table needs to be updated, synchronously update all versions.
                segment_number = 0;
                auto seg_iter = srv.segments.begin();
                while (seg_iter != srv.segments.end()) {
                    const TID table_id = EIT::SegmentToTableId(actual, segment_number);
                    const uint8_t version = nextVersion(service_id, table_id, 0);
                    const bool update = sync_tids.find(table_id) != sync_tids.end();
                    // Loop on all segments of that sub-table.
                    for (size_t seg_count = 0; seg_count < EIT::SEGMENTS_PER_TABLE && seg_iter != srv.segments.end(); seg_count++) {
                        // Update all sections in that segment, if necessary.
                        if (update) {
                            for (auto& sec : (*seg_iter)->sections) {
                                (*sec).startModifying();
                                (*sec).section->setVersion(version, true);
                            }
                        }
                        // Next segment.
                        ++segment_number;
                        ++seg_iter;
                    }
                }
            }

            // Clear service regeneration flag.
            srv.regenerate = false;
        }
    }

    // Clear global regeneration flag.
    _regenerate = false;
}


//----------------------------------------------------------------------------
// Update the EIT database according to the current time.
//----------------------------------------------------------------------------

void ts::EITGenerator::updateForNewTime(const Time& now)
{
    // We cannot regenerate EIT if the TS id or the current time is unknown.
    if (!_actual_ts_id_set || now == Time::Epoch) {
        return;
    }

    // Reference time for EIT schedule.
    const Time last_midnight(now.thisDay());

    // Loop on all services.
    for (auto& srv_iter : _services) {

        const ServiceIdTriplet& service_id(srv_iter.first);
        EService& srv(srv_iter.second);
        assert(!srv.segments.empty());

        // If we changed day, mark the service as being regenerated (will remove obsolete segments or create missing ones).
        if (last_midnight != srv.segments.front()->start_time) {
            _regenerate = srv.regenerate = true;
        }

        // Segments between last midnight and current time shall be regenerated as well (one empty section).
        auto seg_iter = srv.segments.begin();
        while (seg_iter != srv.segments.end() && (*seg_iter)->start_time + EIT::SEGMENT_DURATION <= now) {
            ESegment& seg(**seg_iter);
            seg.events.clear();
            if (seg.sections.size() != 1 || seg.sections.front()->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE) {
                // There are more than one section or the unique section contains events.
                _regenerate = srv.regenerate = seg.regenerate = true;
            }
            ++seg_iter;
        }

        // Remove obsolete events in the segment containing "now".
        if (seg_iter != srv.segments.end()) {
            ESegment& seg(**seg_iter);
            while (!seg.events.empty() && seg.events.front()->end_time <= now) {
                seg.events.pop_front();
                // Regenerate the segment, unless we use the lazy update mode.
                if (!(_options & EITOptions::LAZY_SCHED_UPDATE)) {
                    _regenerate = srv.regenerate = seg.regenerate = true;
                }
            }
        }

        // Discard events too far in the future.
        while (!srv.segments.empty() && srv.segments.back()->start_time >= last_midnight + EIT::TOTAL_DAYS * MilliSecPerDay) {
            srv.segments.pop_back();
        }

        // Renew EIT p/f of the service when necessary.
        regeneratePresentFollowing(service_id, srv, now);
    }
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::EITGenerator::doStuffing()
{
    return bool(_options & EITOptions::PACKET_STUFFING);
}

void ts::EITGenerator::provideSection(SectionCounter counter, SectionPtr& section)
{
    // Look for an EIT section with a due time no later than current time.
    const Time now(getCurrentTime());

    // Update EIT's according to current time.
    updateForNewTime(getCurrentTime());

    // Make sure the EIT schedule are up-to-date.
    regenerateSchedule(now);

    // Make sure no section for the last injected {tid,tidext} is scheduled for _section_gap milliseconds.
    if (_last_tid != TID_NULL) {
        ESectionList& list(_injects[_last_index]);
        const Time next_inject = now + _section_gap;
        int gap_count = 0;
        auto it = list.begin();
        while (it != list.end() && (*it)->next_inject < next_inject) {
            if ((*it)->section->tableId() != _last_tid || (*it)->section->tableIdExtension() != _last_tidext) {
                ++it;
            }
            else {
                // We have a section with the same {tid,tidext}, need to reschedule it later.
                const ESectionPtr next_sec = *it;
                _duck.report().log(2, u"reschedule section %d at %s", {next_sec->section->sectionNumber(), next_inject});
                it = list.erase(it);
                // We can't call enqueueInjectSection() since we are currently walking through the same
                // list and enqueueInjectSection() may change "it" iterator. Insert manually.
                // Also reschedule each section "_section_gap" later than the previous one.
                next_sec->next_inject = next_inject + gap_count++ * _section_gap;
                auto it1 = it;
                while (it1 != list.end() && (*it1)->next_inject < next_sec->next_inject) {
                    ++it1;
                }
                const bool same_place = it1 == it;
                it1 = list.insert(it1, next_sec);
                if (same_place) {
                    it = it1;
                }
            }
        }
        _last_tid = TID_NULL;
    }

    // Loop on all injection queues, in decreasing order of priority.
    for (size_t index = 0; index < _injects.size(); ++index) {

        // Check if the first section in the queue is ready for injection.
        // Loop on obsolete events. Return on first injected event.
        while (!_injects[index].empty() && _injects[index].front()->next_inject <= now) {

            // Remove the first section from the queue.
            const ESectionPtr sec(_injects[index].front());
            _injects[index].pop_front();

            if (sec->obsolete) {
                // This is an obsolete section, no longer in the base, drop it.
                assert(_obsolete_count > 0);
                _obsolete_count--;
            }
            else {
                // This section shall be injected.
                section = sec->section;
                sec->injected = true;

                // Requeue next iteration of that section.
                enqueueInjectSection(sec, now + _profile.repetitionSeconds(*sec->section) * MilliSecPerSec, false);
                _duck.report().log(2, u"inject section TID 0x%X (%<d), service 0x%X (%<d), at %s, requeue for %s",
                                   {section->tableId(), section->tableIdExtension(), now, sec->next_inject});
                _last_tid = section->tableId();
                _last_tidext = section->tableIdExtension();
                _last_index = index;
                return;
            }
        }
    }

    // No section is ready for injection.
    section.clear();
}


//----------------------------------------------------------------------------
// Implementation of SectionHandlerInterface.
// Process a section from the input stream (invoked by demux).
//----------------------------------------------------------------------------

void ts::EITGenerator::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();

    if (tid == TID_PAT && !_actual_ts_id_set) {
        // A PAT section is used to define the transport stream id if not already known.
        setTransportStreamId(section.tableIdExtension());
    }
    else if (EIT::IsEIT(tid) && bool(_options & EITOptions::LOAD_INPUT)) {
        // Use input EIT's as EPG data when specified in the generation options.
        loadEvents(section);
    }
    else if ((tid == TID_TDT || tid == TID_TOT) && section.payloadSize() >= MJD_SIZE) {
        // The first 5 bytes of a TDT or TOT payload is the UTC time.
        Time utc;
        if (DecodeMJD(section.payload(), MJD_SIZE, utc)) {
            setCurrentTime(utc);
        }
    }
}


//----------------------------------------------------------------------------
// Process one packet from the stream.
//----------------------------------------------------------------------------

void ts::EITGenerator::processPacket(TSPacket& pkt)
{
    // Pass incoming packets in the demux.
    _demux.feedPacket(pkt);

    // The packet shall be nullified if it comes from the input EIT PID and not replaced.
    const PID pid = pkt.getPID();
    bool nullify = pid == _eit_pid;

    // Outgoing EIT's can replace null packets or the incoming EIT PID.
    // Check if we reached a possible insertion point for EIT.
    if ((pid == _eit_pid || pid == PID_NULL) && (_eit_inter_pkt == 0 || _packet_index >= _last_eit_pkt + _eit_inter_pkt)) {

        // Replace the packet with either an EIT packet or a null packet.
        if (_packetizer.getNextPacket(pkt)) {
            // An EIT packet was actually generated.
            _last_eit_pkt = _packet_index;
        }

        // No longer nullify packets which were updated.
        nullify = false;
    }

    // Count packets in the stream.
    _packet_index++;

    // Nullify incoming EIT packets which were not replaced.
    if (nullify) {
        pkt = NullPacket;
    }
}


//----------------------------------------------------------------------------
// Dump the internal state of the EIT generator.
//----------------------------------------------------------------------------

void ts::EITGenerator::dumpInternalState(int lev) const
{
    Report& rep(_duck.report());
    if (lev <= rep.maxSeverity()) {
        rep.log(lev, u"");
        rep.log(lev, u"EITGenerator internal state");
        rep.log(lev, u"---------------------------");
        rep.log(lev, u"");
        rep.log(lev, u"EIT PID: 0x%X (%<d)", {_eit_pid});
        rep.log(lev, u"EIT options: 0x%X", {uint16_t(_options)});
        rep.log(lev, u"Actual TS id %s: 0x%X (%<d)", {_actual_ts_id_set ? u"set" : u"not set", _actual_ts_id});
        rep.log(lev, u"TS packets: %'d", {_packet_index});
        rep.log(lev, u"TS bitrate: %'d b/s, max EIT bitrate: %'d b/s", {_ts_bitrate, _max_bitrate});
        rep.log(lev, u"Services count: %d", {_services.size()});
        rep.log(lev, u"Reference time: %s at packet %'d", {_ref_time, _ref_time_pkt});
        rep.log(lev, u"Obsolete sections count: %d", {_obsolete_count});
        rep.log(lev, u"Regenerate: %s", {_regenerate});

        // Dump internal state of services.
        for (const auto& it1 : _services) {
            rep.log(lev, u"");
            rep.log(lev, u"- Service content: %s", {it1.first});
            rep.log(lev, u"  Segment count: %d", {it1.second.segments.size()});
            rep.log(lev, u"  Regenerate: %s", {it1.second.regenerate});
            dumpSection(lev, u"  Present section: ", it1.second.pf[0]);
            dumpSection(lev, u"  Follow section:  ", it1.second.pf[1]);
            for (const auto& it2 : it1.second.segments) {
                const ESegment& seg(*it2);
                rep.log(lev, u"  - Segment %s, regenerate: %s, events: %d, sections: %d", {seg.start_time, seg.regenerate, seg.events.size(), seg.sections.size()});
                rep.log(lev, u"    Events:");
                for (const auto& it3 : it2->events) {
                    const Event& ev(*it3);
                    rep.log(lev, u"    - Event id: 0x%X, start: %s, end: %s, %d bytes", {ev.event_id, ev.start_time, ev.end_time, ev.event_data.size()});
                }
                rep.log(lev, u"    Sections:");
                for (const auto& it3 : it2->sections) {
                    dumpSection(lev, u"    - Section: ", it3);
                }
            }
        }

        // Dump internal state of injection queues.
        for (size_t index = 0; index < _injects.size(); ++index) {
            rep.log(lev, u"");
            rep.log(lev, u"- Injection queue #%d: %d sections", {index, _injects[index].size()});
            for (auto it = _injects[index].begin(); it != _injects[index].end(); ++it) {
                dumpSection(lev, u"  - ", *it);
            }
        }
        rep.log(lev, u"");
    }
}


//----------------------------------------------------------------------------
// Dump the internal state of an ESection for dumpInternalState().
//----------------------------------------------------------------------------

void ts::EITGenerator::dumpSection(int lev, const UString& margin, const ESectionPtr& sec) const
{
    Report& rep(_duck.report());

    // Eliminate null ESection.
    if (sec.isNull()) {
        rep.log(lev, u"%s(null)", {margin});
        return;
    }

    // Eliminate null Section in ESection.
    const UString space(margin.size(), SPACE);
    const UString desc(UString::Format(u"next inject: %s, obsolete: %s, injected: %s", {sec->next_inject, sec->obsolete, sec->injected}));
    if (sec->section.isNull()) {
        rep.log(lev, u"%s(null section)", {margin});
        rep.log(lev, u"%s%s", {space, desc});
        return;
    }

    // Eliminate invalid Section in ESection.
    const Section& section(*sec->section);
    if (!section.isValid() || section.payloadSize() < EIT::EIT_PAYLOAD_FIXED_SIZE) {
        rep.log(lev, u"%sInvalid section, %d bytes", {margin, section.size()});
        rep.log(lev, u"%s%s", {space, desc});
        return;
    }

    // Section common fields.
    rep.log(lev, u"%sTable id: 0x%X, service: 0x%X, ts: 0x%X, size: %d bytes",
            {margin, section.tableId(), section.tableIdExtension(), GetUInt16(section.payload()), section.size()});
    rep.log(lev, u"%s%s", {space, desc});
    rep.log(lev, u"%sversion: %d, last table id: 0x%X, section #: %d, segment last section #: %d, last section#: %d",
            {space, section.version(), section.payload()[5], section.sectionNumber(), section.payload()[4], section.lastSectionNumber()});

    // Display events.
    const uint8_t* data = section.payload() + EIT::EIT_PAYLOAD_FIXED_SIZE;
    size_t size = section.payloadSize() - EIT::EIT_PAYLOAD_FIXED_SIZE;
    while (size >= EIT::EIT_EVENT_FIXED_SIZE) {
        const size_t ev_size = std::min(size, EIT::EIT_EVENT_FIXED_SIZE + (GetUInt16(data + 10) & 0x0FFF));
        Time start;
        DecodeMJD(data + 2, 5, start);
        Time end(start + (MilliSecPerHour * DecodeBCD(data[7])) + (MilliSecPerMin * DecodeBCD(data[8])) + (MilliSecPerSec * DecodeBCD(data[9])));
        rep.log(lev, u"%sevent id: 0x%X, start: %s, end: %s, %d bytes", {space, GetUInt16(data), start, end, ev_size});
        data += ev_size;
        size -= ev_size;
    }
    if (size > 0) {
        rep.log(lev, u"%sinvalid %d trailing bytes", {space, size});
    }

    // Display CRC state.
    const uint32_t act_crc(GetUInt32(section.content() + section.size() - 4));
    const uint32_t exp_crc(CRC32(section.content(), section.size() - 4).value());
    rep.log(lev, u"%s%s", {space, act_crc == exp_crc ? u"valid CRC32" : u"invalid CRC32"});
}
