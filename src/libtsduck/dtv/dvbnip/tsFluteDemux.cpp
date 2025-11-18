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
}


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
    // Experimental code.

    // Get LCT header.
    LCTHeader lct;
    if (!lct.deserialize(udp, udp_size)) {
        _report.error(u"invalid LCT header from %s", source);
        return;
    }

    // The FEC Encoding ID is stored in codepoint (RFC 3926, section 5.1).
    // We currently only support the default one, value 0.
    if (lct.codepoint != FEI_COMPACT_NOCODE) {
        _report.error(u"unsupported FEC Encoding ID %d from %s", lct.codepoint, source);
        return;
    }

    // Display debug message on packet format.
    UString line;
    line.format(u"source: %s, destination: %s\n"
                u"    %s\n"
                u"    payload: %d bytes",
                source, destination, lct, udp_size);
    if (udp_size > 0) {
        line += u'\n';
        line.appendDump(udp, udp_size, UString::ASCII | UString::HEXA | UString::BPL, 4, 16);
        line.trim(false, true);
    }
    _report.info(line);
}
