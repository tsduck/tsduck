//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//
//  This class extracts PES packets from TS packets
//
//----------------------------------------------------------------------------

#include "tsPESDemux.h"
#include "tsMemory.h"
#include "tsPAT.h"
#include "tsPMT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Delimiters
//----------------------------------------------------------------------------

namespace {

    // Start code prefix for ISO 11172-2 (MPEG-1 video) and ISO 13818-2 (MPEG-2 video)
    const uint8_t StartCodePrefix[] = {0x00, 0x00, 0x01};

    // End of AVC NALunit delimiter
    const uint8_t Zero3[] = {0x00, 0x00, 0x00};
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PESDemux::PESDemux(DuckContext& duck, PESHandlerInterface* pes_handler, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter),
    _pes_handler(pes_handler),
    _pids(),
    _stream_types(),
    _section_demux(_duck, this)
{
    // Analyze the PAT, to get the PMT's, to get the stream types.
    _section_demux.addPID(PID_PAT);
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::PESDemux::~PESDemux()
{
}


//----------------------------------------------------------------------------
// PIDContext constructor
//----------------------------------------------------------------------------

ts::PESDemux::PIDContext::PIDContext() :
    pes_count(0),
    continuity(0),
    sync(false),
    first_pkt(0),
    last_pkt(0),
    ts(new ByteBlock()),
    audio(),
    video(),
    avc(),
    ac3(),
    ac3_count(0)
{
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built PES packets).
//----------------------------------------------------------------------------

void ts::PESDemux::immediateReset()
{
    SuperClass::immediateReset();
    _pids.clear();
    _stream_types.clear();

    // Reset the section demux back to initial state (intercepting the PAT).
    _section_demux.reset();
    _section_demux.addPID(PID_PAT);
}

void ts::PESDemux::immediateResetPID(PID pid)
{
    SuperClass::immediateResetPID(pid);
    _pids.erase(pid);
    _stream_types.erase(pid);
}


//----------------------------------------------------------------------------
// Get current audio/video attributes on the specified PID.
// Check isValid() on returned object.
//----------------------------------------------------------------------------

void ts::PESDemux::getAudioAttributes(PID pid, AudioAttributes& va) const
{
    PIDContextMap::const_iterator pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.audio.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.audio;
    }
}

void ts::PESDemux::getVideoAttributes(PID pid, VideoAttributes& va) const
{
    PIDContextMap::const_iterator pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.video.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.video;
    }
}

void ts::PESDemux::getAVCAttributes(PID pid, AVCAttributes& va) const
{
    PIDContextMap::const_iterator pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.avc.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.avc;
    }
}

void ts::PESDemux::getAC3Attributes(PID pid, AC3Attributes& va) const
{
    PIDContextMap::const_iterator pci = _pids.find (pid);
    if (pci == _pids.end() || !pci->second.ac3.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.ac3;
    }
}

