//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsT2MIDemux.h"
#include "tsT2MIPacket.h"
#include "tsT2MIDescriptor.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::T2MIDemux::PLPContext::PLPContext() :
    first_packet(true),
    ts(),
    ts_next(0)
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
    if (!_pid_filter[pid] || pkt.isScrambled()) {
        return;
    }

    // Get / create PID context.
    PIDContextPtr& pc(_pids[pid]);
    if (pc.isNull()) {
        pc = new PIDContext;
        CheckNonNull(pc.pointer());
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

    // Null packet deletion (NPD) from MATYPE.
    // WARNING: usage of NPD is probably wrong here, need to be checked on streams with NPD=1.
    size_t npd = (data[0] & 0x04) ? 1 : 0;

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

    // Get / create PLP context.
    const uint8_t plp = pkt.plp();
    PLPContextPtr& plpp(pc.plps[plp]);
    if (plpp.isNull()) {
        plpp = new PLPContext;
        CheckNonNull(plpp.pointer());
    }

    if (syncd == 0xFFFF) {
        // No user packet in data field
        plpp->ts.append(data, dfl);
    }
    else {
        // Synchronization distance in bytes, bounded by data field size.
        syncd = std::min(syncd / 8, dfl);

        // Process end of previous packet.
        if (!plpp->first_packet && syncd > 0) {
            if (plpp->ts.size() % PKT_SIZE == 0) {
                plpp->ts.append(SYNC_BYTE);
            }
            plpp->ts.append(data, syncd - npd);
        }
        plpp->first_packet = false;
        data += syncd;
        dfl -= syncd;

        // Process subsequent complete packets.
        while (dfl >= PKT_SIZE - 1) {
            plpp->ts.append(SYNC_BYTE);
            plpp->ts.append(data, PKT_SIZE - 1);
            data += PKT_SIZE - 1;
            dfl -= PKT_SIZE - 1;
        }

        // Process optional trailing truncated packet.
        if (dfl > 0) {
            plpp->ts.append(SYNC_BYTE);
            plpp->ts.append(data, dfl);
        }
    }

    // Now process each complete TS packet.
    while (plpp->ts_next + PKT_SIZE <= plpp->ts.size()) {

        // Build the TS packet.
        TSPacket tsPkt;
        std::memcpy(tsPkt.b, &plpp->ts[plpp->ts_next], PKT_SIZE);
        plpp->ts_next += PKT_SIZE;

        // Notify the application. Note that we are already in a protected section.
        if (_handler != nullptr) {
            _handler->handleTSPacket(*this, pkt, tsPkt);
        }
    }

    // Compress or cleanup the TS buffer.
    if (plpp->ts_next >= plpp->ts.size()) {
        // No more packet to output, cleanup.
        plpp->ts.clear();
        plpp->ts_next = 0;
    }
    else if (plpp->ts_next >= 100 * PKT_SIZE) {
        // TS buffer has many unused packets, compress it.
        plpp->ts.erase(0, plpp->ts_next);
        plpp->ts_next = 0;
    }
}


//----------------------------------------------------------------------------
// Invoked by the PSI demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::T2MIDemux::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(_duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                // Add all PMT PID's to PSI demux.
                for (const auto& it : pat.pmts) {
                    _psi_demux.addPID(it.second);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(_duck, table);
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
            if (!dlist[index].isNull()) {
                const T2MIDescriptor desc(_duck, *dlist[index]);
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
}
