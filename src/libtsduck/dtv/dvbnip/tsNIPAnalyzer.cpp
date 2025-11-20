//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzer.h"
#include "tsNIP.h"
#include "tsxmlDocument.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::NIPAnalyzer::NIPAnalyzer(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Reset the analysis.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::reset(const NIPAnalyzerArgs& args)
{
    _args = args;
    _flute_demux.reset();
    _flute_demux.setPacketLogLevel(_args.log_flute_packets ? Severity::Info : Severity::Debug);
    _flute_demux.logPacketContent(_args.dump_flute_payload);
}


//----------------------------------------------------------------------------
// The following method feeds the demux with an IP packet.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::feedPacket(const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}


//----------------------------------------------------------------------------
// The following method feeds the demux with a UDP packet.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Experimental code.
    if (destination == NIPSignallingAddress4() || destination.sameMulticast6(NIPSignallingAddress6())) {
        _flute_demux.feedPacket(source, destination, udp, udp_size);
    }
}


//----------------------------------------------------------------------------
// Process a FLUTE File Delivery Table (FDT).
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFDT(FluteDemux& demux, const FluteFDT& fdt)
{
    if (_args.log_fdt) {
        UString line;
        line.format(u"FDT instance: %d, TSI: %d, source: %s, destination: %s, %d files, expires: %s",
                    fdt.instanceId(), fdt.tsi(), fdt.source(), fdt.destination(), fdt.files.size(), fdt.expires);
        for (const auto& f : fdt.files) {
            line.format(u"\n    TOI: %d, name: %s, %'d bytes, type: %s", f.toi, f.content_location, f.content_length, f.content_type);
        }
        _report.info(line);
    }
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFile(FluteDemux& demux, const FluteFile& file)
{
    // Experimental code.

    UString line;
    line.format(u"received file \"%s\", %'d bytes, type: %s, TOI: %d, TSI: %d, source: %s, destination: %s",
                file.name(), file.size(), file.type(), file.toi(), file.tsi(), file.source(), file.destination());
    if (file.type().contains(u"xml")) {
        // Parse and reformat to get an indented XML text.
        xml::Document doc(_report);
        if (doc.parse(file.toText())) {
            line += u'\n';
            line += doc.toString();
            line.trim(false, true);
        }
    }
    _report.info(line);
}
