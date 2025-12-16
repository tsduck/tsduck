//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPAnalyzer.h"
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
    // Check that the root directory exists for carousel files.
    if (!args.save_dvbgw_dir.empty() && !fs::is_directory(args.save_dvbgw_dir)) {
        _report.error(u"directory not found: %s", args.save_dvbgw_dir);
        return false;
    }

    // Local initialization.
    _args = args;
    _nacis.clear();
    return _demux.reset(_args, _args.summary);
}


//----------------------------------------------------------------------------
// Process a NIPActualCarrierInformation.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteNACI(const NIPActualCarrierInformation& naci)
{
    _nacis.insert(naci);
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteFile(const FluteFile& file)
{
    // Process some files from the DVB-NIP announcement channel.
    if (file.sessionId().nipAnnouncementChannel()) {
        if (file.name().similar(u"urn:dvb:metadata:nativeip:NetworkInformationFile")) {
            // Got a NIF.
            saveXML(file, _args.save_nif);
        }
        else if (file.name().similar(u"urn:dvb:metadata:nativeip:ServiceInformationFile")) {
            // Process a SIF.
            saveXML(file, _args.save_sif);
        }
        else if (file.name().similar(u"urn:dvb:metadata:nativeip:dvb-i-slep")) {
            // Process a service list entry points.
            saveXML(file, _args.save_slep);
        }
        else if (file.name().similar(u"urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap")) {
            // Process boostrap file.
            saveXML(file, _args.save_bootstrap);
        }
    }

    // Save carousel files.
    static const UString dvbgw_prefix(u"http://dvb.gw/");
    if (!_args.save_dvbgw_dir.empty() && file.name().starts_with(dvbgw_prefix)) {
        saveFile(file, _args.save_dvbgw_dir, file.name().substr(dvbgw_prefix.length()));
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

    // Display the DVB-NIP carrier information.
    out << std::endl << "DVB-NIP carriers: " << _nacis.size() << std::endl;
    for (const auto& naci : _nacis) {
        out << "Provider: \"" << naci.stream_provider_name << "\", " << naci.stream_id << std::endl;
    }
    out << std::endl;

    // Get service lists.
    std::list<NIPDemux::ServiceListContext> service_lists;
    _demux.getServiceLists(service_lists);

    // Display service lists information.
    out << "Service lists: " << service_lists.size() << std::endl;
    if (!service_lists.empty()) {
        TextTable tab;
        enum col {PROVIDER, LISTNAME, SESSION, FILENAME};
        tab.addColumn(PROVIDER, u"Provider", TextTable::Align::LEFT);
        tab.addColumn(LISTNAME, u"List name", TextTable::Align::LEFT);
        tab.addColumn(SESSION,  u"Session id", TextTable::Align::LEFT);
        tab.addColumn(FILENAME, u"File URN", TextTable::Align::LEFT);
        for (const auto& it : service_lists) {
            tab.setCell(PROVIDER, it.provider_name);
            tab.setCell(LISTNAME, it.list_name);
            tab.setCell(SESSION,  it.session_id.isValid() ? it.session_id.toString() : u"unknown");
            tab.setCell(FILENAME, it.file_name);
            tab.newLine();
        }
        tab.output(out, TextTable::Headers::TEXT, true, u"  ", u"  ");
    }
    out << std::endl;

    // Get services descriptions.
    std::list<NIPService> services;
    _demux.getServices(services);

    // Display services information.
    out << "Services: " << services.size() << " (V: visible, S: selectable)" << std::endl;
    if (!services.empty()) {
        TextTable tab;
        enum {LCN, FLAGS, TYPE, PROVIDER, SNAME, FILENAME, FILETYPE};
        tab.addColumn(LCN, u"LCN", TextTable::Align::RIGHT);
        tab.addColumn(FLAGS, u"VS", TextTable::Align::LEFT);
        tab.addColumn(TYPE, u"Type", TextTable::Align::LEFT);
        tab.addColumn(PROVIDER, u"Provider", TextTable::Align::LEFT);
        tab.addColumn(SNAME, u"Service", TextTable::Align::LEFT);
        tab.addColumn(FILENAME, u"Media URN", TextTable::Align::LEFT);
        tab.addColumn(FILETYPE, u"Type", TextTable::Align::LEFT);
        for (const auto& serv : services) {
            UString flags(u"--");
            if (serv.visible) {
                flags[0] = 'v';
            }
            if (serv.selectable) {
                flags[1] = 's';
            }
            UString type (serv.service_type);
            const size_t colon = type.rfind(u':');
            if (type.empty()) {
                type = u"linear"; // this is the default
            }
            else if (colon < type.length()) {
                type.erase(0, colon + 1);
            }
            if (serv.instances.empty()) {
                tab.setCell(LCN, UString::Decimal(serv.channel_number));
                tab.setCell(FLAGS, flags);
                tab.setCell(TYPE, type);
                tab.setCell(PROVIDER, serv.provider_name);
                tab.setCell(SNAME, serv.service_name);
                tab.newLine();
            }
            else {
                for (const auto& it2 : serv.instances) {
                    tab.setCell(LCN, UString::Decimal(serv.channel_number));
                    tab.setCell(FLAGS, flags);
                    tab.setCell(TYPE, type);
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
    _demux.getFluteDemux().printFilesStatus(out);

    // Close the user-specified output file if required.
    if (use_file) {
        outfile.close();
    }
}
