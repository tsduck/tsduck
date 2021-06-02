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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITGenerator::EITGenerator(DuckContext& duck, PID pid, int options, const EITRepetitionProfile& profile) :
    _duck(duck),
    _eit_pid(pid),
    _ts_id(0),
    _ts_id_set(false),
    _options(options),
    _profile(profile),
    _demux(_duck, nullptr, this),
    _packetizer(_duck, _eit_pid, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _sections()
{
    // We need the PAT as long as the TS id is not known.
    _demux.addPID(PID_PAT);

    // We need to analyze input EIT's only if they feed the EPG.
    if (_options & EIT_INPUT) {
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
}


//----------------------------------------------------------------------------
// Set new EIT generation options.
//----------------------------------------------------------------------------

void ts::EITGenerator::setOptions(int options)
{
    _options = options;
    if (_options & EIT_INPUT) {
        _demux.addPID(_eit_pid);
    }
    else {
        _demux.removePID(_eit_pid);
    }
}


//----------------------------------------------------------------------------
// Reset the EIT generator to default state.
//----------------------------------------------------------------------------

void ts::EITGenerator::reset()
{
    _ts_id = 0;
    _ts_id_set = false;
    _demux.reset();
    _demux.addPID(PID_PAT);
    _packetizer.reset();
    _sections.clear();
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
    // If the incoming EIT is an actual one, use its TS id as current TS id.
    const ServiceIdTriplet srv(EIT::GetService(section));
    if (!_ts_id_set && EIT::IsActual(tid)) {
        setTransportStreamId(srv.transport_stream_id);
    }
    if (!_ts_id_set || ((_options & EIT_ACTUAL) == 0 && srv.transport_stream_id == _ts_id) || ((_options & EIT_OTHER) == 0 && srv.transport_stream_id != _ts_id)) {
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
    if (section.tableId() == TID_PAT && !_ts_id_set) {
        // A PAT section is used to define the transport stream id if not already known.
        setTransportStreamId(section.tableIdExtension());
    }
    else if (EIT::IsEIT(section.tableId()) && (_options & EIT_INPUT) != 0) {
        // Use input EIT's as EPG data when specified in the generation options.
        loadEvents(section);
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
}
