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
// Predefined EIT repetition profiles.
//----------------------------------------------------------------------------

const ts::EITRepetitionProfile ts::EITRepetitionProfile::SatelliteCable{
    8,       // prime_days
    {        // cycle_seconds
        2,   // PF_ACTUAL
        10,  // PF_OTHER
        10,  // SCHED_ACTUAL_PRIME
        10,  // SCHED_OTHER_PRIME
        30,  // SCHED_ACTUAL_LATER
        30   // SCHED_OTHER_LATER
    }
};

const ts::EITRepetitionProfile ts::EITRepetitionProfile::Terrestrial{
    1,       // prime_days
    {        // cycle_seconds
        2,   // PF_ACTUAL
        20,  // PF_OTHER
        10,  // SCHED_ACTUAL_PRIME
        60,  // SCHED_OTHER_PRIME
        30,  // SCHED_ACTUAL_LATER
        300  // SCHED_OTHER_LATER
    }
};


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITGenerator::EITGenerator(DuckContext& duck, PID pid, EITOption options, const EITRepetitionProfile& profile) :
    _duck(duck),
    _eit_pid(pid),
    _ts_id(0),
    _ts_id_set(false),
    _packet_index(0),
    _max_bitrate(0),
    _ts_bitrate(0),
    _next_midnight(),
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
    _ts_id = 0;
    _ts_id_set = false;
    _packet_index = 0;
    _max_bitrate = 0;
    _ts_bitrate = 0;
    _next_midnight.clear();
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
// Define the "actual" transport stream id for generated EIT's.
//----------------------------------------------------------------------------

void ts::EITGenerator::setTransportStreamId(uint16_t new_ts_id)
{
    // Check if this is a new TS id.
    if (!_ts_id_set || _ts_id != new_ts_id) {

        // Update all EIT's which switch between actual and other.
        // Loop on all services:
        for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
            const bool new_actual = it1->first.transport_stream_id == new_ts_id;
            if ((_ts_id_set && it1->first.transport_stream_id == _ts_id) || new_actual) {
                // This service shall switch between actual and other.
                regeneratePresentFollowing(it1->first, getCurrentTime());
                // Toggle all EIT schedule (all segments, all sections).
                for (auto it2 = it1->second.segments.begin(); it2 != it1->second.segments.end(); ++it2) {
                    for (auto it3 = (*it2)->sections.begin(); it3 != (*it2)->sections.end(); ++it3) {
                        (*it3)->toggleActual(new_actual);
                    }
                }
            }
        }

        // Set new TS id.
        _ts_id = new_ts_id;
        _ts_id_set = true;
        _duck.report().debug(u"EIT generator TS id set to 0x%X (%<d)", {_ts_id});

        // No longer need the PAT when the TS id is known.
        _demux.removePID(PID_PAT);
    }
}


//----------------------------------------------------------------------------
// Set new EIT generation options.
//----------------------------------------------------------------------------

