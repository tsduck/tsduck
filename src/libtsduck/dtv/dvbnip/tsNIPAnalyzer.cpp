//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzer.h"
#include "tsNIP.h"
#include "tsServiceInformationFile.h"
#include "tsMulticastGatewayConfiguration.h"
#include "tsTextTable.h"
#include "tsErrCodeReport.h"


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

bool ts::NIPAnalyzer::reset(const NIPAnalyzerArgs& args)
{
    bool ok = _flute_demux.reset(args);
    _args = args;
    _session_filter.clear();
    _sessions.clear();
    _nacis.clear();

    // Filter the DVB-NIP announcement channel (IPv4 and IPv6).
    static const FluteSessionId announce4(IPAddress(), NIPSignallingAddress4(), NIP_SIGNALLING_TSI);
    static const FluteSessionId announce6(IPAddress(), NIPSignallingAddress6(), NIP_SIGNALLING_TSI);
    addSession(announce4);
    addSession(announce6);

    // Check that the root directory exists for carousel files.
    if (!_args.save_dvbgw_dir.empty() && !fs::is_directory(_args.save_dvbgw_dir)) {
        _report.error(u"directory not found: %s", _args.save_dvbgw_dir);
        ok = false;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Add a FLUTE session in the DVB-NIP analyzer.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::addSession(const FluteSessionId& session)
{
    if (!_session_filter.contains(session)) {
        _report.verbose(u"adding session %s", session);
        _session_filter.insert(session);
    }
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
// Process a NIPActualCarrierInformation.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteNACI(FluteDemux& demux, const NIPActualCarrierInformation& naci)
{
    _nacis.insert(naci);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteFile(FluteDemux& demux, const FluteFile& file)
{
    const UString& name(file.name());

    // Filter out files from non-filtered sessions.
    if (!isFiltered(file.sessionId())) {
        _report.debug(u"ignoring %s from %s", file.name(), file.sessionId());
        return;
    }

    // Remember statistics about files.
    if (_args.summary) {
        auto& session(_sessions[file.sessionId()]);
        auto& fctx(session.files[file.name()]);
        fctx.complete = true;
        fctx.type = file.type();
        fctx.size = fctx.received = file.size();
        fctx.toi = file.toi();
    }

    // Process some known files in the announcement channel.
    if (file.sessionId().nipAnnouncementChannel()) {
        if (name.similar(u"urn:dvb:metadata:nativeip:NetworkInformationFile")) {
            saveXML(file, _args.save_nif);
        }
        else if (name.similar(u"urn:dvb:metadata:nativeip:ServiceInformationFile")) {
            saveXML(file, _args.save_sif);
            ServiceInformationFile sif(_report, file);
            if (sif.isValid()) {
                NIPActualCarrierInformation naci;
                naci.valid = true;
                naci.nip_stream_provider_name = sif.provider_name;
                for (const auto& st : sif.streams) {
                    naci.nip_network_id = st.nip_network_id;
                    naci.nip_carrier_id = st.nip_carrier_id;
                    naci.nip_link_id = st.nip_link_id;
                    naci.nip_service_id = st.nip_service_id;
                    handleFluteNACI(demux, naci);
                }
            }
        }
        else if (name.similar(u"urn:dvb:metadata:nativeip:dvb-i-slep")) {
            saveXML(file, _args.save_slep);
        }
    }

    // Process gateway configurations to find other sessions.
    const bool is_bootstrap = name.similar(u"urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap");
    if (is_bootstrap) {
        saveXML(file, _args.save_bootstrap);
    }
    if (is_bootstrap || file.type().similar(u"application/xml+dvb-mabr-session-configuration")) {
        // Add all transport sessions in the session filter.
        const MulticastGatewayConfiguration mgc(_report, file);
        _report.debug(u"got %s session configuration in %s, %s", mgc.isValid() ? u"valid" : u"invalid", name, file.sessionId());
        if (mgc.isValid()) {
            for (const auto& sess : mgc.transport_sessions) {
                for (const auto& id : sess.endpoints) {
                    addSession(id);
                }
            }
            for (const auto& sess1 : mgc.multicast_sessions) {
                for (const auto& sess2 : sess1.transport_sessions) {
                    for (const auto& id : sess2.endpoints) {
                        addSession(id);
                    }
                }
            }
        }
    }

    // Save carousel files.
    static const UString dvbgw_prefix(u"http://dvb.gw/");
    if (!_args.save_dvbgw_dir.empty() && name.starts_with(dvbgw_prefix)) {
        saveFile(file, _args.save_dvbgw_dir, name.substr(dvbgw_prefix.length()));
    }
}


//----------------------------------------------------------------------------
// Save a XML file (if the file name is not empty).
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::saveXML(const FluteFile& file, const fs::path& path)
{
    // Don't save the file if the path is empty.
    if (!path.empty()) {
        _report.debug(u"saving %s", path);
        if (!file.toXML().save(path, false, true)) {
            _report.error(u"error creating file %s", path);
        }
    }
}


//----------------------------------------------------------------------------
// Save a carousel file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::saveFile(const FluteFile& file, const fs::path& root_dir, const UString& relative_path)
{
    // Cleanup the file path to avoid directory traversal attack.
    UStringVector comp;
    relative_path.split(comp, u'/', true, true);
    fs::path path(root_dir);
    fs::path basename;
    for (size_t i = 0; i < comp.size(); ++i) {
        if (comp[i] != u"." && comp[i] != u"..") {
            if (i + 1 < comp.size()) {
                path /= comp[i];
            }
            else {
                basename = comp[i];
            }
        }
    }
    if (basename.empty()) {
        _report.error(u"no filename specified in \"%s\"", relative_path);
        return;
    }

    // Create intermediate subdirectories if required.
    fs::create_directories(path, &ErrCodeReport(_report, u"error creating directory", path));

    // Save final file.
    path /= basename;
    _report.verbose(u"saving %s", path);
    file.content().saveToFile(path, &_report);
}


//----------------------------------------------------------------------------
// Print a summary of the DVB-NIP session.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::printSummary(std::ostream& out)
{
    // Get status of incomplete files from the FLUTE demux.
    _flute_demux.getFilesStatus();

    // Display the DVB-NIP carrier information.
    out << std::endl << "DVB-NIP carriers: " << _nacis.size() << std::endl;
    for (const auto& naci : _nacis) {
        out << "Provider: \"" << naci.nip_stream_provider_name
            << "\", network: " << naci.nip_network_id
            << ", carrier: " << naci.nip_carrier_id
            << ", link: " << naci.nip_link_id
            << ", service: " << naci.nip_service_id << std::endl;
    }
    out << std::endl;

    // Display the status of all files.
    size_t session_count = 0;
    for (const auto& sess : _sessions) {
        out << "Session #" << (++session_count) << ": " << sess.first << std::endl;
        if (sess.second.files.empty()) {
            out << "  No file received" << std::endl;
        }
        else {
            TextTable tab;
            enum col {SIZE, TOI, STATUS, NAME, TYPE};
            tab.addColumn(SIZE, u"Size", TextTable::Align::RIGHT);
            tab.addColumn(TOI, u"TOI", TextTable::Align::RIGHT);
            tab.addColumn(STATUS, u"Status", TextTable::Align::RIGHT);
            tab.addColumn(NAME, u"Name", TextTable::Align::LEFT);
            tab.addColumn(TYPE, u"Type", TextTable::Align::LEFT);
            for (const auto& file : sess.second.files) {
                tab.setCell(SIZE, UString::Decimal(file.second.size));
                tab.setCell(TOI, UString::Decimal(file.second.toi));
                tab.setCell(STATUS, file.second.complete ? u"complete" : UString::Decimal(file.second.received));
                tab.setCell(NAME, file.first);
                tab.setCell(TYPE, file.second.type);
                tab.newLine();
            }
            tab.output(out, TextTable::Headers::TEXT, true, u"  ", u"  ");
        }
        out << std::endl;
    }
}


//----------------------------------------------------------------------------
// Invoked by FluteDemux::getFilesStatus() for each file.
//----------------------------------------------------------------------------

void ts::NIPAnalyzer::handleFluteStatus(FluteDemux& demux,
                                        const FluteSessionId& session_id,
                                        const UString& name,
                                        const UString& type,
                                        uint64_t toi,
                                        uint64_t total_length,
                                        uint64_t received_length)
{
    auto& session(_sessions[session_id]);
    auto file = session.files.find(name);

    // If the file is unnamed, try to find a matching TOI in the session.
    if (name.empty()) {
        for (file = session.files.begin(); file != session.files.end() && file->second.toi != toi; ++file) {
        }
    }

    // If the file is still not found, create an entry for an incomplete file.
    if (file == session.files.end()) {
        // Do not create a new entry for an FDT. This is a FLUTE-level file, not a DVB-NIP one.
        if (toi == 0) {
            return;
        }
        // Create an entry for an incomplete file.
        UString new_name(name);
        if (new_name.empty()) {
            new_name.format(u"(unknown, TOI %d)", toi);
        }
        file = session.files.emplace(new_name, FileContext()).first;
    }

    // If the file is not completely received, update the description.
    if (!file->second.complete) {
        file->second.size = total_length;
        file->second.received = received_length;
        file->second.toi = toi;
        if (!type.empty()) {
            file->second.type = type;
        }
    }
}
