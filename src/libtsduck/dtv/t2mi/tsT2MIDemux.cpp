//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsT2MIDemux.h"
#include "tsT2MIPacket.h"
#include "tsT2MIDescriptor.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsMemory.h"

namespace {
    // Values of the MODE field in a DVB-T2 BBHEADER, see ETSI EN 302 755, section 5.1.7.
    constexpr uint8_t T2_MODE_NM  = 0;  // Normal Mode
    constexpr uint8_t T2_MODE_HEM = 1;  // High Efficiency Mode

    // CRC-8 of the first 9 bytes of a DVB-T2 BBHEADER, see ETSI EN 302 755, annex F.
    // Generator polynomial X^8+X^7+X^6+X^4+X^2+1, register initialized to zero.
    uint8_t BBHeaderCRC8(const uint8_t* bbh)
    {
        uint8_t crc = 0;
        for (size_t i = 0; i < ts::T2_BBHEADER_SIZE - 1; ++i) {
            crc ^= bbh[i];
            for (int bit = 0; bit < 8; ++bit) {
                crc = (crc & 0x80) != 0 ? uint8_t((crc << 1) ^ 0xD5) : uint8_t(crc << 1);
            }
        }
        return crc;
    }
}


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::T2MIDemux::PLPContext::PLPContext() :
    sync(false),
    up()
{
}

ts::T2MIDemux::PIDContext::PIDContext() :
    continuity(0),
    sync(false),
    t2mi(),
    plps()
{
}

ts::T2MIDemux::T2MIDemux(DuckContext& duck, T2MIHandlerInterface* t2mi_handler, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter),
    _handler(t2mi_handler),
    _pids(),
    _psi_demux(duck, this)
{
    // No virtual dispatch in constructor, use explicit dispatch.
    T2MIDemux::immediateReset();
}

ts::T2MIDemux::~T2MIDemux()
{
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built PES packets).
//----------------------------------------------------------------------------

void ts::T2MIDemux::immediateReset()
{
    SuperClass::immediateReset();
    _pids.clear();

    // Reset the PSI demux since the transport may be completely different.
    _psi_demux.reset();

    // To get PID's with T2-MI, we need to analyze the PMT's.
    // To get the PMT PID's, we need to analyze the PAT.
    _psi_demux.addPID(PID_PAT);
}

void ts::T2MIDemux::immediateResetPID(PID pid)
{
    SuperClass::immediateResetPID(pid);
    _pids.erase(pid);
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::T2MIDemux::feedPacket(const TSPacket& pkt)
{
    const PID pid = pkt.getPID();

    // Super class processing first.
    SuperClass::feedPacket(pkt);

    // Submit the packet to the PSI handler to detect T2-MI streams.
    _psi_demux.feedPacket(pkt);

    // If the packet is not filtered or scrambled, nothing more to do.
    if (!isFilteredPID(pid) || pkt.isScrambled()) {
        return;
    }

    // Get / create PID context.
    PIDContextPtr& pc(_pids[pid]);
    if (pc == nullptr) {
        pc = std::make_shared<PIDContext>();
    }

    // Ignore packets without a payload (their CC should not be incremented, no need to check the synchronization).
    if (!pkt.hasPayload()) {
        return;
    }

    // Drop duplicate packet in outer transport stream.
    if (pc->sync && pkt.getCC() == pc->continuity) {
        return;
    }

    // Check if we loose synchronization.
    if (pc->sync && (pkt.getDiscontinuityIndicator() || pkt.getCC() != ((pc->continuity + 1) & CC_MASK))) {
        pc->lostSync();
    }

    // Keep track of continuity counters.
    pc->continuity = pkt.getCC();

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
            pc->lostSync();
            return;
        }

        // Remove pointer field from packet payload.
        data++;
        size--;

        // If we were previously desynchronized, we are back on track.
        if (!pc->sync) {
            pc->sync = true;
            // Skip end of previous packet, before retrieving synchronization.
            data += pf;
            size -= pf;
        }
    }

    // Accumulate packet data and process T2-MI packets.
    if (pc->sync) {
        pc->t2mi.append(data, size);
        processT2MI(pid, *pc);
    }
}


//----------------------------------------------------------------------------
// Reset PID context after lost of synchronization.
//----------------------------------------------------------------------------

void ts::T2MIDemux::PIDContext::lostSync()
{
    t2mi.clear();   // accumulated T2-MI packet buffer.
    plps.clear();   // we also lose partially demuxed PLP's.
    sync = false;
}

void ts::T2MIDemux::PLPContext::lostSync()
{
    up.clear();     // partial user packet.
    sync = false;
}


//----------------------------------------------------------------------------
// Process and remove complete T2-MI packets from the buffer.
//----------------------------------------------------------------------------

void ts::T2MIDemux::processT2MI(PID pid, PIDContext& pc)
{
    // Start index in buffer of T2-MI packet header.
    size_t start = 0;

    // Protect sequence which may call application-defined handlers.
    beforeCallingHandler(pid);
    try {

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
            if (pkt.isValid()) {

                // Notify the application.
                if (_handler != nullptr) {
                    _handler->handleT2MIPacket(*this, pkt);
                }

                // Demux TS packets from the T2-MI packet.
                demuxTS(pid, pc, pkt);
            }

            // Point to next T2-MI packet.
            start += packet_size;
        }

        // Remove processed T2-MI packets.
        pc.t2mi.erase(0, start);
    }
    catch (...) {
        afterCallingHandler(false);
        throw;
    }
    afterCallingHandler(true);
}


//----------------------------------------------------------------------------
// Demux all encapsulated TS packets from a T2-MI packet.
//----------------------------------------------------------------------------

