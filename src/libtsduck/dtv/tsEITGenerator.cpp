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
    _bitrate(0),
    _ref_time(),
    _ref_time_pkt(0),
    _options(options),
    _profile(profile),
    _demux(_duck, nullptr, this),
    _packetizer(_duck, _eit_pid, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _sections()
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
// Define the "actual" transport stream id for generated EIT's.
//----------------------------------------------------------------------------

void ts::EITGenerator::setTransportStreamId(uint16_t ts_id)
{
    _ts_id = ts_id;
    _ts_id_set = true;
    _demux.removePID(PID_PAT);
    _duck.report().debug(u"EIT generator TS id set to 0x%X (%<d)", {_ts_id});
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
// Set/get current time in the stream processing.
//----------------------------------------------------------------------------

void ts::EITGenerator::setCurrentTime(Time current_utc)
{
    _ref_time = current_utc;
    _ref_time_pkt = _packet_index;

    if (_duck.report().debug()) {
        _duck.report().debug(u"setting TS time to %s at packet index %'d", {_ref_time.format(), _ref_time_pkt});
    }
}

ts::Time ts::EITGenerator::getCurrentTime(Time current_utc) const
{
    return _ref_time == Time::Epoch ? Time::Epoch : _ref_time + PacketInterval(_bitrate, _packet_index - _ref_time_pkt);
}


//----------------------------------------------------------------------------
// Reset the EIT generator to default state.
//----------------------------------------------------------------------------

void ts::EITGenerator::reset()
{
    _ts_id = 0;
    _ts_id_set = false;
    _packet_index = 0;
    _bitrate = 0;
    _ref_time.clear();
    _ref_time_pkt = 0;
    _demux.reset();
    _demux.addPID(PID_PAT);
    _packetizer.reset();
    _sections.clear();
}


//----------------------------------------------------------------------------
// An internal structure to store binary events from sections.
// Constructor based on EIT section payload.
//----------------------------------------------------------------------------

ts::EITGenerator::BinaryEvent::BinaryEvent(const uint8_t*& data, size_t& size) :
    start_time(),
    end_time(),
    event_data()
{
    size_t event_size = size;

    if (size >= EIT::EIT_EVENT_FIXED_SIZE) {
        event_size = std::min(size, EIT::EIT_EVENT_FIXED_SIZE + (GetUInt16(data + 10) & 0x0FFF));
        DecodeMJD(data + 2, 5, start_time);
        end_time = start_time + (MilliSecPerHour * DecodeBCD(data[7])) + (MilliSecPerMin * DecodeBCD(data[8])) + (MilliSecPerSec * DecodeBCD(data[9]));
        event_data.copy(data, event_size);
    }

    data += event_size;
    size -= event_size;
}


//----------------------------------------------------------------------------
// Load EPG data from an EIT section.
//----------------------------------------------------------------------------

void ts::EITGenerator::loadEvents(const Section& section)
{
    // Filter the right EIT's.
    const TID tid = section.tableId();
    if (!section.isValid() || !EIT::IsEIT(tid) || section.payloadSize() < EIT::EIT_PAYLOAD_FIXED_SIZE) {
        return;
    }

    // If the TS is not yet known, we cannot sort actual and other EIT's.
    const ServiceIdTriplet srv(EIT::GetService(section));
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



    //@@@
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

void ts::EITGenerator::saveEITs(SectionFile& sections) const
{
    //@@@
}


//----------------------------------------------------------------------------
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

    //@@@

    // Count packets in the stream.
    _packet_index++;
}
