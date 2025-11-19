//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class extract files from FLUTE streams in UDP datagrams.
//!
//----------------------------------------------------------------------------

#include "tsFluteDemux.h"
#include "tsFluteFDT.h"
#include "tsFlute.h"
#include "tsIPSocketAddress.h"
#include "tsIPPacket.h"
#include "tsLCTHeader.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::FluteDemux::FluteDemux(DuckContext& duck, FluteHandlerInterface* handler) :
    _duck(duck),
    _handler(handler)
{
}

ts::FluteDemux::~FluteDemux()
{
}


//----------------------------------------------------------------------------
// Reset the demux.
//----------------------------------------------------------------------------

void ts::FluteDemux::reset()
{
    _tsi_filter.clear();
    _sessions.clear();
}


//----------------------------------------------------------------------------
// SessionId: Identification of a session.
//----------------------------------------------------------------------------

bool ts::FluteDemux::SessionId::operator<(const SessionId& other) const
{
    if (tsi != other.tsi) {
        return tsi < other.tsi;
    }
    else if (source != other.source) {
        return source < other.source;
    }
    else {
        return destination < other.destination;
    }
}


//----------------------------------------------------------------------------
// FileContext: Description of a file being received.
//----------------------------------------------------------------------------

void ts::FluteDemux::FileContext::clear()
{
    processed = false;
    instance = 0xFFFFFFFF;
    transfer_length = current_length = 0;
    chunks.clear();
};


//----------------------------------------------------------------------------
// The following method feeds the demux with an IP packet.
//----------------------------------------------------------------------------

void ts::FluteDemux::feedPacket(const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}


//----------------------------------------------------------------------------
// The following method feeds the demux with a UDP packet.
//----------------------------------------------------------------------------

