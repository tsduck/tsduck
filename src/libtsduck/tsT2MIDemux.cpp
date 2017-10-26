//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "tsT2MIDemux.h"
#include "tsT2MIPacket.h"
#include "tsT2MIDescriptor.h"
#include "tsPAT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::T2MIDemux::PIDContext::PIDContext() :
    continuity(0),
    sync(false),
    t2mi()
{
}

ts::T2MIDemux::T2MIDemux(T2MIHandlerInterface* t2mi_handler, const PIDSet& pid_filter) :
    SuperClass(pid_filter),
    _handler(t2mi_handler),
    _pids(),
    _psi_demux(this)
{
    immediateReset();
}

ts::T2MIDemux::~T2MIDemux()
{
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built PES packets).
//----------------------------------------------------------------------------

void ts::T2MIDemux::immediateReset()
{
    _pids.clear();

    // Reset the PSI demux since the transport may be completely different.
    _psi_demux.reset();

    // To get PID's with T2-MI, we need to analyze the PMT's.
    // To get the PMT PID's, we need to analyze the PAT.
    _psi_demux.addPID(PID_PAT);
}

void ts::T2MIDemux::immediateResetPID(PID pid)
{
    _pids.erase(pid);
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::T2MIDemux::feedPacket(const TSPacket& pkt)
{
    const PID pid = pkt.getPID();

    // Submit the packet to the PSI handler to detect T2-MI streams.
    _psi_demux.feedPacket(pkt);

    // If the packet is not filtered or scrambled, nothing more to do.
    if (!_pid_filter[pid] || pkt.isScrambled()) {
        return;
    }

    // Get / create PID context.
    PIDContext& pc(_pids[pid]);

    // Check if we loose synchronization.
    if (pc.sync && (pkt.getDiscontinuityIndicator() || pkt.getCC() != ((pc.continuity + 1) & CC_MASK))) {
        pc.t2mi.clear();
        pc.sync = false;
    }

    // Keep track of continuity counters.
    pc.continuity = pkt.getCC();

    // Locate packet payload.
    const uint8_t* data = pkt.getPayload();
    size_t size = pkt.getPayloadSize();

    // Process packet with Payload Unit Start Indicator.
    if (pkt.getPUSI()) {

        // The first byte in the TS payload is a pointer field to the start of a new T2-MI packet.
        // Note: this is exactly the same mechanism as section packetization.
        const size_t pf = size == 0 ? 0 : data[0];
        if (1 + pf >= size) {
            // There is no pointer field or it points outside the TS payload. Loosing sync.
            pc.t2mi.clear();
            pc.sync = false;
            return;
        }

        // Remove pointer field from packet payload.
        data++;
        size--;

        // If we were previously desynchronized, we are back on track.
        if (!pc.sync) {
            pc.sync = true;
            // Skip end of previous packet, before retrieving synchronization.
            data += pf;
            size -= pf;
        }
    }

    // Accumulate packet data and process T2-MI packets.
    if (pc.sync) {
        pc.t2mi.append(data, size);
        processT2MI(pid, pc);
    }
}


//----------------------------------------------------------------------------
// Process and remove complete T2-MI packets from the buffer.
//----------------------------------------------------------------------------

void ts::T2MIDemux::processT2MI(PID pid, PIDContext& pc)
{
    // Start index in buffer of T2-MI packet header.
    size_t start = 0;

    // Loop on all complete T2-MI packets.
    while (start + T2MI_HEADER_SIZE < pc.t2mi.size()) {

        // Extract T2-MI packet size in bytes.
        const uint16_t payload_bits = GetUInt16(&pc.t2mi[start + 4]);
        const uint16_t payload_bytes = (payload_bits + 7) / 8;
        const size_t packet_size = T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE;

        if (start + packet_size > pc.t2mi.size()) {
            // Current T2-MI packet not completely present in buffer, stop here.
            break;
        }

        // Build a T2-MI packet.
        T2MIPacket pkt(pc.t2mi.data() + start, packet_size, pid);

        // Notify the application.
        if (pkt.isValid() && _handler != 0) {
            beforeCallingHandler(pid);
            try {
                _handler->handleT2MIPacket(*this, pkt);
            }
            catch (...) {
                afterCallingHandler(false);
                throw;
            }
            afterCallingHandler(true);
        }

        // Point to next T2-MI packet.
        start += T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE;
    }

    // Remove processed T2-MI packets.
    pc.t2mi.erase(0, start);
}


//----------------------------------------------------------------------------
// Invoked by the PSI demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::T2MIDemux::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                // Add all PMT PID's to PSI demux.
                for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _psi_demux.addPID(it->second);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::T2MIDemux::processPMT(const PMT& pmt)
{
    // Loop on all components of the service, until the T2-MI PID is found.
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
         // Search a T2MI_descriptor in this component.
        // Loop on all extension_descriptors.
        const PID pid = it->first;
        const DescriptorList& dlist(it->second.descs);
        for (size_t index = dlist.search(DID_EXTENSION); index < dlist.count(); index = dlist.search(DID_EXTENSION, index + 1)) {
            if (!dlist[index].isNull()) {
                const T2MIDescriptor desc(*dlist[index]);
                if (desc.isValid() && _handler != 0) {
                    // Invoke the user-defined handler to signal the new PID.
                    beforeCallingHandler(pid);
                    try {
                        _handler->handleT2MINewPID(*this, pid, desc);
                    }
                    catch (...) {
                        afterCallingHandler(false);
                        throw;
                    }
                    afterCallingHandler(true);
                }
            }
        }
    }
}
