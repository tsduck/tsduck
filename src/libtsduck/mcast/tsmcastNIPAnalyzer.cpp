//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPAnalyzer.h"
#include "tsmcast.h"
#include "tsTextTable.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::mcast::NIPAnalyzer::NIPAnalyzer(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Reset the analysis.
//----------------------------------------------------------------------------

bool ts::mcast::NIPAnalyzer::reset(const NIPAnalyzerArgs& args)
{
    bool ok = _flute_demux.reset(args);
    _args = args;
    _session_filter.clear();
    _sessions.clear();
    _nacis.clear();
    _service_lists.clear();
    _services.clear();

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

void ts::mcast::NIPAnalyzer::addProtocolSession(const TransportProtocol& protocol, const FluteSessionId& session)
{
    // We currently support FLUTE only.
    if (protocol.protocol == FT_FLUTE) {
        addSession(session);
    }
    else {
        _report.warning(u"ignoring session %s, unsupported protocol %s", protocol.protocol_identifier);
    }
}

void ts::mcast::NIPAnalyzer::addSession(const FluteSessionId& session)
{
    if (!_session_filter.contains(session)) {
        _report.verbose(u"adding session %s", session);
        _session_filter.insert(session);
    }
}


//----------------------------------------------------------------------------
// Check if a UDP packet or FLUTE file is part of a filtered session.
//----------------------------------------------------------------------------

bool ts::mcast::NIPAnalyzer::isFiltered(const IPAddress& source, const IPSocketAddress& destination) const
{
    for (const auto& it : _session_filter) {
        if (it.source.match(source) && it.destination.match(destination)) {
            return true;
        }
    }
    return false;
}

bool ts::mcast::NIPAnalyzer::isFiltered(const FluteSessionId& session) const
{
    for (const auto& it : _session_filter) {
        if (it.match(session)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Process a NIPActualCarrierInformation.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteNACI(FluteDemux& demux, const NIPActualCarrierInformation& naci)
{
    _nacis.insert(naci);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteFile(FluteDemux& demux, const FluteFile& file)
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
            // Got a NIF.
            saveXML(file, _args.save_nif);
        }
        else if (name.similar(u"urn:dvb:metadata:nativeip:ServiceInformationFile")) {
            // Got a SIF.
            saveXML(file, _args.save_sif);
            const ServiceInformationFile sif(_report, file);
            if (sif.isValid()) {
                processSIF(demux, sif);
            }
        }
        else if (name.similar(u"urn:dvb:metadata:nativeip:dvb-i-slep")) {
            // Got a service list entry points.
            saveXML(file, _args.save_slep);
            const ServiceListEntryPoints slep(_report, file);
            if (slep.isValid()) {
                processSLEP(demux, slep);
            }
        }
    }

    // Process gateway configurations to find other sessions.
    const bool is_bootstrap = name.similar(u"urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap");
    if (is_bootstrap) {
        saveXML(file, _args.save_bootstrap);
    }
    if (is_bootstrap || file.type().similar(u"application/xml+dvb-mabr-session-configuration")) {
        const GatewayConfiguration mgc(_report, file);
        if (mgc.isValid()) {
            processGatewayConfiguration(demux, mgc);
        }
    }

    // Process service lists.
    if (file.type().similar(u"application/vnd.dvb.dvbisl+xml")) {
        const ServiceList slist(_report, file);
        if (slist.isValid()) {
            processServiceList(demux, slist);
        }
    }

    // Save carousel files.
    static const UString dvbgw_prefix(u"http://dvb.gw/");
    if (!_args.save_dvbgw_dir.empty() && name.starts_with(dvbgw_prefix)) {
        saveFile(file, _args.save_dvbgw_dir, name.substr(dvbgw_prefix.length()));
    }
}


//----------------------------------------------------------------------------
// Process a bootstrap or multicast gateway configuration.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processGatewayConfiguration(FluteDemux& demux, const GatewayConfiguration& mgc)
{
    // Add all transport sessions in the session filter.
    for (const auto& sess : mgc.transport_sessions) {
        for (const auto& id : sess.endpoints) {
            addProtocolSession(sess.protocol, id);
        }
    }

    for (const auto& sess1 : mgc.multicast_sessions) {
        for (const auto& sess2 : sess1.transport_sessions) {
            for (const auto& id : sess2.endpoints) {
                addProtocolSession(sess2.protocol, id);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a Service Information File (SIF).
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processSIF(FluteDemux& demux, const ServiceInformationFile& sif)
{
    // Register all NIP actual carrier information.
    NIPActualCarrierInformation naci;
    naci.valid = true;
    naci.stream_provider_name = sif.provider_name;
    for (const auto& st : sif.streams) {
        naci.stream_id = st.stream_id;
        handleFluteNACI(demux, naci);
    }
}


//----------------------------------------------------------------------------
// Process a Service List Entry Points (SLEP).
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processSLEP(FluteDemux& demux, const ServiceListEntryPoints& slep)
{
    // Grab all service lists.
    for (const auto& prov : slep.providers) {
        for (const auto& l1 : prov.lists) {
            for (const auto& l2 : l1.lists) {
                if (l2.type.contains(u"xml", CASE_INSENSITIVE)) {
                    auto& slc(_service_lists[l2.uri]);
                    slc.list_name = l1.name;
                    slc.provider_name = prov.provider.name;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a Service List.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processServiceList(FluteDemux& demux, const ServiceList& slist)
{
    // Report a verbose message if not yet registered from a service list entry point.
    if (!_service_lists.contains(slist.name())) {
        _report.verbose(u"unannounced service list %s on %s", slist.name(), slist.sessionId());
    }

    // Service list global properties.
    auto& slc(_service_lists[slist.name()]);
    slc.session_id = slist.sessionId();
    slc.list_name = slist.list_name;
    slc.provider_name = slist.provider_name;

    // Process each service.
    for (const auto& it1 : slist.services) {
        auto& serv(_services[it1.unique_id]);
        serv.service_name = it1.service_name;
        serv.provider_name = it1.provider_name;
        for (const auto& it2 : it1.instances) {
            auto& inst(serv.instances[it2.media_params]);
            inst.instance_priority = it2.priority;
            inst.media_type = it2.media_params_type;
        }
    }

    // Assign logical channel numbers.
    for (const auto& it1 : slist.lcn_tables) {
        for (const auto& it2 : it1.lcns) {
            auto& serv(_services[it2.service_ref]);
            serv.channel_number = it2.channel_number;
            serv.selectable = it2.selectable;
            serv.visible = it2.visible;
        }
    }
}


//----------------------------------------------------------------------------
// Save a XML file (if the file name is not empty).
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::saveXML(const FluteFile& file, const fs::path& path)
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

void ts::mcast::NIPAnalyzer::saveFile(const FluteFile& file, const fs::path& root_dir, const UString& relative_path)
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

void ts::mcast::NIPAnalyzer::printSummary(std::ostream& user_output)
{
    // Create the user-specified output file if required.
    const bool use_file = !_args.output_file.empty() && _args.output_file != u"-";
    std::ofstream outfile;
    if (use_file) {
        outfile.open(_args.output_file);
        if (!outfile) {
            _report.error(u"error creating %s", _args.output_file);
        }
    }
    std::ostream& out(use_file ? outfile : user_output);

    // Get status of incomplete files from the FLUTE demux.
    _flute_demux.getFilesStatus();

    // Display the DVB-NIP carrier information.
    out << std::endl << "DVB-NIP carriers: " << _nacis.size() << std::endl;
    for (const auto& naci : _nacis) {
        out << "Provider: \"" << naci.stream_provider_name << "\", " << naci.stream_id << std::endl;
    }
    out << std::endl;

    // Display service lists information.
    out << "Service lists: " << _service_lists.size() << std::endl;
    if (!_service_lists.empty()) {
        TextTable tab;
        enum col {PROVIDER, LISTNAME, SESSION, FILENAME};
        tab.addColumn(PROVIDER, u"Provider", TextTable::Align::LEFT);
        tab.addColumn(LISTNAME, u"List name", TextTable::Align::LEFT);
        tab.addColumn(SESSION,  u"Session id", TextTable::Align::LEFT);
        tab.addColumn(FILENAME, u"File URN", TextTable::Align::LEFT);
        for (const auto& it : _service_lists) {
            tab.setCell(PROVIDER, it.second.provider_name);
            tab.setCell(LISTNAME, it.second.list_name);
            tab.setCell(SESSION,  it.second.session_id.isValid() ? it.second.session_id.toString() : u"unknown");
            tab.setCell(FILENAME, it.first);
            tab.newLine();
        }
        tab.output(out, TextTable::Headers::TEXT, true, u"  ", u"  ");
    }
    out << std::endl;

    // Display services information.
    out << "Services: " << _services.size() << " (V: visible, S: selectable)" << std::endl;
    if (!_services.empty()) {
        // Build a temporary map, indexed by LCN, to get a sorted list of services.
        std::multimap<uint32_t, const ServiceContext*> sorted;
        for (const auto& it1 : _services) {
            sorted.insert(std::make_pair(it1.second.channel_number, &it1.second));
        }
        // Now use the sorted map to display.
        TextTable tab;
        enum {LCN, FLAGS, PROVIDER, SNAME, FILENAME, FILETYPE};
        tab.addColumn(LCN, u"LCN", TextTable::Align::RIGHT);
        tab.addColumn(FLAGS, u"VS", TextTable::Align::RIGHT);
        tab.addColumn(PROVIDER, u"Provider", TextTable::Align::LEFT);
        tab.addColumn(SNAME, u"Service", TextTable::Align::LEFT);
        tab.addColumn(FILENAME, u"Media URN", TextTable::Align::LEFT);
        tab.addColumn(FILETYPE, u"Type", TextTable::Align::LEFT);
        for (const auto& it1 : sorted) {
            const auto& serv(*it1.second);
            UString flags(u"--");
            if (serv.visible) {
                flags[0] = 'v';
            }
            if (serv.selectable) {
                flags[1] = 's';
            }
            if (serv.instances.empty()) {
                tab.setCell(LCN, UString::Decimal(serv.channel_number));
                tab.setCell(FLAGS, flags);
                tab.setCell(PROVIDER, serv.provider_name);
                tab.setCell(SNAME, serv.service_name);
                tab.newLine();
            }
            else {
                for (const auto& it2 : serv.instances) {
                    tab.setCell(LCN, UString::Decimal(serv.channel_number));
                    tab.setCell(FLAGS, flags);
                    tab.setCell(PROVIDER, serv.provider_name);
                    tab.setCell(SNAME, serv.service_name);
                    tab.setCell(FILENAME, it2.first);
                    tab.setCell(FILETYPE, it2.second.media_type);
                    tab.newLine();
                }
            }
        }
        tab.output(out, TextTable::Headers::TEXT, true, u"  ", u"  ");
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
            enum {SIZE, TOI, STATUS, NAME, TYPE};
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

    // Close the user-specified output file if required.
    if (use_file) {
        outfile.close();
    }
}


//----------------------------------------------------------------------------
// Invoked by FluteDemux::getFilesStatus() for each file.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteStatus(FluteDemux& demux,
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
            // Remove qualification such as "charset=utf-8" in type.
            const size_t sc = file->second.type.find(u";");
            if (sc < file->second.type.length()) {
                file->second.type.resize(sc);
            }
        }
    }
}
