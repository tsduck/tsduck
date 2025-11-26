//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzer.h"
#include "tsNIP.h"
#include "tsMulticastGatewayConfiguration.h"


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
    _flute_demux.reset(_args);

    // Reset the list of sessions to filter with the DVB-NIP announcement channel (IPv4 and IPv6).
    static const FluteSessionId announce4(IPAddress(), NIPSignallingAddress4(), NIP_SIGNALLING_TSI);
    static const FluteSessionId announce6(IPAddress(), NIPSignallingAddress6(), NIP_SIGNALLING_TSI);
    _session_filter.clear();
    addSession(announce4);
    addSession(announce6);
}


//----------------------------------------------------------------------------
// Add a FLUTE session in the DVB-NIP analyzer.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::addSession(const FluteSessionId& session)
{
    _report.debug(u"adding session %s", session);
    _session_filter.insert(session);
}


//----------------------------------------------------------------------------
// Check if a UDP packet or FLUTE file is part of a filtered session.
//----------------------------------------------------------------------------

bool ts::NIPAnalyzer::isFiltered(const IPAddress& source, const IPSocketAddress& destination) const
{
    for (const auto& it : _session_filter) {
        if (it.source.match(source) && it.destination.match(destination)) {
            return true;
        }
    }
    return false;
}

bool ts::NIPAnalyzer::isFiltered(const FluteSessionId& session) const
{
    for (const auto& it : _session_filter) {
        if (it.match(session)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// The following method feeds the demux with an IP or UDP packet.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::feedPacket(const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}

void ts::NIPAnalyzer::feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Feed the FLUTE demux with possibly filtered packets. The TSI is not yet accessible, only the addresses.
    if (isFiltered(source, destination)) {
        _flute_demux.feedPacket(source, destination, udp, udp_size);
    }
}


//----------------------------------------------------------------------------
// Process a FLUTE File Delivery Table (FDT).
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFDT(FluteDemux& demux, const FluteFDT& fdt)
{
    // Filter out files from non-filtered sessions.
    if (!isFiltered(fdt.sessionId())) {
        return;
    }

    // Log the content of the FDT.
    if (_args.log_fdt) {
        UString line;
        line.format(u"FDT instance: %d, %s, %d files, expires: %s", fdt.instance_id, fdt.sessionId(), fdt.files.size(), fdt.expires);
        for (const auto& f : fdt.files) {
            line.format(u"\n    TOI: %d, name: %s, %'d bytes, type: %s", f.toi, f.content_location, f.content_length, f.content_type);
        }
        _report.info(line);
    }

    // Save the content of the FDT.
    saveXML(fdt, _args.save_fdt, fdt.instance_id);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFile(FluteDemux& demux, const FluteFile& file)
{
    // Filter out files from non-filtered sessions.
    if (!isFiltered(file.sessionId())) {
        return;
    }

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

    // Process some known files in the announcement channel.
    if (file.sessionId().nipAnnouncementChannel()) {
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

            // Add all transport sessions in the session filter.
            const MulticastGatewayConfiguration mgc(_report, file);
            if (mgc.isValid()) {
                for (const auto& sess : mgc.sessions) {
                    for (const auto& id : sess.endpoints) {
                        addSession(id);
                    }
                }
            }
        }
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
        _report.debug(u"saving %s", actual_path);
        if (!file.toXML().save(actual_path, false, true)) {
            _report.error(u"error creating file %s", actual_path);
        }
    }
}