void ts::EITGenerator::setOptions(EITOption options)
{
    _options = options;
    if (bool(_options & EITOption::INPUT)) {
        _demux.addPID(_eit_pid);
    }
    else {
        _demux.removePID(_eit_pid);
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
// If the reference time is not set, force it to oldest event.
//----------------------------------------------------------------------------

void ts::EITGenerator::forceReferenceTime()
{
    if (_ref_time == Time::Epoch) {

        // Need to force a reference time. Loop on all services, looking for the oldest event.
        for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
            // Get first event of first non-empty segment in the service.
            for (auto it2 = it1->second.segments.begin(); it2 != it1->second.segments.end(); ++it2) {
                if (!(*it2)->events.empty()) {
                    const Time& start_time((*it2)->events.front()->start_time);
                    if (_ref_time == Time::Epoch || start_time < _ref_time) {
                        _ref_time = start_time;
                        _ref_time_pkt = _packet_index;
                    }
                    break;
                }
            }
        }

        // Update EIT database if necessary.
        if (_ref_time != Time::Epoch) {
            _duck.report().debug(u"forcing TS time to %s (oldest event start time) at packet index %'d", {_ref_time.format(), _ref_time_pkt});
            updateForNewTime(_ref_time);
        }
    }
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
// An internal structure to store binary events from sections.
// Constructor based on EIT section payload.
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
// An internal structure to store an EIT section, ready to inject.
// Constructor to build an empty section for the specified service.
//----------------------------------------------------------------------------

ts::EITGenerator::ESection::ESection(const ServiceIdTriplet& srv, TID tid, uint8_t section_number, uint8_t last_section_number) :
    obsolete(false),
    regenerate(false),
    injected(false),
    next_inject(),
    start_time(),
    end_time(),
    section()
{
    // Build section data.
    ByteBlockPtr section_data(new ByteBlock(LONG_SECTION_HEADER_SIZE + EIT::EIT_PAYLOAD_FIXED_SIZE + SECTION_CRC32_SIZE));
    CheckNonNull(section_data.pointer());
    uint8_t* data = section_data->data();

    // Section header
    PutUInt8(data, tid);
    PutUInt16(data + 1, 0xF000 | uint16_t(section_data->size() - 3));
    PutUInt16(data + 3, srv.service_id); // table id extension
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
// Indicate that a section will be modified.
//----------------------------------------------------------------------------

void ts::EITGenerator::ESection::startModifying()
{
    // Do something only if the section is maybe still used in a packetizer.
    if (injected && !section.isNull()) {
        // Duplicate the section.
        section = new Section(*section, ShareMode::COPY);
        // Mark the new section data as no longer used by a packetizer.
        injected = false;
    }
}


//----------------------------------------------------------------------------
// Toogle the actual/other status for a section.
//----------------------------------------------------------------------------

void ts::EITGenerator::ESection::toggleActual(bool actual)
{
    if (!section.isNull()) {
        startModifying();
        section->setTableId(EIT::ToggleActual(section->tableId(), actual), true);
    }
}


//----------------------------------------------------------------------------
// Description of an EIT schedule segment (3 hours, up to 8 sections)
//----------------------------------------------------------------------------

ts::EITGenerator::ESegment::ESegment(EITGenerator& eit_gen, const ServiceIdTriplet& service_id, const Time& seg_start_time) :
    start_time(seg_start_time),
    table_id(TID_NULL),
    section_number(0),
    events(),
    sections()
{
    // Compute table id, segment and first section number.
    const bool actual = service_id.transport_stream_id == eit_gen._ts_id;
    const Time reference_midnight(eit_gen.getCurrentTime().thisDay());
    const size_t segment_number = EIT::TimeToSegment(reference_midnight, seg_start_time);
    table_id = EIT::SegmentToTableId(actual, segment_number);
    section_number = EIT::SegmentToSection(segment_number);

    // Build one empty section.
    ESectionPtr sec(new ESection(service_id, table_id, section_number, section_number));
    CheckNonNull(sec.pointer());

    // Leave the section empty for now.
    sec->section->recomputeCRC();

    // Enqueue the unique section of the segment.
    sections.push_back(sec);

    // Determine the profile of EIT for the injection queue.
    size_t index = size_t(actual ? EITProfile::SCHED_ACTUAL_PRIME : EITProfile::SCHED_OTHER_PRIME);
    if (seg_start_time - reference_midnight >= MilliSecond(eit_gen._profile.prime_days) * MilliSecPerDay) {
        index += 2; // move to profile "later"
    }

    // Enqueue the section for injection.
    sec->next_inject = eit_gen.getCurrentTime();
    eit_gen._injects[index].push_front(sec);
}


//----------------------------------------------------------------------------
// Description of an EIT schedule segment (3 hours, up to 8 sections)
//----------------------------------------------------------------------------

ts::EITGenerator::EService::EService() :
    present(),
    following(),
    segments()
{
}


//----------------------------------------------------------------------------
// Regenerate, if necessary, the EIT p/f in a service.
//----------------------------------------------------------------------------

void ts::EITGenerator::regeneratePresentFollowing(const ServiceIdTriplet& service_id, const Time& now)
{
    EService& srv(_services[service_id]);
    const bool actual = _ts_id_set && _ts_id == service_id.transport_stream_id;

    if (now == Time::Epoch || !(_options & EITOption::PF) || (actual && !(_options & EITOption::ACTUAL)) || (!actual && !(_options & EITOption::OTHER))) {
        // This type of EIT cannot be (no time ref) or shall not be (excluded) generated. If sections exist, delete them.
        if (!srv.present.isNull()) {
            markObsoleteSection(*srv.present);
            srv.present.clear();
        }
        if (!srv.following.isNull()) {
            markObsoleteSection(*srv.following);
            srv.following.clear();
        }
    }
    else {
        // Find first and second event in the service. Can be null if none is found.
        EventPtr event0;
        EventPtr event1;
        for (auto seg_iter = srv.segments.begin(); event1.isNull() && seg_iter != srv.segments.end(); ++seg_iter) {
            const ESegment& seg(**seg_iter);
            for (auto ev_iter = seg.events.begin(); event1.isNull() && ev_iter != seg.events.end(); ++ev_iter) {
                if (event0.isNull()) {
                    event0 = *ev_iter;
                }
                else {
                    event1 = *ev_iter;
                }
            }
        }

        // If the first event is not yet current, make it the "following" one.
        if (!event0.isNull() && now != Time::Epoch && now < event0->start_time) {
            event1 = event0;
            event0.clear();
        }

        // Rebuild the two sections when necessary.
        const TID tid = _ts_id_set && service_id.transport_stream_id == _ts_id ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        regeneratePresentFollowing(service_id, srv.present, tid, 0, event0);
        regeneratePresentFollowing(service_id, srv.following, tid, 1, event1);
    }
}


//----------------------------------------------------------------------------
// Regenerate, if necessary, one EIT present or following.
//----------------------------------------------------------------------------

void ts::EITGenerator::regeneratePresentFollowing(const ServiceIdTriplet& service_id, ESectionPtr& sec, TID tid, bool section_number, const EventPtr& event)
{
    if (sec.isNull()) {
        // The section did not exist, create it.
        sec = new ESection(service_id, tid, section_number, 1);
        // The initial state of the section is: no event, no CRC.
        if (event.isNull()) {
            // No event to set in the section just recompute the CRC.
            sec->section->recomputeCRC();
        }
        else {
            // Append the event in the payload and recompute the CRC.
            sec->section->appendPayload(event->event_data, true);
            sec->start_time = event->start_time;
            sec->end_time = event->end_time;
        }
        // Place the section in the inject queue.
        const size_t index = size_t(tid == TID_EIT_PF_ACT ? EITProfile::PF_ACTUAL : EITProfile::PF_OTHER);
        sec->next_inject = getCurrentTime();
        _injects[index].push_front(sec);
    }
    else if (event.isNull()) {
        // There is no more event, truncate the section payload to remove the event (if any is present).
        sec->startModifying();
        sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, true);
        sec->start_time.clear();
        sec->end_time.clear();
    }
    else if (sec->section->payloadSize() != EIT::EIT_PAYLOAD_FIXED_SIZE + event->event_data.size() ||
             ::memcmp(sec->section->payload() + EIT::EIT_PAYLOAD_FIXED_SIZE, event->event_data.data(), event->event_data.size()) != 0)
    {
        // The event is not the same as the one in the section, update the section.
        sec->startModifying();
        sec->section->truncatePayload(EIT::EIT_PAYLOAD_FIXED_SIZE, false);
        sec->section->appendPayload(event->event_data, true);
        sec->start_time = event->start_time;
        sec->end_time = event->end_time;
    }
}


//----------------------------------------------------------------------------
// Regenerate all EIT schedule in a segment.
//----------------------------------------------------------------------------

void ts::EITGenerator::regenerateSegment(const ServiceIdTriplet& service_id, Time segment_start_time)
{
    EService& srv(_services[service_id]);
    for (auto seg_iter = srv.segments.begin(); seg_iter != srv.segments.end(); ++seg_iter) {
        if ((*seg_iter)->start_time == segment_start_time) {
            regenerateSegment(service_id, **seg_iter);
            break;
        }
    }
}

void ts::EITGenerator::regenerateSegment(const ServiceIdTriplet& service_id, ESegment& segment)
{
    // First pass: check or regenerate EIT schedule sections in that segment.
    //@@@

    // Second pass: check or update last_section_number, segment_last_section_number, last_table_id
    // in all EIT schedule with the same table id.
}


//----------------------------------------------------------------------------
// Update the EIT database according to the current time.
//----------------------------------------------------------------------------

void ts::EITGenerator::updateForNewTime(Time now)
{
    // Loop on all services.
    for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
        EService& srv(it1->second);
        bool service_updated = false;

        // Remove obsolete segments.
        while (!srv.segments.empty() && srv.segments.front()->start_time + EIT::SEGMENT_DURATION <= now) {
            // The first segment is obsolete, drop it from the list.
            ESegmentPtr seg(srv.segments.front());
            srv.segments.pop_front();
            service_updated = true;

            // Mark all sections in the segment as obsolete.
            // If they are in an injection queue, they will dropped later.
            for (auto it2 = seg->sections.begin(); it2 != seg->sections.end(); ++it2) {
                markObsoleteSection(**it2);
            }
        }

        // Remove obsolete events in the first segment.
        if (!srv.segments.empty()) {
            ESegmentPtr seg(srv.segments.front());
            bool segment_updated = false;
            while (!seg->events.empty() && seg->events.front()->end_time <= now) {
                seg->events.pop_front();
                segment_updated = true;
                service_updated = true;
            }
            if (segment_updated) {
                // Some event were removed, mark all sections in the segment to be regenerated.
                markRegenerateSegment(*seg);
            }
        }

        // Renew EIT p/f of the service when necessary.
        if (service_updated) {
            // Leading events or segments were updated, the EIT p/f must be updated.
            regeneratePresentFollowing(it1->first, now);
        }
    }

    // Process the "midnight effect": All EIT schedule are organized in segments
    // which are based on the start of the current day. At midnight, we change day
    // and we need to shift all EIT sections. The affected fields are table_id,
    // section_number, last_section_number, segment_last_section_number and last_table_id.

    if (now >= _next_midnight) {
        // We passed the limit of validity for the current organization of EIT schedule.
        const Time current_midnight(now.thisDay());
        _next_midnight = current_midnight + MilliSecPerDay;

        //@@@
    }
}


//----------------------------------------------------------------------------
// Load EPG data from binary events descriptions.
//----------------------------------------------------------------------------

void ts::EITGenerator::loadEvents(const ServiceIdTriplet& service_id, const uint8_t* data, size_t size)
{
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
            // Could not get the event, probably incorrect data.
            break;
        }

        // Discard events in the past.
        if (now != Time::Epoch && ev->end_time <= now) {
            continue;
        }

        // Locate or allocate the segment for that event.
        const Time seg_start_time(EIT::SegmentStart(ev->start_time));
        ESegmentPtr seg;

        // According to ETSI TS 101 211, all contiguous segments in the same table_id must be present.
        // So if a segment must be created, we enforce that all intermediate segments are created as empty.
        if (srv.segments.empty()) {
            // First segment to be created, there will be one only for now.
            seg = new ESegment(*this, service_id, seg_start_time);
            srv.segments.push_back(seg);
        }
        else if (seg_start_time < srv.segments.front()->start_time) {
            // New segments must be added on the front of the list.
            do {
                seg = new ESegment(*this, service_id, srv.segments.front()->start_time - EIT::SEGMENT_DURATION);
                srv.segments.push_front(seg);
            } while (seg->start_time > seg_start_time);
            assert(seg->start_time == seg_start_time);
        }
        else if (seg_start_time > srv.segments.back()->start_time) {
            // New segments must be added after the end of the list.
            do {
                seg = new ESegment(*this, service_id, srv.segments.back()->start_time + EIT::SEGMENT_DURATION);
                srv.segments.push_back(seg);
            } while (seg->start_time < seg_start_time);
            assert(seg->start_time == seg_start_time);
        }
        else {
            // The segment already exists. Search it in the list.
            for (auto it = srv.segments.begin(); seg.isNull() && it != srv.segments.end(); ++it) {
                if ((*it)->start_time == seg_start_time) {
                    seg = *it;
                }
            }
            assert(!seg.isNull());
        }

        // Insert the binary event in the list of events for that segment.
        auto ev_iter = seg->events.begin();
        while (ev_iter != seg->events.end() && (*ev_iter)->start_time < ev->start_time) {
            ++ev_iter;
        }
        if (ev_iter != seg->events.end() && (*ev_iter)->event_id == ev->event_id && (*ev_iter)->event_data == ev->event_data) {
            // Duplicate event, ignore it.
            continue;
        }
        seg->events.insert(ev_iter, ev);
        ev_count++;

        // Mark all EIT schedule in this segment as to be regenerated.
        markRegenerateSegment(*seg);
    }

    // If some events were added, it may be necessary to regenerate the EIT p/f in this service.
    if (ev_count > 0) {
        regeneratePresentFollowing(service_id, now);
    }
}


