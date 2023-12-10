//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSILogger.h"
#include "tsDuckContext.h"
#include "tsArgs.h"
#include "tsTablesDisplay.h"
#include "tsBinaryTable.h"
#include "tsSectionFile.h"
#include "tsxmlElement.h"
#include "tsjsonArray.h"
#include "tsjsonObject.h"
#include "tsTSPacket.h"

#define MIN_CLEAR_PACKETS 100000


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::PSILogger::PSILogger(TablesDisplay& display) :
    _display(display),
    _duck(_display.duck()),
    _report(_duck.report())
{
}

ts::PSILogger::~PSILogger()
{
    close();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILogger::defineArgs(Args& args)
{
    // Define XML options.
    _xml_tweaks.defineArgs(args);

    args.option(u"all-versions", 'a');
    args.help(u"all-versions",
              u"Display all versions of PSI tables (need to read the complete "
              u"transport stream). By default, display only the first version "
              u"of each PSI table and stop when all expected PSI are extracted.");

    args.option(u"cat-only");
    args.help(u"cat-only", u"Display only the CAT, ignore other PSI tables.");

    args.option(u"clear", 'c');
    args.help(u"clear",
              u"Indicate that this is a clear transport stream, without "
              u"conditional access information. Useful to avoid reading the "
              u"complete transport stream, waiting for a non-existent CAT.");

    args.option(u"dump", 'd');
    args.help(u"dump", u"Dump all PSI sections.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude PSI tables with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include PSI tables with \"next\" indicator. By default, they are excluded.");

    args.option(u"log-xml-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"log-xml-line", u"'prefix'",
              u"Log each table as one single XML line in the message logger instead of an output file. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the XML text to locate the appropriate line in the logs.");

    args.option(u"log-json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"log-json-line", u"'prefix'",
              u"Log each table as one single JSON line in the message logger instead of an output file. "
              u"The table is formatted as XML and automated XML-to-JSON conversion is applied. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");

    args.option(u"output-file", 'o', Args::FILENAME);
    args.help(u"output-file", u"filename",
              u"Save the tables in human-readable text format in the specified file. "
              u"By default, when no output option is specified, text is produced on the standard output. "
              u"If you need text formatting on the standard output in addition to other output such as XML, "
              u"explicitly specify this option with \"-\" as output file name.");

    args.option(u"text-output", 0, Args::FILENAME);
    args.help(u"text-output", u"filename", u"A synonym for --output-file.");

    args.option(u"xml-output", 'x',  Args::FILENAME);
    args.help(u"xml-output", u"filename",
              u"Save the tables in XML format in the specified file. "
              u"To output the XML text on the standard output, explicitly specify this option with \"-\" as output file name.");

    args.option(u"json-output", 'j',  Args::FILENAME);
    args.help(u"json-output", u"filename",
              u"Save the tables in JSON format in the specified file. "
              u"The tables are initially formatted as XML and automated XML-to-JSON conversion is applied. "
              u"To output the JSON text on the standard output, explicitly specify this option with \"-\" as output file name.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::PSILogger::loadArgs(DuckContext& duck, Args& args)
{
    // Type of output, text is the default.
    _use_xml = args.present(u"xml-output");
    _use_json = args.present(u"json-output");
    _log_xml_line = args.present(u"log-xml-line");
    _log_json_line = args.present(u"log-json-line");
    _use_text = args.present(u"output-file") ||
                args.present(u"text-output") ||
                (!_use_xml && !_use_json && !_log_xml_line && !_log_json_line &&
                 _table_handler == nullptr && _section_handler == nullptr);

    // --output-file and --text-output are synonyms.
    if (args.present(u"output-file") && args.present(u"text-output")) {
        args.error(u"--output-file and --text-output are synonyms, do not use both");
    }

    // Output destinations.
    args.getValue(_xml_destination, u"xml-output");
    args.getValue(_json_destination, u"json-output");
    args.getValue(_text_destination, u"output-file", args.value(u"text-output").c_str());
    args.getValue(_log_xml_prefix, u"log-xml-line");
    args.getValue(_log_json_prefix, u"log-json-line");

    // Other options.
    _all_versions = args.present(u"all-versions");
    _cat_only = args.present(u"cat-only");
    _clear = args.present(u"clear");
    _dump = args.present(u"dump");
    _use_current = !args.present(u"exclude-current");
    _use_next = args.present(u"include-next");

    // Load XML options.
    return _xml_tweaks.loadArgs(duck, args);
}


//----------------------------------------------------------------------------
// Open / close the PSI logger.
//----------------------------------------------------------------------------

bool ts::PSILogger::open()
{
    // Reset content.
    _xml_doc.clear();
    _x2j_conv.clear();
    _clear_packets_cnt = _scrambled_packets_cnt = 0;
    _previous_pat.clear();
    _previous_pat.invalidate();

    // Set XML options in document.
    _xml_doc.setTweaks(_xml_tweaks);
    _x2j_conv.setTweaks(_xml_tweaks);

    // Load the XML model for tables if we need to convert to JSON.
    if ((_use_json || _log_json_line) && !SectionFile::LoadModel(_x2j_conv)) {
        return false;
    }

    // Open/create the destination
    if (_use_text) {
        if (!_duck.setOutput(_text_destination)) {
            _abort = true;
            return false;
        }
        // Initial blank line
        _duck.out() << std::endl;
    }

    // Open/create the XML output.
    if (_use_xml && !_xml_doc.open(u"tsduck", u"", _xml_destination, std::cout)) {
        _abort = true;
        return false;
    }

    // Open/create the JSON output.
    if (_use_json) {
        json::ValuePtr root;
        if (_xml_tweaks.x2jIncludeRoot) {
            root = new json::Object;
            root->add(u"#name", u"tsduck");
            root->add(u"#nodes", json::ValuePtr(new json::Array));
        }
        if (!_json_doc.open(root, _json_destination, std::cout)) {
            _abort = true;
            return false;
        }
    }

    // Specify the PID filters
    _demux.reset();
    if (!_cat_only) {
        _demux.addPID(PID_PAT);   // MPEG
        _demux.addPID(PID_TSDT);  // MPEG
        _demux.addPID(PID_SDT);   // DVB, ISDB (also contain BAT)
        _demux.addPID(PID_PCAT);  // ISDB
        _demux.addPID(PID_BIT);   // ISDB
        _demux.addPID(PID_LDT);   // ISDB (also contain NBIT)
        _demux.addPID(PID_PSIP);  // ATSC
    }
    if (!_clear) {
        _demux.addPID(PID_CAT);
    }

    // Type of sections to get.
    _demux.setCurrentNext(_use_current, _use_next);

    return true;
}

void ts::PSILogger::close()
{
    _xml_doc.close();
    _json_doc.close();
}


//----------------------------------------------------------------------------
// Return true when the analysis is complete.
//----------------------------------------------------------------------------

bool ts::PSILogger::completed() const
{
    return _abort || (!_all_versions && _pat_ok && _cat_ok && _sdt_ok && _received_pmt >= _expected_pmt);
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::PSILogger::feedPacket(const TSPacket& pkt)
{
    // Feed the packet to the demux
    _demux.feedPacket(pkt);

    // On clear streams, there is no CAT (usually). To avoid waiting indefinitely,
    // if no CAT and no scrambled packet is found after a defined number of packets
    // (~ 4 seconds at satellite bitrate), no longer expect a CAT.
    if (pkt.getScrambling() == SC_CLEAR) {
        _clear_packets_cnt++;
    }
    else {
        _scrambled_packets_cnt++;
    }
    if (_scrambled_packets_cnt == 0 && _clear_packets_cnt > MIN_CLEAR_PACKETS) {
        _cat_ok = true;
    }

    // Check if the list of standards has changed.
    const Standards new_standards = _duck.standards();
    if (new_standards != _standards) {
        _report.debug(u"standards are now %s", {StandardsNames(new_standards)});
        _standards = new_standards;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::PSILogger::handleTable(SectionDemux&, const BinaryTable& table)
{
    assert(table.sectionCount() > 0);

    const TID tid = table.tableId();
    const PID pid = table.sourcePID();

    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(_duck, table);
            if (pid != PID_PAT) {
                // A PAT is only expected on PID 0
                _report.warning(u"got unexpected PAT on PID %d (0x%<X)", {pid});
            }
            else if (pat.isValid()) {
                // Got the PAT.
                _pat_ok = true;
                // Stop filtering the PAT PID if we don't need all versions.
                if (!_all_versions) {
                    _demux.removePID(pid);
                }
                // Reset all PMT PID's which disappeared or changed.
                if (_previous_pat.isValid()) {
                    for (const auto& prev_it : _previous_pat.pmts) {
                        const auto new_it = pat.pmts.find(prev_it.first);
                        if (new_it == pat.pmts.end() || new_it->second != prev_it.second) {
                            // Service disappeared or changed PMT PID, remove the previous PMT PID.
                            _demux.removePID(prev_it.second);
                        }
                    }
                }
                _previous_pat = pat;
                // Add a filter on each referenced PID to get the PMT
                for (const auto& it : pat.pmts) {
                    _demux.addPID(it.second);
                    _expected_pmt++;
                }
                // Also include NIT (considered as a PMT)
                _demux.addPID(pat.nit_pid != PID_NULL ? pat.nit_pid : PID(PID_NIT));
                _expected_pmt++;
            }
            displayTable(table);
            break;
        }

        case TID_CAT: {
            if (pid != PID_CAT) {
                // A CAT is only expected on PID 1
                _report.warning(u"got unexpected CAT on PID %d (0x%<X)", {pid});
            }
            else {
                // Got the CAT.
                _cat_ok = true;
                // Stop filtering the CAT PID if we don't need all versions.
                if (!_all_versions) {
                    _demux.removePID(pid);
                }
            }
            displayTable(table);
            break;
        }

        case TID_NIT_ACT:  // NIT and PMT are processed identically.
        case TID_PMT: {
            // Stop filtering this PID if we don't need all versions.
            if (!_all_versions) {
                _demux.removePID(pid);
                _received_pmt++;
            }
            displayTable(table);
            break;
        }

        case TID_NIT_OTH: {
            // Ignore NIT for other networks if only one version required
            if (_all_versions) {
                displayTable(table);
            }
            break;
        }

        case TID_TSDT: {
            if (pid != PID_TSDT) {
                // A TSDT is only expected on PID 0x0002
                _report.warning(u"got unexpected TSDT on PID %d (0x%<X)", {pid});
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            displayTable(table);
            break;
        }

        case TID_SDT_ACT: {
            if (pid != PID_SDT) {
                // An SDT is only expected on PID 0x0011
                _report.warning(u"got unexpected SDT on PID %d (0x%<X)", {pid});
                displayTable(table);
            }
            else if (_all_versions || !_sdt_ok) {
                _sdt_ok = true;
                // We cannot stop filtering this PID if we don't need all versions since a BAT can also be found here.
                displayTable(table);
            }
            break;
        }

        case TID_SDT_OTH: {
            // Ignore SDT for other networks if only one version required
            if (_all_versions) {
                displayTable(table);
            }
            break;
        }

        case TID_BAT: {
            if (pid != PID_BAT) {
                // An SDT is only expected on PID 0x0011
                _report.warning(u"got unexpected BAT on PID %d (0x%<X)", {pid});
                displayTable(table);
            }
            else if (_all_versions || !_bat_ok) {
                // Got the BAT.
                _bat_ok = true;
                // We cannot stop filtering this PID if we don't need all versions since the SDT can also be found here.
                displayTable(table);
            }
            break;
        }

        case TID_PCAT: {
            if (pid != PID_PCAT) {
                // An ISDB PCAT is only expected on PID 0x0022
                _report.warning(u"got unexpected ISDB PCAT on PID %d (0x%<X)", {pid});
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            displayTable(table);
            break;
        }

        case TID_BIT: {
            if (pid != PID_BIT) {
                // An ISDB BIT is only expected on PID 0x0024
                _report.warning(u"got unexpected ISDB BIT on PID %d (0x%<X)", {pid});
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            displayTable(table);
            break;
        }

        case TID_NBIT_REF:
        case TID_NBIT_BODY: {
            if (pid != PID_NBIT) {
                // An ISDB NBIT is only expected on PID 0x0025
                _report.warning(u"got unexpected ISDB NBIT on PID %d (0x%<X)", {pid});
            }
            // We cannot stop filtering this PID if we don't need all versions since the LDT can also be found here.
            displayTable(table);
            break;
        }

        // case TID_LDT: (same value as TID_MGT)
        case TID_MGT: {
            // ATSC MGT and ISDB LDT use the same table id, so it can be any.
            if (pid != PID_PSIP && pid != PID_LDT) {
                // An ATSC MGT is only expected on PID 0x1FFB.
                // An ISDB LDT is only expected on PID 0x0025.
                _report.warning(u"got unexpected ATSC MGT / ISDB LDT on PID %d (0x%<X)", {pid});
            }
            // We cannot stop filtering this PID if we don't need all versions
            // since the TVCT or CVCT (ATSC) and NBIT (ISDB) can also be found here.
            displayTable(table);
            break;
        }

        case TID_TVCT:
        case TID_CVCT: {
            // ATSC tables with channel description.
            displayTable(table);
            break;
        }

        default: {
            if (_duck.report().verbose()) {
                _report.warning(u"got unexpected TID %d (0x%<X) on PID %d (0x%<X)", {tid, pid});
            }
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --dump.
//----------------------------------------------------------------------------

void ts::PSILogger::handleSection(SectionDemux&, const Section& sect)
{
    sect.dump(_duck.out()) << std::endl;
}


//----------------------------------------------------------------------------
// Displays a binary table.
//----------------------------------------------------------------------------

void ts::PSILogger::displayTable(const BinaryTable& table)
{
    // Text output.
    if (_use_text) {
        _display.displayTable(table);
        _duck.out() << std::endl;
    }

    // XML options.
    BinaryTable::XMLOptions xml_options;
    xml_options.setPID = true;

    // Full XML output.
    if (_use_xml) {
        // Convert the table into an XML structure.
        table.toXML(_duck, _xml_doc.rootElement(), xml_options);

        // Print and delete the new table.
        _xml_doc.flush();
    }

    // Save table in JSON format.
    if (_use_json) {
        // First, build an XML document with the table.
        xml::Document doc(_report);
        doc.initialize(u"tsduck");
        table.toXML(_duck, doc.rootElement(), xml_options);

        // Convert to JSON. Force "tsduck" root to appear so that the path to the first table is always the same.
        // Query the first (and only) converted table and add it to the running document.
        _json_doc.add(_x2j_conv.convertToJSON(doc, true)->query(u"#nodes[0]"));
    }

    // XML and/or JSON one-liner in the log.
    if (_log_xml_line || _log_json_line) {

        // Build an XML document.
        xml::Document doc;
        doc.initialize(u"tsduck");

        // Convert the table into an XML structure.
        xml::Element* elem = table.toXML(_duck, doc.rootElement(), xml_options);
        if (elem != nullptr) {
            // Log the XML line.
            if (_log_xml_line) {
                _report.info(_log_xml_prefix + doc.oneLiner());
            }

            // Log the JSON line.
            if (_log_json_line) {
                // Convert the XML document into JSON.
                // Force "tsduck" root to appear so that the path to the first table is always the same.
                const json::ValuePtr root(_x2j_conv.convertToJSON(doc, true));

                // Query the first (and only) converted table and log it as one line.
                _report.info(_log_json_prefix + root->query(u"#nodes[0]").oneLiner(_report));
            }
        }
    }

    // Notify table, either at once or section by section.
    if (_table_handler != nullptr) {
        _table_handler->handleTable(_demux, table);
    }
    else if (_section_handler != nullptr) {
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            _section_handler->handleSection(_demux, *table.sectionAt(i));
        }
    }
}


//----------------------------------------------------------------------------
// Report the demux errors (if any)
//----------------------------------------------------------------------------

void ts::PSILogger::reportDemuxErrors()
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        _duck.out() << "* PSI/SI analysis errors:" << std::endl;
        status.display(_duck.out(), 4, true);
    }
}