bool ts::PESDemux::allAC3(PID pid) const
{
    PIDContextMap::const_iterator pci = _pids.find(pid);
    return pci != _pids.end() && pci->second.pes_count > 0 && pci->second.ac3_count == pci->second.pes_count;
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::PESDemux::feedPacket(const TSPacket& pkt)
{
    // Feed the section demux to get the PAT and PMT's.
    _section_demux.feedPacket(pkt);

    // Process PES data on filtered PID's.
    if (_pid_filter[pkt.getPID()]) {
        processPacket(pkt);
    }

    // Invoke super class for its own processing.
    SuperClass::feedPacket(pkt);
}

void ts::PESDemux::processPacket(const TSPacket& pkt)
{
    // Reject invalid packets
    if (!pkt.hasValidSync()) {
        return;
    }

    // Get PID and check if context exists
    PID pid = pkt.getPID();
    PIDContextMap::iterator pci = _pids.find (pid);
    bool pc_exists = pci != _pids.end();

    // If no context established and not at a unit start, ignore packet
    if (!pc_exists && !pkt.getPUSI()) {
        return;
    }

    // If at a unit start and the context exists, process previous PES packet in context
    if (pc_exists && pkt.getPUSI() && pci->second.sync) {
        // Process packet, invoke all handlers
        processPESPacket(pid, pci->second);
        // Recheck PID context in case it was reset by a handler
        pci = _pids.find(pid);
        pc_exists = pci != _pids.end();
    }

    // If the packet is scrambled, we cannot get PES content.
    // Usually, if the PID becomes scrambled, it will remain scrambled
    // for a while => release context.
    if (pkt.getScrambling() != SC_CLEAR) {
        if (pc_exists) {
            _pids.erase(pid);
        }
        return;
    }

    // TS packet payload
    size_t pl_size = pkt.getPayloadSize();
    const uint8_t* pl = pkt.getPayload();

    // If the packet contains a unit start
    if (pkt.getPUSI()) {
        // If the beginning of a PUSI payload is 00 00 01, this is a PES packet
        // (it is not possible to have 00 00 01 in a PUSI packet containing sections).
        if (pl_size >= 3 && pl[0] == 0 && pl[1] == 0 && pl[2] == 1) {
            // We are at the beginning of a PES packet. Create context if non existent.
            PIDContext& pc(_pids[pid]);
            pc.continuity = pkt.getCC();
            pc.sync = true;
            pc.ts->copy(pl, pl_size);
            pc.first_pkt = _packet_count;
            pc.last_pkt = _packet_count;
        }
        else if (pc_exists) {
            // This PID does not contain PES packet, reset context
            _pids.erase(pid);
        }
        // PUSI packet processing done.
        return;
    }

    // At this point, the TS packet contains part of a PES packet, but not beginning.
    // Check that PID context is valid.
    if (!pc_exists || !pci->second.sync) {
        return;
    }
    PIDContext& pc(pci->second);

    // Ignore duplicate packets (same CC)
    if (pkt.getCC() == pc.continuity) {
        return;
    }

    // Check if we are still synchronized
    if (pkt.getCC() != (pc.continuity + 1) % CC_MAX) {
        pc.syncLost();
        return;
    }
    pc.continuity = pkt.getCC();

    // Append the TS payload in PID context.
    size_t capacity = pc.ts->capacity();
    if (pc.ts->size() + pl_size > capacity) {
        // Internal reallocation needed in ts buffer.
        // Do not allow implicit reallocation, do it manually for better performance.
        // Use two predefined thresholds: 64 kB and 512 kB. Above that, double the size.
        // Note that 64 kB is OK for audio PIDs. Video PIDs are usually unbounded. The
        // maximum observed PES rate is 2 PES/s, meaning 512 kB / PES at 8 Mb/s.
        if (capacity < 64 * 1024) {
            pc.ts->reserve(64 * 1024);
        }
        else if (capacity < 512 * 1024) {
            pc.ts->reserve(512 * 1024);
        }
        else {
            pc.ts->reserve(2 * capacity);
        }
    }
    pc.ts->append(pl, pl_size);

    // Last TS packet containing actual data for this PES packet
    pc.last_pkt = _packet_count;

    // Check if the complete PES packet is now present (without waiting for the next PUSI).
    if (pc.ts->size() >= 6 && pc.sync) {
        // There is enought to get the PES packet length.
        const size_t len = GetUInt16(pc.ts->data() + 4);
        // If the size is zero, the PES packet is "unbounded", meaning it ends at the next PUSI.
        // But if the PES packet size is specified, check if we have the complete PES packet.
        if (len != 0 && pc.ts->size() >= 4 + len) {
            // We have the complete PES packet.
            processPESPacket(pid, pc);
            // Reset PES buffer.
            pc.ts->clear();
        }
    }
}


//-----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
// Implementation of TableHandlerInterface.
//-----------------------------------------------------------------------------

void ts::PESDemux::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            // Got a PAT, add all PMT PID's to section demux.
            const PAT pat(_duck, table);
            if (pat.isValid()) {
                for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _section_demux.addPID(it->second);
                }
            }
            break;
        }
        case TID_PMT: {
            // Got a PMT, collect all stream types.
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    _stream_types[it->first] = it->second.stream_type;
                }
            }
            break;
        }
        default: {
            // Nothing to do.
            break;
        }
    }
}


//-----------------------------------------------------------------------------
// This hook is invoked when a complete PES packet is available.
//-----------------------------------------------------------------------------

void ts::PESDemux::handlePESPacket(const PESPacket& packet)
{
    if (_pes_handler != nullptr) {
        _pes_handler->handlePESPacket(*this, packet);
    }
}


//----------------------------------------------------------------------------
// Process a complete PES packet
//----------------------------------------------------------------------------

