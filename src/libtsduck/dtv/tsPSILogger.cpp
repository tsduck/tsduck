//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPSILogger.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsxmlComment.h"
#include "tsxmlElement.h"
#include "tsTSPacket.h"
#include "tsNames.h"
#include "tsPAT.h"
TSDUCK_SOURCE;

#define MIN_CLEAR_PACKETS 100000


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::PSILogger::PSILogger(TablesDisplay& display) :
    ArgsSupplierInterface(),
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _all_versions(false),
    _clear(false),
    _cat_only(false),
    _dump(false),
    _use_text(false),
    _use_xml(false),
    _log_xml_line(false),
    _use_current(true),
    _use_next(false),
    _text_destination(),
    _xml_destination(),
    _log_xml_prefix(),
    _xml_tweaks(),
    _display(display),
    _duck(_display.duck()),
    _report(_duck.report()),
    _xml_doc(_report),
    _abort(false),
    _pat_ok(_cat_only),
    _cat_ok(_clear),
    _sdt_ok(_cat_only),
    _bat_ok(false),
    _expected_pmt(0),
    _received_pmt(0),
    _clear_packets_cnt(0),
    _scrambled_packets_cnt(0),
    _demux(_duck, this, _dump ? this : nullptr),
    _standards(Standards::NONE)
{
}

ts::PSILogger::~PSILogger()
{
    close();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILogger::defineArgs(Args& args) const
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

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"filename",
              u"Save the tables in human-readable text format in the specified file. "
              u"By default, when no output option is specified, text is produced on the standard output. "
              u"If you need text formatting on the standard output in addition to other output such as XML, "
              u"explicitly specify this option with \"-\" as output file name.");

    args.option(u"text-output", 0, Args::STRING);
    args.help(u"text-output", u"filename", u"A synonym for --output-file.");

    args.option(u"xml-output", 'x',  Args::STRING);
    args.help(u"xml-output", u"filename",
              u"Save the tables in XML format in the specified file. To output the XML "
              u"text on the standard output, explicitly specify this option with \"-\" "
              u"as output file name.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::PSILogger::loadArgs(DuckContext& duck, Args& args)
{
    // Type of output, text is the default.
    _use_xml = args.present(u"xml-output");
    _log_xml_line = args.present(u"log-xml-line");
    _use_text = args.present(u"output-file") || args.present(u"text-output") || (!_use_xml && !_log_xml_line);

    // --output-file and --text-output are synonyms.
    if (args.present(u"output-file") && args.present(u"text-output")) {
        args.error(u"--output-file and --text-output are synonyms, do not use both");
    }

    // Output destinations.
    args.getValue(_xml_destination, u"xml-output");
    args.getValue(_text_destination, u"output-file", args.value(u"text-output").c_str());
    args.getValue(_log_xml_prefix, u"log-xml-line");

    // Accept "-" as a specification for standard output (common convention in UNIX world).
    if (_text_destination == u"-") {
        _text_destination.clear();
    }
    if (_xml_destination == u"-") {
        _xml_destination.clear();
    }

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
    // Open/create the destination
    if (_use_text) {
        if (!_duck.setOutput(_text_destination)) {
            _abort = true;
            return false;
        }
        // Initial blank line
        _duck.out() << std::endl;
    }

    // Set XML options in document.
    _xml_doc.clear();
    _xml_doc.setTweaks(_xml_tweaks);

    // Open/create the XML output.
    if (_use_xml && !_xml_doc.open(u"tsduck", u"", _xml_destination, std::cout)) {
        _abort = true;
        return false;
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
    // Complete XML output.
    _xml_doc.close();
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
                // Add a filter on each referenced PID to get the PMT
                for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _demux.addPID(it->second);
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
    BinaryTable::XMLOptions xml_opt;
    xml_opt.setPID = true;

    // Full XML output.
    if (_use_xml) {
        // Convert the table into an XML structure.
        table.toXML(_duck, _xml_doc.rootElement(), xml_opt);

        // Print and delete the new table.
        _xml_doc.flush();
    }

    // XML one-liner in the log.
    if (_log_xml_line) {

        // Build an XML document.
        xml::Document doc;
        doc.initialize(u"tsduck");

        // Convert the table into an XML structure.
        xml::Element* elem = table.toXML(_duck, doc.rootElement(), xml_opt);
        if (elem != nullptr) {
            // Initialize a text formatter for one-liner.
            TextFormatter text(_report);
            text.setString();
            text.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);

            // Serialize the XML object inside the text formatter.
            doc.print(text);

            // Log the XML line.
            _report.info(_log_xml_prefix + text.toString());
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
