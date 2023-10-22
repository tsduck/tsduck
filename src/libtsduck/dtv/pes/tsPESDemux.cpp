//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPESDemux.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsMemory.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsPSI.h"
#include "tsPES.h"
#include "tsAccessUnitIterator.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESDemux::PESDemux(DuckContext& duck, PESHandlerInterface* pes_handler, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter),
    _pes_handler(pes_handler),
    _section_demux(_duck, this)
{
    // Analyze the PAT, to get the PMT's, to get the stream types.
    _section_demux.addPID(PID_PAT);
}

ts::PESDemux::~PESDemux()
{
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built PES packets).
//----------------------------------------------------------------------------

void ts::PESDemux::immediateReset()
{
    SuperClass::immediateReset();
    _pids.clear();
    _pid_types.clear();

    // Reset the section demux back to initial state (intercepting the PAT).
    _section_demux.reset();
    _section_demux.addPID(PID_PAT);
}

void ts::PESDemux::immediateResetPID(PID pid)
{
    SuperClass::immediateResetPID(pid);
    _pids.erase(pid);
    _pid_types.erase(pid);
}


//----------------------------------------------------------------------------
// Set/get the default audio or video codec for one specific PES PID's.
//----------------------------------------------------------------------------

void ts::PESDemux::setDefaultCodec(PID pid, CodecType codec)
{
    _pid_types[pid].default_codec = codec;
}

ts::CodecType ts::PESDemux::getDefaultCodec(PID pid) const
{
    const auto it = _pid_types.find(pid);
    return it == _pid_types.end() || it->second.default_codec == CodecType::UNDEFINED ? _default_codec : it->second.default_codec;
}


//----------------------------------------------------------------------------
// Get current audio/video attributes on the specified PID.
// Check isValid() on returned object.
//----------------------------------------------------------------------------

void ts::PESDemux::getAudioAttributes(PID pid, MPEG2AudioAttributes& va) const
{
    const auto pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.audio.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.audio;
    }
}

void ts::PESDemux::getVideoAttributes(PID pid, MPEG2VideoAttributes& va) const
{
    const auto pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.video.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.video;
    }
}

void ts::PESDemux::getAVCAttributes(PID pid, AVCAttributes& va) const
{
    const auto pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.avc.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.avc;
    }
}

void ts::PESDemux::getHEVCAttributes(PID pid, HEVCAttributes& va) const
{
    const auto pci = _pids.find(pid);
    if (pci == _pids.end() || !pci->second.hevc.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.hevc;
    }
}

void ts::PESDemux::getAC3Attributes(PID pid, AC3Attributes& va) const
{
    const auto pci = _pids.find (pid);
    if (pci == _pids.end() || !pci->second.ac3.isValid()) {
        va.invalidate();
    }
    else {
        va = pci->second.ac3;
    }
}