//----------------------------------------------------------------------------
// Load EPG data from an EIT section.
//----------------------------------------------------------------------------

void ts::EITGenerator::loadEvents(const Section& section)
{
    // Section id and payload.
    const TID tid = section.tableId();
    const uint8_t* const pl_data = section.payload();
    const size_t pl_size = section.payloadSize();

    // Filter valid EIT's.
    if (!section.isValid() || !EIT::IsEIT(tid) || pl_size < EIT::EIT_PAYLOAD_FIXED_SIZE) {
        return;
    }

    // Get the service id triplet for this EIT/
    const ServiceIdTriplet srv(EIT::GetService(section));

    // If the TS is not yet known, we cannot sort actual and other EIT's.
    if (!_ts_id_set && EIT::IsActual(tid)) {
        // TS id not set but the incoming EIT is an actual one, use its TS id as current TS id.
        setTransportStreamId(srv.transport_stream_id);
    }
    if (!_ts_id_set) {
        _duck.report().warning(u"EIT generator TS id not set, event not stored in EPG, TS: 0x%X (%<d), service: 0x%X (%<d)", {srv.transport_stream_id, srv.service_id});
        return;
    }
    if ((!(_options & EITOption::ACTUAL) && srv.transport_stream_id == _ts_id) || (!(_options & EITOption::OTHER) && srv.transport_stream_id != _ts_id)) {
        // We do not generate this type of EIT (actual or other).
        return;
    }

    // Load all events in the EIT payload.
    loadEvents(srv, pl_data + EIT::EIT_PAYLOAD_FIXED_SIZE, pl_size - EIT::EIT_PAYLOAD_FIXED_SIZE);
}