void ts::PESDemux::processPESPacket(PID pid, PIDContext& pc)
{
    // Build a PES packet object around the TS buffer
    PESPacket pp(pc.ts, pid);
    if (!pp.isValid()) {
        return;
    }

    // Count valid PES packets
    pc.pes_count++;

    // Location of the PES packet inside the demultiplexed stream
    pp.setFirstTSPacketIndex(pc.first_pkt);
    pp.setLastTSPacketIndex(pc.last_pkt);

    // Set stream type if known.
    const StreamTypeMap::const_iterator it = _stream_types.find(pid);
    if (it != _stream_types.end()) {
        pp.setStreamType(it->second);
    }

    // Mark that we are in the context of handlers.
    // This is used to prevent the destruction of PID contexts during
    // the execution of a handler.
    beforeCallingHandler(pid);
    try {
        // Handle complete packet
        handlePESPacket(pp);

        // Packet payload content (constants)
        const uint8_t* const pdata = pp.payload();
        const size_t psize = pp.payloadSize();

        // Process MPEG-1 (ISO 11172-2) and MPEG-2 (ISO 13818-2) video start codes
        if (pp.isMPEG2Video()) {
            // Locate all start codes and invoke handler.
            // The beginning of the payload is already a start code prefix.
            for (size_t offset = 0; offset < psize; ) {
                // Look for next start code
                const void* pnext = LocatePattern(pdata + offset + 1, psize - offset - 1, StartCodePrefix, sizeof(StartCodePrefix));
                size_t next = pnext == nullptr ? psize : reinterpret_cast<const uint8_t*>(pnext) - pdata;
                // Invoke handler
                if (_pes_handler != nullptr) {
                    _pes_handler->handleVideoStartCode(*this, pp, pdata[offset + 3], offset, next - offset);
                }
                // Accumulate info from video units to extract video attributes.
                // If new attributes were found, invoke handler.
                if (pc.video.moreBinaryData(pdata + offset, next - offset) && _pes_handler != nullptr) {
                    _pes_handler->handleNewVideoAttributes(*this, pp, pc.video);
                }
                // Move to next start code
                offset = next;
            }
        }

        // Process AVC (ISO 14496-10, ITU H.264) access units (aka "NALunits")
        else if (pp.isAVC()) {
            for (size_t offset = 0; offset < psize; ) {
                // Locate next access unit: starts with 00 00 01 (this start code is not part of the NALunit)
                const uint8_t* p1 = reinterpret_cast<const uint8_t*>(
                            LocatePattern(pdata + offset, psize - offset, StartCodePrefix, sizeof(StartCodePrefix)));
                if (p1 == nullptr) {
                    break;
                }
                offset = p1 - pdata + sizeof(StartCodePrefix);

                // Locate end of access unit: ends with 00 00 00, 00 00 01 or end of data.
                const uint8_t* p2 = reinterpret_cast<const uint8_t*>(
                            LocatePattern(pdata + offset, psize - offset, StartCodePrefix, sizeof(StartCodePrefix)));
                const uint8_t* p3 = reinterpret_cast<const uint8_t*>(
                            LocatePattern(pdata + offset, psize - offset, Zero3, sizeof(Zero3)));
                size_t nalunit_size = 0;
                if (p2 == nullptr && p3 == nullptr) {
                    // No 00 00 01, no 00 00 00, the NALunit extends up to the end of data.
                    nalunit_size = psize - offset;
                }
                else if (p2 == nullptr || (p3 != nullptr && p3 < p2)) {
                    // NALunit ends at 00 00 00.
                    assert(p3 != nullptr);
                    nalunit_size = p3 - pdata - offset;
                }
                else {
                    // NALunit ends at 00 00 01.
                    assert(p2 != nullptr);
                    nalunit_size = p2 - pdata - offset;
                }

                // Compute NALunit type.
                const uint8_t nalunit_type = nalunit_size == 0 ? 0 : (pdata[offset] & 0x1F);
                const uint8_t* nalunit_end = pdata + offset + nalunit_size;

                // Invoke handler for the complete NALunit.
                if (_pes_handler != nullptr) {
                    _pes_handler->handleAVCAccessUnit(*this, pp, nalunit_type, offset, nalunit_size);
                }

                // If the NALunit is an SEI, locate it.
                if (nalunit_type == AVC_AUT_SEI) {
                    // See H.264 section 7.3.2.3.1, "Supplemental enhancement information message syntax".
                    // Point after nalunit type:
                    const uint8_t* p = pdata + offset + 1;

                    while (p < nalunit_end)
                    {
                       // Compute SEI payload type.
                       uint32_t sei_type = 0;
                       while (p < nalunit_end && *p == 0xFF) {
                          sei_type += *p++;
                       }
                       if (p < nalunit_end) {
                          sei_type += *p++;
                       }
                       // Compute SEI payload size.
                       size_t sei_size = 0;
                       while (p < nalunit_end && *p == 0xFF) {
                          sei_size += *p++;
                       }
                       if (p < nalunit_end) {
                          sei_size += *p++;
                       }
                       sei_size = std::min<size_t>(sei_size, nalunit_end - p);
                       // Invoke handler for the SEI.
                       if (_pes_handler != nullptr) {
                          _pes_handler->handleSEI(*this, pp, sei_type, p - pdata, sei_size);
                       }
                       p += sei_size;
                    }
                }

                // Accumulate info from access units to extract video attributes.
                // If new attributes were found, invoke handler.
                if (pc.avc.moreBinaryData(pdata + offset, nalunit_size) && _pes_handler != nullptr) {
                    _pes_handler->handleNewAVCAttributes(*this, pp, pc.avc);
                }

                // Move to next start code
                offset += nalunit_size;
            }
        }

        // Process AC-3 audio frames
        else if (pp.isAC3()) {
            // Count PES packets with potential AC-3 packet.
            pc.ac3_count++;
            // Accumulate info from audio frames to extract audio attributes.
            // If new attributes were found, invoke handler.
            if (pc.ac3.moreBinaryData(pdata, psize) && _pes_handler != nullptr) {
                _pes_handler->handleNewAC3Attributes(*this, pp, pc.ac3);
            }
        }

        // Process other audio frames
        else if (IsAudioSID(pp.getStreamId())) {
            // Accumulate info from audio frames to extract audio attributes.
            // If new attributes were found, invoke handler.
            if (pc.audio.moreBinaryData(pdata, psize) && _pes_handler != nullptr) {
                _pes_handler->handleNewAudioAttributes(*this, pp, pc.audio);
            }
        }
    }
    catch (...) {
        afterCallingHandler(false);
        throw;
    }
    afterCallingHandler(true);
}