bool ts::PESDemux::allAC3(PID pid) const
{
    const auto pci = _pids.find(pid);
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
    auto pci = _pids.find(pid);
    bool pc_exists = pci != _pids.end();

    // If no context established and not at a unit start, ignore packet
    if (!pc_exists && !pkt.getPUSI()) {
        return;
    }

    // If at a unit start and the context exists, process previous PES packet in context
    if (pc_exists && pkt.getPUSI() && pci->second.sync && !pci->second.ts.isNull() && !pci->second.ts->empty()) {
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
            pc.pcr = pkt.getPCR(); // can be invalid

            // Check if the complete PES packet is now present (without waiting for the next PUSI).
            processPESPacketIfComplete(pid, pc);
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

    // Keep track of first PCR in the PES packet.
    if (pc.pcr == INVALID_PCR && pkt.hasPCR()) {
        pc.pcr = pkt.getPCR();
    }

    // Check if the complete PES packet is now present (without waiting for the next PUSI).
    processPESPacketIfComplete(pid, pc);
}


//-----------------------------------------------------------------------------
// Flush any unterminated unbounded PES packet on the specified PID.
//-----------------------------------------------------------------------------

void ts::PESDemux::flushUnboundedPES(PID pid)
{
    const auto pci = _pids.find(pid);
    if (pci != _pids.end() && pci->second.sync && !pci->second.ts.isNull() && !pci->second.ts->empty()) {
        processPESPacket(pid, pci->second);
    }
}


//-----------------------------------------------------------------------------
//! Flush any unterminated unbounded PES packet on all PID's.
//-----------------------------------------------------------------------------

void ts::PESDemux::flushUnboundedPES()
{
    // Get the list of PID's first, then search each of them one by one.
    // Because a handler can modify the list, we cannot call processPESPacket() while walkig through the map.
    const std::set<PID> pids(MapKeysSet(_pids));
    for (auto pid : pids) {
        flushUnboundedPES(pid);
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
                for (const auto& it : pat.pmts) {
                    _section_demux.addPID(it.second);
                }
            }
            break;
        }
        case TID_PMT: {
            // Got a PMT, collect all stream types.
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                for (const auto& it : pmt.streams) {
                    _pid_types[it.first].stream_type = it.second.stream_type;
                    _pid_types[it.first].default_codec = it.second.getCodec(_duck);
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


//----------------------------------------------------------------------------
// If a PID context contains a complete PES packet, process it.
//----------------------------------------------------------------------------

void ts::PESDemux::processPESPacketIfComplete(PID pid, PIDContext& pc)
{
    if (pc.ts->size() >= 6 && pc.sync) {
        // There is enough to get the PES packet length.
        const size_t len = GetUInt16(pc.ts->data() + 4);
        // If the size is zero, the PES packet is "unbounded", meaning it ends at the next PUSI.
        // But if the PES packet size is specified, check if we have the complete PES packet.
        if (len != 0 && pc.ts->size() >= 6 + len) {
            // We have the complete PES packet.
            processPESPacket(pid, pc);
        }
    }
}


//----------------------------------------------------------------------------
// Process a complete PES packet
//----------------------------------------------------------------------------

void ts::PESDemux::processPESPacket(PID pid, PIDContext& pc)
{
    // Note: We need to process even if _pes_handler is null. Subclasses of the
    // PES demux may override handlePESPacket() and have their own processing.

    // Mark that we are in the context of handlers.
    // This is used to prevent the destruction of PID contexts during the execution of a handler.
    beforeCallingHandler(pid);
    try {
        // Build a PES packet object around the TS buffer
        PESPacket pes(pc.ts, pid);

        if (pes.isValid()) {
            // Count valid PES packets
            pc.pes_count++;

            // Location of the PES packet inside the demultiplexed stream
            pes.setFirstTSPacketIndex(pc.first_pkt);
            pes.setLastTSPacketIndex(pc.last_pkt);
            pes.setPCR(pc.pcr);

            // Set stream type and codec if known.
            const auto it_type = _pid_types.find(pid);
            if (it_type != _pid_types.end()) {
                pes.setStreamType(it_type->second.stream_type);
                pes.setCodec(it_type->second.default_codec);
            }

            // Set a default codec if none was set from the PMT and the data look compatible.
            pes.setDefaultCodec(getDefaultCodec(pid));

            // Handle complete packet (virtual method). This must be executed even if _pes_handler is null
            // because handlePESPacket() is virtual and can be overridden in a subclass (cf. TeletextDemux).
            handlePESPacket(pes);

            // Analyze audio/video content of the packet and notify all corresponding events.
            if (_pes_handler != nullptr) {
                handlePESContent(pc, pes);
            }
        }
        else if (_pes_handler != nullptr) {
            // Handle an invalid PES packet. Prepare raw demuxed data.
            DemuxedData data(pc.ts, pid);
            data.setFirstTSPacketIndex(pc.first_pkt);
            data.setLastTSPacketIndex(pc.last_pkt);
            _pes_handler->handleInvalidPESPacket(*this, data);
        }
    }
    catch (...) {
        afterCallingHandler(false);
        throw;
    }
    afterCallingHandler(true);

    // Consider that we lose sync in case there are additional TS packets on that PID before next PUSI.
    pc.syncLost();
}


//-----------------------------------------------------------------------------
// This hook is invoked when a complete PES packet is available.
// This is a protected virtual method.
//-----------------------------------------------------------------------------

void ts::PESDemux::handlePESPacket(const PESPacket& pes)
{
    // Process PES packet
    if (_pes_handler != nullptr) {
        _pes_handler->handlePESPacket(*this, pes);
    }
}


//----------------------------------------------------------------------------
// Process all video/audio analysis on the PES packet.
//----------------------------------------------------------------------------

void ts::PESDemux::handlePESContent(PIDContext& pc, const PESPacket& pes)
{
    // Packet payload content (constants).
    const uint8_t* const pl_data = pes.payload();
    const size_t pl_size = pes.payloadSize();

    // Process intra-coded images.
    const size_t intra_offset = pes.findIntraImage();
    if (intra_offset != NPOS) {
        _pes_handler->handleIntraImage(*this, pes, intra_offset);
    }

    // Iterator on AVC/HEVC/VVC access units.
    AccessUnitIterator au_iter(pl_data, pl_size, pes.getStreamType(), pes.getCodec());

    // Process AVC/HEVC/VVC access units (aka "NALunits")
    if (au_iter.isValid()) {
        const CodecType codec = au_iter.videoFormat();
        // Loop on all access units.
        for (; !au_iter.atEnd(); au_iter.next()) {
            const uint8_t au_type = au_iter.currentAccessUnitType();
            const size_t au_offset = au_iter.currentAccessUnitOffset(); // offset in PES payload
            const size_t au_size = au_iter.currentAccessUnitSize();
            const uint8_t* const au_end = pl_data + au_offset + au_size;
            assert(au_end <= pl_data + pl_size);

            // Invoke handler for the complete NALunit.
            _pes_handler->handleAccessUnit(*this, pes, au_type, au_offset, au_size);

            // If the NALunit is an SEI, process all SEI messages.
            if (au_iter.currentAccessUnitIsSEI()) {
                // See H.264 (7.3.2.3.1), H.265 (7.3.5), H.266 (7.3.6).
                const uint8_t* p = pl_data + au_offset + au_iter.currentAccessUnitHeaderSize();
                // Loop on all SEI messages in the NALunit.
                while (p < au_end) {
                    // Compute SEI payload type.
                    uint32_t sei_type = 0;
                    while (p < au_end && *p == 0xFF) {
                        sei_type += *p++;
                    }
                    if (p < au_end) {
                        sei_type += *p++;
                    }
                    // Compute SEI payload size.
                    size_t sei_size = 0;
                    while (p < au_end && *p == 0xFF) {
                        sei_size += *p++;
                    }
                    if (p < au_end) {
                        sei_size += *p++;
                    }
                    sei_size = std::min<size_t>(sei_size, au_end - p);
                    // Invoke handler for the SEI.
                    if (sei_size > 0) {
                        _pes_handler->handleSEI(*this, pes, sei_type, p - pl_data, sei_size);
                    }
                    p += sei_size;
                }
            }

            // Accumulate info from access units to extract video attributes.
            // If new attributes were found, invoke handler.
            if (codec == CodecType::AVC && pc.avc.moreBinaryData(pl_data + au_offset, au_size)) {
                _pes_handler->handleNewAVCAttributes(*this, pes, pc.avc);
            }
            else if (codec == CodecType::HEVC && pc.hevc.moreBinaryData(pl_data + au_offset, au_size)) {
                _pes_handler->handleNewHEVCAttributes(*this, pes, pc.hevc);
            }
        }
    }

    // Process MPEG-1 (ISO 11172-2) and MPEG-2 (ISO 13818-2) video start codes
    else if (pes.isMPEG2Video()) {
        // Locate all start codes and invoke handler.
        // The beginning of the payload is already a start code prefix.
        for (size_t offset = 0; offset < pl_size; ) {
            // Look for next start code
            static const uint8_t StartCodePrefix[] = {0x00, 0x00, 0x01};
            const uint8_t* pnext = LocatePattern(pl_data + offset + 1, pl_size - offset - 1, StartCodePrefix, sizeof(StartCodePrefix));
            size_t next = pnext == nullptr ? pl_size : pnext - pl_data;
            // Invoke handler
            _pes_handler->handleVideoStartCode(*this, pes, pl_data[offset + 3], offset, next - offset);
            // Accumulate info from video units to extract video attributes.
            // If new attributes were found, invoke handler.
            if (pc.video.moreBinaryData(pl_data + offset, next - offset)) {
                _pes_handler->handleNewMPEG2VideoAttributes(*this, pes, pc.video);
            }
            // Move to next start code
            offset = next;
        }
    }

    // Process AC-3 audio frames
    else if (pes.isAC3()) {
        // Count PES packets with potential AC-3 packet.
        pc.ac3_count++;
        // Accumulate info from audio frames to extract audio attributes.
        // If new attributes were found, invoke handler.
        if (pc.ac3.moreBinaryData(pl_data, pl_size)) {
            _pes_handler->handleNewAC3Attributes(*this, pes, pc.ac3);
        }
    }

    // Process other audio frames
    else if (IsAudioSID(pes.getStreamId())) {
        // Accumulate info from audio frames to extract audio attributes.
        // If new attributes were found, invoke handler.
        if (pc.audio.moreBinaryData(pl_data, pl_size)) {
            _pes_handler->handleNewMPEG2AudioAttributes(*this, pes, pc.audio);
        }
    }
}