void ts::FluteDemux::feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Get LCT header.
    LCTHeader lct;
    if (!lct.deserialize(udp, udp_size)) {
        _report.error(u"invalid LCT header from %s", source);
        return;
    }

    // Filter sessions.
    if (!_tsi_filter.empty() && !_tsi_filter.contains(lct.tsi)) {
        _report.debug(u"ignore unfiltered TSI %n", lct.tsi);
        return;
    }

    // The FEC Encoding ID is stored in codepoint (RFC 3926, section 5.1).
    // We currently only support the default one, value 0.
    if (lct.codepoint != FEI_COMPACT_NOCODE) {
        _report.error(u"unsupported FEC Encoding ID %d from %s", lct.codepoint, source);
        return;
    }

    // Log message for the packet.
    if (_packet_log_level <= _report.maxSeverity()) {
        UString line;
        line.format(u"source: %s, destination: %s\n"
                    u"    %s\n"
                    u"    payload: %d bytes",
                    source, destination, lct, udp_size);
        if (_log_packet_content && udp_size > 0) {
            line += u'\n';
            line.appendDump(udp, udp_size, UString::ASCII | UString::HEXA | UString::BPL, 4, 16);
            line.trim(false, true);
        }
        _report.log(_packet_log_level, line);
    }

    // With empty payload, nothing more to do.
    if (udp_size == 0) {
        return;
    }

    // Get/create transport session and file.
    const SessionId sid{source, destination, lct.tsi};
    SessionContext& session(_sessions[sid]);
    FileContext& file(session.files[lct.toi]);

    // If the file is the FDT of the session, it must have FDT and FTI headers.
    if (lct.toi == FLUTE_FDT_TOI) {
        if (!lct.fdt.valid) {
            _report.error(u"FDT in FLUTE packet without EXT_FDT header, TSI %n from %s", lct.tsi, source);
            return;
        }
        if (!lct.fti.valid) {
            _report.error(u"FDT in FLUTE packet without EXT_FTI header, TSI %n from %s", lct.tsi, source);
            return;
        }
        if (file.instance != lct.fdt.fdt_instance_id) {
            _report.debug(u"new FDT instance %n, TSI %n from %s", lct.fdt.fdt_instance_id, lct.tsi, source);
            file.clear();
            file.instance = lct.fdt.fdt_instance_id;
        }
    }

    // If the file was already processed, nothing more to do.
    if (file.processed) {
        return;
    }

    // Update/check transfer length coming from FTI header.
    if (lct.fti.valid) {
        if (file.transfer_length > 0 && file.transfer_length != lct.fti.transfer_length) {
            _report.error(u"file transfer length changed in the middle of transmission, was %'d, now %'d, TOI %d, TSI %d from %s",
                          file.transfer_length, lct.fti.transfer_length, lct.toi, lct.tsi, source);
        }
        file.transfer_length = lct.fti.transfer_length;
    }

    // Check the FEC payload ID. We don't currently support non-zero SBN (to be studied).
    if (!lct.fpi.valid) {
        _report.error(u"FEC payload ID not found in FLUTE packet, TSI %n from %s", lct.tsi, source);
        return;
    }
    if (lct.fpi.source_block_number > 0) {
        _report.error(u"Source block number is %n in FEC payload ID, not supported yet, TOI %d, TSI %d from %s", lct.fpi.source_block_number, lct.toi, lct.tsi, source);
        return;
    }

    // Store the file chunk if not already there.
    const size_t chunk_index = size_t(lct.fpi.encoding_symbol_id);
    if (chunk_index >= file.chunks.size()) {
        file.chunks.resize(chunk_index + 1);
    }
    if (file.chunks[chunk_index] != nullptr) {
        // Already got that chunk.
        if (udp_size != file.chunks[chunk_index]->size()) {
            _report.error(u"size of file chunk #%n changed in the middle of transmission, was %'d, now %'d, TOI %d, TSI %d from %s",
                          chunk_index, file.chunks[chunk_index]->size(), udp_size, lct.toi, lct.tsi, source);
        }
        return;
    }
    file.chunks[chunk_index] = std::make_shared<ByteBlock>(udp, udp_size);
    file.current_length += udp_size;

    // If file is not complete (or its size is unknown), nothing more to do.
    if (file.transfer_length == 0 || file.current_length < file.transfer_length) {
        return;
    }

    // Rebuild the content of the file.
    ByteBlockPtr data(std::make_shared<ByteBlock>(file.transfer_length));
    size_t next_index = 0;
    for (const auto& bb : file.chunks) {
        if (bb != nullptr) {
            if (next_index + bb->size() > data->size()) {
                // Should not happen, but let's be conservative.
                _report.debug(u"need to increase size of file buffer, was %'d, now %'d, TOI %d, TSI %d from %s",
                              data->size(), next_index + bb->size(), lct.toi, lct.tsi, source);
                data->resize(next_index + bb->size());
            }
            MemCopy(data->data() + next_index, bb->data(), bb->size());
            next_index += bb->size();
        }
    }
    // Important: we currently support FEC Encoding ID zero, meaning no encoding,
    // therefore the raw transport data are identical to the file content.

    if (lct.toi == FLUTE_FDT_TOI) {
        // Process a new FDT.
        const FluteFDT fdt(_report, sid.source, sid.destination, sid.tsi, file.instance, data);
        if (fdt.isValid()) {
            processFDT(fdt, session);
            if (_handler != nullptr) {
                _handler->handleFluteFDT(*this, fdt);
            }
        }
    }
    else if (_handler != nullptr) {
        // Process a normal file.
        const FluteFile ff(sid.source, sid.destination, sid.tsi, lct.toi, file.name, data);
        _handler->handleFluteFile(*this, ff);
    }

    // Now forget about this file.
    file.processed = true;
    file.chunks.clear();
}


//----------------------------------------------------------------------------
// Process a new FDT in a session.
//----------------------------------------------------------------------------

void ts::FluteDemux::processFDT(const FluteFDT& fdt, SessionContext& session)
{
    //@@@
}
