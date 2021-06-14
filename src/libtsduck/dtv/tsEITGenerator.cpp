//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsEITGenerator.h"
#include "tsDuckContext.h"
#include "tsEIT.h"
#include "tsMJD.h"
#include "tsBCD.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITGenerator::EITGenerator(DuckContext& duck, PID pid, EITOption options, const EITRepetitionProfile& profile) :
    _duck(duck),
    _eit_pid(pid),
    _actual_ts_id(0),
    _actual_ts_id_set(false),
    _regenerate(false),
    _packet_index(0),
    _max_bitrate(0),
    _ts_bitrate(0),
    _ref_time(),
    _ref_time_pkt(0),
    _eit_inter_pkt(0),
    _last_eit_pkt(0),
    _options(options),
    _profile(profile),
    _demux(_duck, nullptr, this),
    _packetizer(_duck, _eit_pid, this),
    _services(),
    _injects(),
    _obsolete_count(0)
{
    // We need the PAT as long as the TS id is not known.
    _demux.addPID(PID_PAT);

    // We always get TDT/TOT.
    _demux.addPID(PID_TDT);

    // We need to analyze input EIT's only if they feed the EPG.
    if (bool(_options & EITOption::INPUT)) {
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
    _obsolete_count = 0;
}


//----------------------------------------------------------------------------
// Event: Constructor of the structure containing binary events.
//----------------------------------------------------------------------------

ts::EITGenerator::Event::Event(const uint8_t*& data, size_t& size) :
    event_id(0),
    start_time(),
    end_time(),
    event_data()
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
// ESection: Constructor of the structure for  a section, ready to inject.
//----------------------------------------------------------------------------

ts::EITGenerator::ESection::ESection(const ServiceIdTriplet& srv, TID tid, uint8_t section_number, uint8_t last_section_number) :
    obsolete(false),
    injected(false),
    next_inject(),
    section()
{
    // Build section data.
    ByteBlockPtr section_data(new ByteBlock(LONG_SECTION_HEADER_SIZE + EIT::EIT_PAYLOAD_FIXED_SIZE + SECTION_CRC32_SIZE));
    CheckNonNull(section_data.pointer());
    uint8_t* data = section_data->data();

    // Section header
    PutUInt8(data, tid);
    PutUInt16(data + 1, 0xF000 | uint16_t(section_data->size() - 3));
    PutUInt16(data + 3, srv.service_id);       // table id extension
    PutUInt8(data + 5, 0xC1 | uint8_t(srv.version << 1));
    PutUInt8(data + 6, section_number);
    PutUInt8(data + 7, last_section_number);

    // EIT section payload, without event.
    PutUInt16(data + 8, srv.transport_stream_id);
    PutUInt16(data + 10, srv.original_network_id);
    PutUInt8(data + 12, last_section_number);  // last section number in this segment
    PutUInt8(data + 13, tid);                  // last table id in this service

    // Build a section from the binary data.
    section = new Section(section_data, PID_NULL, CRC32::IGNORE);
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
// ESegment: Constructor of an EIT sched segment (3 hours, up to 8 sections)
//----------------------------------------------------------------------------

ts::EITGenerator::ESegment::ESegment(const Time& seg_start_time) :
    start_time(seg_start_time),
    regenerate(false),
    events(),
    sections()
{
}


//----------------------------------------------------------------------------
// EService: Constructor of the description of one service.
//----------------------------------------------------------------------------

ts::EITGenerator::EService::EService() :
    regenerate(false),
    pf(),
    segments()
{
}


//----------------------------------------------------------------------------
// Load EPG data from binary events descriptions.
//----------------------------------------------------------------------------

bool ts::EITGenerator::loadEvents(const ServiceIdTriplet& service_id, const uint8_t* data, size_t size)
{
    bool success = true;

    // Description of the service.
    EService& srv(_services[service_id]);

    // Number of loaded event.
    size_t ev_count = 0;

    // Current time according to the transport stream. Can be "Epoch" (undefined).
    const Time now(getCurrentTime());

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
            continue;
        }

        // Locate or allocate the segment for that event. At this stage, we only create this
        // segment if necessary. This is the minimum to store an event. We do not try to create
        // empty intermediate segments. This will be done in regenerateSchedule().

        const Time seg_start_time(EIT::SegmentStartTime(ev->start_time));
        auto seg_iter = srv.segments.begin();
        while (seg_iter != srv.segments.end() && (*seg_iter)->start_time < seg_start_time) {
            ++seg_iter;
        }
        if (seg_iter == srv.segments.end() || (*seg_iter)->start_time != seg_start_time) {
            // The segment does not exist, create it.
            _duck.report().debug(u"creating EIT segment starting at %s for %s", {seg_start_time.format(), service_id});
            const ESegmentPtr seg(new ESegment(seg_start_time));
            CheckNonNull(seg.pointer());
            seg_iter = srv.segments.insert(seg_iter, seg);
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
        _duck.report().log(2, u"loaded event id 0x%X (%<d), %s, starting %s", {ev->event_id, service_id, ev->start_time.format()});
        seg.events.insert(ev_iter, ev);
        ev_count++;

        // Mark all EIT schedule in this segment as to be regenerated.
        _regenerate = srv.regenerate = seg.regenerate = true;
    }

    // If some events were added, it may be necessary to regenerate the EIT p/f in this service.
    if (ev_count > 0) {
        regeneratePresentFollowing(service_id, now);
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
        for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
            // Get first event of first non-empty segment in the service.
            for (auto it2 = it1->second.segments.begin(); it2 != it1->second.segments.end(); ++it2) {
                if (!(*it2)->events.empty()) {
                    const Time& start_time((*it2)->events.front()->start_time);
                    if (_ref_time == Time::Epoch || start_time < _ref_time) {
                        _ref_time = start_time;
                        _ref_time_pkt = _packet_index;
                    }
                    break; // found oldest in this service, move to next service
                }
            }
        }
        if (_ref_time != Time::Epoch) {
            _duck.report().debug(u"forcing TS time to %s (oldest event start time) at packet index %'d", {_ref_time.format(), _ref_time_pkt});
        }
    }

    // Ensure all EIT sections are correctly regenerated.
    const Time now(getCurrentTime());
    updateForNewTime(now);
    regenerateSchedule(now);

    size_t pf_count = 0;
    size_t sched_count = 0;

    // Loop on all services, saving all EIT p/f.
    for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
        for (size_t i = 0; i < it1->second.pf.size(); ++i) {
            if (!it1->second.pf[i].isNull()) {
                sections.push_back(it1->second.pf[i]->section);
                pf_count++;
            }
        }
    }

    // Loop on all services again, saving all EIT schedule.
    for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
        for (auto it2 = it1->second.segments.begin(); it2 != it1->second.segments.end(); ++it2) {
            for (auto it3 = (*it2)->sections.begin(); it3 != (*it2)->sections.end(); ++it3) {
                sections.push_back((*it3)->section);
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

    // Update all EIT's which switch between actual and other.
    for (auto srv_iter = _services.begin(); srv_iter != _services.end(); ++srv_iter) {
        EService& srv(srv_iter->second);

        // Does this service changes between actual and other?
        const bool new_actual = srv_iter->first.transport_stream_id == new_ts_id;
        const bool new_other = srv_iter->first.transport_stream_id == old_ts_id;
        const bool need_eit = (new_actual && bool(_options & EITOption::ACTUAL)) || (new_other && bool(_options & EITOption::OTHER));

        // Test if this service shall switch between actual and other.
        if (new_other || new_actual) {

            // Process EIT p/f.
            if (bool(_options & EITOption::PF)) {
                if (need_eit && (srv.pf[0].isNull() || srv.pf[1].isNull())) {
                    // At least one EIT p/f shall be rebuilt.
                    regeneratePresentFollowing(srv_iter->first, now);
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
            if (bool(_options & EITOption::SCHED)) {
                if ((_options & (EITOption::ACTUAL | EITOption::OTHER)) == (EITOption::ACTUAL | EITOption::OTHER)) {
                    // Actual and others are both requested. Toggle the state of existing sections.
                    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
                        ESegment& seg(**seg_iter);
                        for (auto sec_iter = seg.sections.begin(); sec_iter != seg.sections.end(); ++sec_iter) {
                            (*sec_iter)->toggleActual(new_actual);
                        }
                    }
                }
                else if (need_eit) {
                    // The EIT schedule for that service were not there, we need them now, regenerate later.
                    _regenerate = srv.regenerate = true;
                    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
                        (*seg_iter)->regenerate = true;
                    }
                }
                else {
                    // We no longer need the EIT schedule.
                    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
                        ESegment& seg(**seg_iter);
                        for (auto sec_iter = seg.sections.begin(); sec_iter != seg.sections.end(); ++sec_iter) {
                            markObsoleteSection(**sec_iter);
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

void ts::EITGenerator::setOptions(EITOption options)
{
    // Update the options.
    const EITOption old_options = _options;
    _options = options;

    // If the new options request to load events from input EIT's, demux the EIT PID.
    if (bool(options & EITOption::INPUT)) {
        _demux.addPID(_eit_pid);
    }
    else {
        _demux.removePID(_eit_pid);
    }

    // Current time according to the transport stream. Can be "Epoch" (undefined).
    const Time now(getCurrentTime());

    // Check the configuration has changed for EIT p/f and EIT schedule, respectively.
    const bool pf_changed = (_options & (EITOption::PF | EITOption::ACTUAL | EITOption::OTHER)) != (old_options & (EITOption::PF | EITOption::ACTUAL | EITOption::OTHER));
    const bool sched_changed = (_options & (EITOption::SCHED | EITOption::ACTUAL | EITOption::OTHER)) != (old_options & (EITOption::SCHED | EITOption::ACTUAL | EITOption::OTHER));

    // If the combination of EIT to generate has changed, regenerate EIT.
    if ((pf_changed || sched_changed) && _actual_ts_id_set && now != Time::Epoch) {

        // Loop on all services.
        for (auto srv_iter = _services.begin(); srv_iter != _services.end(); ++srv_iter) {
            EService& srv(srv_iter->second);
            const bool actual = srv_iter->first.transport_stream_id == _actual_ts_id;
            const bool need_eit = (actual && bool(_options & EITOption::ACTUAL)) || (!actual && bool(_options & EITOption::OTHER));

            // Process EIT p/f.
            if (pf_changed) {
                if (!need_eit || !(_options & EITOption::PF)) {
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
                    regeneratePresentFollowing(srv_iter->first, now);
                }
            }

            // Process EIT schedule (all segments, all sections).
            if (sched_changed) {
                if (!need_eit || !(_options & EITOption::SCHED)) {
                    // We no longer need the EIT schedule.
                    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
                        ESegment& seg(**seg_iter);
                        for (auto sec_iter = seg.sections.begin(); sec_iter != seg.sections.end(); ++sec_iter) {
                            markObsoleteSection(**sec_iter);
                        }
                        seg.sections.clear();
                        seg.regenerate = false;
                    }
                }
                else {
                    // The EIT schedule for that service were not there, we need them now, regenerate later.
                    _regenerate = srv.regenerate = true;
                    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
                        (*seg_iter)->regenerate = true;
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Set TS bitrate and maximum EIT bitrate of the EIT PID.
//----------------------------------------------------------------------------

void ts::EITGenerator::setBitRateField(BitRate EITGenerator::* field, BitRate bitrate)
{
    // Update the target field (_ts_bitrate or _max_bitrate).
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
    _duck.report().debug(u"setting TS time to %s at packet index %'d", {_ref_time.format(), _ref_time_pkt});

    // Update EIT database if necessary.
    updateForNewTime(_ref_time);
}


//----------------------------------------------------------------------------
// Mark a segment or section as obsolete, garbage collect obsolete sections
//----------------------------------------------------------------------------

void ts::EITGenerator::markObsoleteSegment(ESegment &seg)
{
    for (auto it = seg.sections.begin(); it != seg.sections.end(); ++it) {
        markObsoleteSection(**it);
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
// Regenerate, if necessary, the EIT p/f in a service.
//----------------------------------------------------------------------------

void ts::EITGenerator::regeneratePresentFollowing(const ServiceIdTriplet& service_id, const Time& now)
{
    // Don't know what to generate if the actual TS or current time are unknown.
    if (!_actual_ts_id_set || now == Time::Epoch) {
        return;
    }

    EService& srv(_services[service_id]);
    const bool actual = _actual_ts_id == service_id.transport_stream_id;

    if (!(_options & EITOption::PF) || (actual && !(_options & EITOption::ACTUAL)) || (!actual && !(_options & EITOption::OTHER))) {
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
        // Start with following, then present, because they are pushed in front of the queue in that order.
        // Thus, the present will be injected first, then the following. This is not mandatory but nicer.
        const TID tid = actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        regeneratePresentFollowingSection(service_id, srv.pf[1], tid, 1, events[1]);
        regeneratePresentFollowingSection(service_id, srv.pf[0], tid, 0, events[0]);
    }
}


//----------------------------------------------------------------------------
// Regenerate, if necessary, one EIT present or following.
//----------------------------------------------------------------------------

void ts::EITGenerator::regeneratePresentFollowingSection(const ServiceIdTriplet& service_id, ESectionPtr& sec, TID tid, bool section_number, const EventPtr& event)
{
    if (sec.isNull()) {
        // The section did not exist, create it.
        sec = new ESection(service_id, tid, section_number, 1);
        CheckNonNull(sec.pointer());
        // The initial state of the section is: no event, no CRC.
        if (event.isNull()) {
            // No event to set in the section just recompute the CRC.
            sec->section->recomputeCRC();
        }
        else {
            // Append the event in the payload and recompute the CRC.
            sec->section->appendPayload(event->event_data, true);
        }
        // Place the section in the inject queue.
        const size_t index = size_t(tid == TID_EIT_PF_ACT ? EITProfile::PF_ACTUAL : EITProfile::PF_OTHER);
        sec->next_inject = getCurrentTime();
        _injects[index].push_front(sec);
    }
    else if (event.isNull()) {
        // The section already exists. It must be already in an injection queue.
        // There is no more event, truncate the section payload to remove the event (if any is present).
        if (sec->section->tableId() != tid || sec->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE) {
            sec->startModifying();
            sec->section->setTableId(tid, false);
            sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, true);
        }
    }
    else if (sec->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE + event->event_data.size() ||
             ::memcmp(sec->section->payload() + EIT::EIT_PAYLOAD_FIXED_SIZE, event->event_data.data(), event->event_data.size()) != 0)
    {
        // The section already exists. It must be already in an injection queue.
        // The event is not the same as the one in the section, update the section.
        sec->startModifying();
        sec->section->setTableId(tid, false);
        sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, false);
        sec->section->appendPayload(event->event_data, true);
    }
    else if (sec->section->tableId() != tid) {
        // The same event is already in the section but the table id changed (because the TS id changed).
        sec->startModifying();
        sec->section->setTableId(tid, true);
    }
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

    // Loop on all services, regenerating those which are marked for regeneration.
    for (auto srv_iter = _services.begin(); srv_iter != _services.end(); ++srv_iter) {
        if (srv_iter->second.regenerate) {

            const ServiceIdTriplet& service_id(srv_iter->first);
            EService& srv(srv_iter->second);
            const bool actual = service_id.transport_stream_id == _actual_ts_id;

            // Check if EIT schedule are needed for the service.
            const bool need_eits = (actual && (_options & (EITOption::SCHED | EITOption::ACTUAL)) == (EITOption::SCHED | EITOption::ACTUAL)) ||
                                   (!actual && (_options & (EITOption::SCHED | EITOption::OTHER)) == (EITOption::SCHED | EITOption::OTHER));

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
                const ESegmentPtr seg(new ESegment(last_midnight));
                CheckNonNull(seg.pointer());
                seg->regenerate = true;
                srv.segments.push_front(seg);
            }

            // Loop on all segments. The first segment must be at last midnight.
            Time seg_time(last_midnight);
            size_t seg_index = 0;
            for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {

                // Enforce the existence of contiguous segments. Create missing segments when necessary.
                if ((*seg_iter)->start_time != seg_time) {
                    assert((*seg_iter)->start_time > seg_time);
                    const ESegmentPtr seg(new ESegment(seg_time));
                    CheckNonNull(seg.pointer());
                    seg->regenerate = true;
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
                    const TID table_id = EIT::SegmentToTableId(actual, seg_index);
                    uint8_t section_number = EIT::SegmentToSection(seg_index);

                    // We need at least one section, possibly empty, in each segment.
                    if (seg.sections.empty()) {
                        const ESectionPtr sec(new ESection(service_id, table_id, section_number, section_number));
                        CheckNonNull(sec.pointer());
                        seg.sections.push_back(sec);
                    }

                    // Update or generate all sections.
                    auto ev_iter = seg.events.begin();
                    auto sec_iter = seg.sections.begin();
                    while (ev_iter != seg.events.end()) {

                        // Check if the current event can fit into the current section.
                        if ((*sec_iter)->section->payloadSize() + (*ev_iter)->event_data.size() > MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE) {
                            // Need to create another section in this segment.
                            // Close current section and move to next one.
                            (*sec_iter++)->section->recomputeCRC();
                            section_number++;
                            // If too many events for that segment, skip the last ones.
                            if (seg.events.size() >= EIT::SECTIONS_PER_SEGMENT) {
                                break;
                            }
                            // Move to next section.
                            if (sec_iter == seg.sections.end()) {
                                // No next section, create one.
                                const ESectionPtr sec(new ESection(service_id, table_id, section_number, section_number));
                                CheckNonNull(sec.pointer());
                                sec_iter = seg.sections.insert(sec_iter, sec);
                            }
                        }

                        // Now append the event to the section payload.
                        (*sec_iter)->section->appendPayload((*ev_iter)->event_data, false);
                    }

                    // Close last section.
                    (*sec_iter++)->section->recomputeCRC();

                    // Deallocate remaining sections, if any.
                    while (sec_iter != seg.sections.end()) {
                        markObsoleteSection(**sec_iter);
                        sec_iter = seg.sections.erase(sec_iter);
                    }
                }

                // Clear segment regeneration flag.
                seg.regenerate = false;

                // Time and index of next expected segment:
                seg_time += EIT::SEGMENT_DURATION;
                seg_index++;
            }

            // Fix synthetic fields in all EIT-schedule sections: last_section_number, segment_last_section_number, last_table_id.
            if (need_eits) {
                assert(!srv.segments.empty());
                assert(!srv.segments.back()->sections.empty());

                seg_index = srv.segments.size();
                TID previous_table_id = TID_NULL;
                TID last_table_id = TID_NULL;
                uint8_t last_section_number = 0;

                // Loop on segments from last to first.
                for (auto seg_iter = srv.segments.rbegin(); seg_iter != srv.segments.rend(); ++seg_iter) {
                    ESegment& seg(**seg_iter);
                    assert(!seg.sections.empty());

                    const TID table_id = EIT::SegmentToTableId(actual, --seg_index);
                    uint8_t section_number = EIT::SegmentToSection(seg_index);
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
                    for (auto sec_iter = seg.sections.begin(); sec_iter != seg.sections.end(); ++sec_iter) {
                        ESection& sec(**sec_iter);
                        const uint8_t* pl = sec.section->payload();
                        if (sec.section->sectionNumber() != section_number ||
                            sec.section->lastSectionNumber() != last_section_number ||
                            pl[4] != segment_last_section_number ||
                            pl[5] != last_table_id)
                        {
                            sec.section->setSectionNumber(section_number, false);
                            sec.section->setLastSectionNumber(last_section_number, false);
                            sec.section->setUInt8(4, segment_last_section_number, false);
                            sec.section->setUInt8(5, last_table_id, true);
                        }
                        section_number++;
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
    for (auto srv_iter = _services.begin(); srv_iter != _services.end(); ++srv_iter) {

        const ServiceIdTriplet& service_id(srv_iter->first);
        EService& srv(srv_iter->second);
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

        // Remove obsolete events in the first segments (containing "now").
        if (seg_iter != srv.segments.end()) {
            ESegment& seg(**seg_iter);
            while (!seg.events.empty() && seg.events.front()->end_time <= now) {
                seg.events.pop_front();
                _regenerate = srv.regenerate = seg.regenerate = true;
            }
        }

        // Renew EIT p/f of the service when necessary.
        if (srv.regenerate) {
            regeneratePresentFollowing(service_id, now);
        }
    }
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::EITGenerator::doStuffing()
{
    return bool(_options & EITOption::STUFFING);
}

void ts::EITGenerator::provideSection(SectionCounter counter, SectionPtr& section)
{
    // Look for an EIT section with a due time no later than current time.
    const Time now(getCurrentTime());

    // Make sure the EIT schedule are up-to-date.
    regenerateSchedule(now);

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

                // Requeue next iteration of that section. Check which injection queue shall be used.
                // In the general case, this is the same queue. But if the TS id has changed, actual and
                // other EIT may be swapped. EIT schedule "later" may become "prime" when changing day.
                size_t new_index = index;
                const TID tid = section->tableId();
                const bool actual = EIT::IsActual(tid);
                if (actual && new_index % 2 == 1) {
                    // EIT actual but the queue is "other" (all EITProfile::*_OTHER are odd values).
                    // Now use the associated "actual" queue.
                    // An "actual" queue immediately preceeds its associated "other" queue.
                    new_index--;
                }
                else if (!actual && new_index % 2 == 0) {
                    // EIT other but the queue is "actual" (all EITProfile::*_ACTUAL are even values).
                    // Now use the associated "other" queue.
                    // An "other" queue immediately follows its associated "actual" queue.
                    new_index++;
                }
                if (new_index >= size_t(EITProfile::SCHED_ACTUAL_LATER) && tid < _profile.laterTableId(actual)) {
                    // This is an EIT schedule in the "later" queue but it is now a "prime" one.
                    // The associated "prime" queue is 2 index before the "later" one.
                    new_index -= 2;
                }
                sec->next_inject = now + _profile.cycle_seconds[new_index] * MilliSecPerSec;
                _injects[new_index].push_back(sec);
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
    else if (EIT::IsEIT(tid) && bool(_options & EITOption::INPUT)) {
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

        // Update EIT's according to current time.
        updateForNewTime(getCurrentTime());

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