//----------------------------------------------------------------------------
// Load EPG data from all EIT sections in a section file.
//----------------------------------------------------------------------------

void ts::EITGenerator::loadEvents(const SectionFile& secfile)
{
    const SectionPtrVector& sections(secfile.sections());
    for (size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].isNull()) {
            loadEvents(*sections[i]);
        }
    }
}


//----------------------------------------------------------------------------
// Save all current EIT sections in a section file.
//----------------------------------------------------------------------------

void ts::EITGenerator::saveEITs(SectionFile& secfile)
{
    // Ensure all EIT sections are correctly regenerated.
    forceReferenceTime();
    updateForNewTime(getCurrentTime());

    // Loop on all services, saving all EIT p/f.
    for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
        if (!it1->second.present.isNull()) {
            secfile.add(it1->second.present->section);
        }
        if (!it1->second.following.isNull()) {
            secfile.add(it1->second.following->section);
        }
    }

    // Loop on all services again, saving all EIT schedule.
    for (auto it1 = _services.begin(); it1 != _services.end(); ++it1) {
        // Loop on all segments in the service.
        for (auto it2 = it1->second.segments.begin(); it2 != it1->second.segments.end(); ++it2) {
            ESegment& seg(**it2);
            if (!seg.sections.empty()) {
                // Regenerate all sections if necessary.
                if (seg.sections.front()->regenerate) {
                    regenerateSegment(it1->first, seg);
                }
                // Save all EIT schedule in this segment.
                for (auto it3 = seg.sections.begin(); it3 != seg.sections.end(); ++it3) {
                    secfile.add((*it3)->section);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Mark a section as obsolete, garbage collect obsolete sections
//----------------------------------------------------------------------------

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
                // Loop on all section in the queue.
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
// Mark all sections in a segment as to be regenerated.
//----------------------------------------------------------------------------

void ts::EITGenerator::markRegenerateSegment(ESegment& seg)
{
    for (auto it = seg.sections.begin(); it != seg.sections.end(); ++it) {
        (*it)->regenerate = true;
    }
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::EITGenerator::doStuffing()
{
    // Never stuff TS packets, always pack EIT sections.
    return false;
}

void ts::EITGenerator::provideSection(SectionCounter counter, SectionPtr& section)
{
    // Look for an EIT section with a due time no later than current time.
    const Time now(getCurrentTime());

    // Loop on all injection queues, in decreasing order of priority.
    for (size_t index = 0; index < _injects.size(); ++index) {

        // Check if the first section in the queue is ready for injection.
        // Loop on obsolete events. Return on first injected event.
        while (!_injects[index].empty() && _injects[index].front()->next_inject <= now) {

            // Remove the first section from the queue.
            const ESectionPtr sec(_injects[index].front());
            _injects[index].pop_front();

            // If the section is part of a segment which has been modified, regenerate all sections from this segment.
            if (sec->regenerate && !sec->obsolete) {
                regenerateSegment(EIT::GetService(*sec->section), EIT::SegmentStart(sec->start_time));
            }

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
                sec->next_inject = now + _profile.cycle_seconds[index] * MilliSecPerSec;
                _injects[index].push_back(sec);
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

    if (tid == TID_PAT && !_ts_id_set) {
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
