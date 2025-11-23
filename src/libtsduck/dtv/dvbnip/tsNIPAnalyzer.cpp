//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzer.h"
#include "tsNIP.h"


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
    // Log the content of the FDT.
    if (_args.log_fdt) {
        UString line;
        line.format(u"FDT instance: %d, %s, %d files, expires: %s", fdt.instanceId(), fdt.sessionId(), fdt.files.size(), fdt.expires);
        for (const auto& f : fdt.files) {
            line.format(u"\n    TOI: %d, name: %s, %'d bytes, type: %s", f.toi, f.content_location, f.content_length, f.content_type);
        }
        _report.info(line);
    }

    // Save the content of the FDT.
    saveXML(fdt, _args.save_fdt, fdt.instanceId());
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFile(FluteDemux& demux, const FluteFile& file)
{
    const UString name(file.name());
    const bool is_xml = file.type().contains(u"xml");

    // Log a description of the file when requested.
    if (_args.log_files || (is_xml && _args.dump_xml_files)) {
        UString line;
        line.format(u"received file \"%s\" (%'d bytes)\n    type: %s\n    %s, TOI: %d", name, file.size(), file.type(), file.sessionId(), file.toi());

        // Dump XML content when requested.
        if (is_xml && _args.dump_xml_files) {
            line += u"\n    XML content:\n";
            line += file.toXML();
        }
        _report.info(line);
    }

    // Save the content of various files.
    if (name.similar(u"urn:dvb:metadata:nativeip:NetworkInformationFile")) {
        saveXML(file, _args.save_nif);
    }
    else if (name.similar(u"urn:dvb:metadata:nativeip:ServiceInformationFile")) {
        saveXML(file, _args.save_sif);
    }
    else if (name.similar(u"urn:dvb:metadata:nativeip:dvb-i-slep")) {
        saveXML(file, _args.save_slep);
    }
    else if (name.similar(u"urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap")) {
        saveXML(file, _args.save_bootstrap);
    }
}


//----------------------------------------------------------------------------
// Save a XML file (if the file name is not empty).
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::saveXML(const FluteFile& file, const fs::path& path, const std::optional<uint32_t> instance)
{
    // Don't save the file if the path is empty.
    if (!path.empty()) {
        // Build the path.
        fs::path actual_path(path);
        if (path != u"-" && instance.has_value()) {
            actual_path.replace_extension();
            actual_path += UString::Format(u"-%d", instance.value());
            actual_path += path.extension();
        }

        // Save the file.
        if (!file.toXML().save(actual_path, false, true)) {
            _report.error(u"error creating file %s", actual_path);
        }
    }
}