void ts::T2MIDemux::demuxTS(PID pid, PIDContext& pc, const T2MIPacket& pkt)
{
    // Keep only baseband frames.
    const uint8_t* data = pkt.basebandFrame();
    size_t size = pkt.basebandFrameSize();

    if (data == nullptr || size < T2_BBHEADER_SIZE) {
        // Not a base band frame packet.
        return;
    }

    // Structure of T2-MI packet: see ETSI TS 102 773, section 5.
    // Structure of a T2 baseband frame: see ETSI EN 302 755, section 5.1.7.

    // Extract the TS/GS field of the MATYPE in the BBHEADER.
    // Values: 00 = GFPS, 01 = GCS, 10 = GSE, 11 = TS
    // We only support TS encapsulation here.
    const uint8_t tsgs = (data[0] >> 6) & 0x03;
    if (tsgs != 3) {
        // Not TS mode, cannot extract TS packets.
        return;
    }

    // Get / create PLP context.
    const uint8_t plp = pkt.plp();
    PLPContextPtr& plpp(pc.plps[plp]);
    if (plpp == nullptr) {
        plpp = std::make_shared<PLPContext>();
    }

    // Layout of a transmitted user packet (UP), see ETSI EN 302 755, section 5.1.8.
    // Normal Mode: CRC-8 of the previous UP (in place of the sync byte), 187 bytes, optional ISSY, optional DNP.
    // The size of the UP is given by UPL, including CRC-8, ISSY and DNP.
    // High Efficiency Mode: 187 bytes (sync byte removed), optional DNP. The ISSY, if any, is in the BBHEADER.
    // The MODE is XOR'ed with the CRC-8 of the BBHEADER, any other value means a corrupted BBHEADER.
    const size_t npd = (data[0] & 0x04) ? 1 : 0;  // Size of DNP field (null packet deletion).
    const uint8_t mode = data[9] ^ BBHeaderCRC8(data);
    size_t up_size = 0;  // Size in bytes of a transmitted UP.
    size_t up_crc = 0;   // Size of the CRC-8 field before the TS packet without sync byte.
    if (mode == T2_MODE_HEM) {
        up_size = PKT_SIZE - 1 + npd;
    }
    else if (mode == T2_MODE_NM) {
        const size_t upl = GetUInt16(data + 2);
        up_size = upl % 8 == 0 ? upl / 8 : 0;
        up_crc = 1;
    }
    if (up_size < up_crc + PKT_SIZE - 1 + npd) {
        // Unknown mode or invalid UPL, cannot locate TS packets.
        plpp->lostSync();
        return;
    }

    // Data Field Length in bytes.
    size_t dfl = (GetUInt16(data + 4) + 7) / 8;

    // Synchronization distance in bits.
    size_t syncd = GetUInt16(data + 7);

    // Now skip baseband header.
    data += T2_BBHEADER_SIZE;
    size -= T2_BBHEADER_SIZE;

    // Adjust invalid DFL (should not happen).
    if (dfl > size) {
        dfl = size;
    }

    if (syncd == 0xFFFF) {
        // No user packet starts in data field, the current one continues.
        if (plpp->sync) {
            plpp->up.append(data, dfl);
            if (plpp->up.size() > up_size) {
                plpp->lostSync();
            }
        }
    }
    else {
        // Synchronization distance in bytes, bounded by data field size.
        syncd = std::min(syncd / 8, dfl);

        // Process end of previous packet. Drop it when its size is wrong (lost or inconsistent baseband frames).
        if (plpp->sync) {
            plpp->up.append(data, syncd);
            if (plpp->up.size() == up_size) {
                outputTS(pkt, plpp->up.data() + up_crc, npd ? plpp->up[up_size - 1] : 0);
            }
        }
        plpp->sync = true;
        data += syncd;
        dfl -= syncd;

        // Process subsequent complete packets.
        while (dfl >= up_size) {
            outputTS(pkt, data + up_crc, npd ? data[up_size - 1] : 0);
            data += up_size;
            dfl -= up_size;
        }

        // Keep optional trailing truncated packet.
        plpp->up.copy(data, dfl);
    }
}


//----------------------------------------------------------------------------
// Notify a TS packet from a user packet (data points after the sync byte),
// after the null packets which were deleted before it.
//----------------------------------------------------------------------------

void ts::T2MIDemux::outputTS(const T2MIPacket& pkt, const uint8_t* data, size_t deleted_nulls)
{
    // Notify the application. Note that we are already in a protected section.
    if (_handler != nullptr) {
        for (size_t i = 0; i < deleted_nulls; ++i) {
            _handler->handleTSPacket(*this, pkt, NullPacket);
        }
        TSPacket tsPkt;
        tsPkt.b[0] = SYNC_BYTE;
        MemCopy(tsPkt.b + 1, data, PKT_SIZE - 1);
        _handler->handleTSPacket(*this, pkt, tsPkt);
    }
}


//----------------------------------------------------------------------------
// Invoked by the PSI demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::T2MIDemux::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(duck(), table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                // Add all PMT PID's to PSI demux.
                for (const auto& it : pat.pmts) {
                    _psi_demux.addPID(it.second);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(duck(), table);
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
    for (const auto& it : pmt.streams) {
         // Search a T2MI_descriptor in this component.
        // Loop on all extension_descriptors.
        const PID pid = it.first;
        const DescriptorList& dlist(it.second.descs);
        for (size_t index = dlist.search(DID_DVB_EXTENSION); index < dlist.count(); index = dlist.search(DID_DVB_EXTENSION, index + 1)) {
            const T2MIDescriptor desc(duck(), dlist[index]);
            if (desc.isValid() && _handler != nullptr) {
                // Invoke the user-defined handler to signal the new PID.
                beforeCallingHandler(pid);
                try {
                    _handler->handleT2MINewPID(*this, pmt, pid, desc);
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
