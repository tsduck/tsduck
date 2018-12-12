//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Command line arguments for the class TablesLogger.
//
//----------------------------------------------------------------------------

#include "tsTablesLoggerArgs.h"
#include "tsException.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::TablesLoggerArgs::DEFAULT_LOG_SIZE;
#endif


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TablesLoggerArgs::TablesLoggerArgs() :
    use_text(false),
    use_xml(false),
    use_binary(false),
    use_udp(false),
    text_destination(),
    xml_destination(),
    bin_destination(),
    udp_destination(),
    multi_files(false),
    flush(false),
    rewrite_xml(false),
    rewrite_binary(false),
    udp_local(),
    udp_ttl(0),
    udp_raw(false),
    all_sections(false),
    all_once(false),
    max_tables(0),
    time_stamp(false),
    packet_index(false),
    diversified(false),
    logger(false),
    log_size(DEFAULT_LOG_SIZE),
    negate_tid(false),
    negate_tidext(false),
    pid(),
    add_pmt_pids(false),
    no_duplicate(false),
    pack_all_sections(false),
    pack_and_flush(false),
    fill_eit(false),
    tid(),
    tidext(),
    use_current(true),
    use_next(false),
    xml_tweaks()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesLoggerArgs::defineOptions(Args& args) const
{
    xml_tweaks.defineOptions(args);

    args.option(u"all-once");
    args.help(u"all-once",
              u"Same as --all-sections but collect each section only once per combination of "
              u"PID, table id, table id extension, section number and version.");

    args.option(u"all-sections", 'a');
    args.help(u"all-sections", u"Display/save all sections, as they appear in the stream. By default, "
              u"collect complete tables, with all sections of the tables grouped and "
              u"ordered and collect each version of a table only once. Note that this "
              u"mode is incompatible with --xml-output since valid XML structures may "
              u"contain complete tables only.");

    args.option(u"binary-output", 'b', Args::STRING);
    args.help(u"binary-output", u"filename",
              u"Save sections in the specified binary output file. "
              u"See also option -m, --multiple-files.");

    args.option(u"diversified-payload", 'd');
    args.help(u"diversified-payload",
              u"Select only sections with \"diversified\" payload. This means that "
              u"section payloads containing the same byte value (all 0x00 or all 0xFF "
              u"for instance) are ignored. Typically, such sections are stuffing and "
              u"can be ignored that way.");

    args.option(u"fill-eit");
    args.help(u"fill-eit",
              u"Before exiting, add missing empty sections in EIT's and flush them. "
              u"This can be useful with segmented EIT schedule where empty sections "
              u"at end of segments are usually not transmitted.");

    args.option(u"flush", 'f');
    args.help(u"flush", u"Flush output after each display.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude short sections and long sections with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include long sections with \"next\" indicator. By default, they are excluded.");

    args.option(u"ip-udp", 'i', Args::STRING);
    args.help(u"ip-udp", u"address:port",
              u"Send binary tables over UDP/IP to the specified destination. "
              u"The 'address' specifies an IP address which can be either unicast "
              u"or multicast. It can be also a host name that translates to an IP "
              u"address. The 'port' specifies the destination UDP port.");

    args.option(u"local-udp", 0, Args::STRING);
    args.help(u"local-udp", u"address",
              u"With --ip-udp, when the destination is a multicast address, specify "
              u"the IP address of the outgoing local interface. It can be also a host "
              u"name that translates to a local address.");

    args.option(u"log", 0);
    args.help(u"log", u"Display a short one-line log of each table instead of full table display.");

    args.option(u"log-size", 0, Args::UNSIGNED);
    args.help(u"log-size",
              u"With option --log, specify how many bytes are displayed at the "
              u"beginning of the table payload (the header is not displayed). "
              u"The default is 8 bytes.");

    args.option(u"max-tables", 'x', Args::POSITIVE);
    args.help(u"max-tables", u"Maximum number of tables to dump. Stop logging tables when this limit is reached.");

    args.option(u"multiple-files", 'm');
    args.help(u"multiple-files",
              u"Create multiple binary output files, one per section. A binary "
              u"output file name must be specified (option -b or --binary-output). "
              u"Assuming that the specified file name has the form 'base.ext', "
              u"each file is created with the name 'base_pXXXX_tXX.ext' for "
              u"short sections and 'base_pXXXX_tXX_eXXXX_vXX_sXX.ext' for long "
              u"sections, where the XX specify the hexadecimal values of the "
              u"PID, TID (table id), TIDext (table id extension), version and "
              u"section index.");

    args.option(u"negate-pid");
    args.help(u"negate-pid",
              u"Negate the PID filter: specified PID's are excluded. "
              u"Warning: this can be a dangerous option on complete transport "
              u"streams since PID's not containing sections can be accidentally "
              u"selected.");

    args.option(u"negate-tid", 'n');
    args.help(u"negate-tid", u"Negate the TID filter: specified TID's are excluded.");

    args.option(u"negate-tid-ext");
    args.help(u"negate-tid-ext", u"Negate the TID extension filter: specified TID extensions are excluded.");

    args.option(u"no-duplicate");
    args.help(u"no-duplicate",
              u"Do not report consecutive identical tables with a short section in the "
              u"same PID. This can be useful for ECM's. This is the way to display new "
              u"ECM's only. By default, tables with long sections are reported only when "
              u"a new version is detected but tables with a short section are all reported.");

    args.option(u"no-encapsulation");
    args.help(u"no-encapsulation",
              u"With --ip-udp, send the tables as raw binary messages in UDP packets. "
              u"By default, the tables are formatted into TLV messages.");

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"",
              u"Save the tables or sections in human-readable text format in the specified "
              u"file. By default, when no output option is specified, text is produced on "
              u"the standard output. If you need text formatting on the standard output in "
              u"addition to other output like binary files or UPD/IP, explicitly specify "
              u"this option with \"-\" as output file name.\n\n"
              u"By default, the tables are interpreted and formatted as text on the standard "
              u"output. Several destinations can be specified at the same time: human-readable "
              u"text output, binary output, UDP/IP messages.");

    args.option(u"pack-all-sections");
    args.help(u"pack-all-sections",
              u"Same as --all-sections but also modify each long section so that it becomes a "
              u"valid complete table. Its section_number and last_section_number are forced "
              u"to zero. Use with care because this may create inconsistent tables. This "
              u"option can be useful with tables with sparse sections such as EIT's to save "
              u"them in XML format (as an alternative, see also --fill-eit).");

    args.option(u"pack-and-flush");
    args.help(u"pack-and-flush",
              u"Before exiting, pack incomplete tables, ignoring missing sections, and flush "
              u"them. Use with care because this may create inconsistent tables. Unlike option "
              u"--pack-all-sections, --pack-and-flush does not force --all-sections because it "
              u"only applies to the last incomplete tables before exiting.");

    args.option(u"packet-index");
    args.help(u"packet-index",
              u"Display the index of the first and last TS packet of each displayed "
              u"section or table.");

    args.option(u"pid", 'p', Args::PIDVAL, 0, Args::UNLIMITED_COUNT);
    args.help(u"pid", u"pid1[-pid2]",
              u"PID filter: select packets with this PID value or range of PID values. "
              u"Several -p or --pid options may be specified. "
              u"Without -p or --pid option, all PID's are used (this can be a "
              u"dangerous option on complete transport streams since PID's not "
              u"containing sections can be accidentally selected).");

    args.option(u"psi-si");
    args.help(u"psi-si",
              u"Add all PID's containing PSI/SI tables, ie. PAT, CAT, PMT, NIT, SDT "
              u"and BAT. Note that EIT, TDT and TOT are not included. Use --pid 18 "
              u"to get EIT and --pid 20 to get TDT and TOT.");

    args.option(u"rewrite-binary");
    args.help(u"rewrite-binary",
              u"With --binary-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"rewrite-xml");
    args.help(u"rewrite-xml",
              u"With --xml-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"text-output", 0, Args::STRING);
    args.help(u"text-output", u"A synonym for --output-file.");

    args.option(u"tid", 't', Args::UINT8, 0, Args::UNLIMITED_COUNT);
    args.help(u"tid", u"tid1[-tid2]",
              u"TID filter: select sections with this TID (table id) value or range of TID values. "
              u"Several -t or --tid options may be specified. "
              u"Without -t or --tid option, all tables are saved.");

    args.option(u"tid-ext", 'e', Args::UINT16, 0, Args::UNLIMITED_COUNT);
    args.help(u"tid-ext", u"ext1[-ext2]",
              u"TID extension filter: select sections with this table id "
              u"extension value or range of values (apply to long sections only). "
              u"Several -e or --tid-ext options may be specified. "
              u"Without -e or --tid-ext option, all tables are saved.");

    args.option(u"time-stamp");
    args.help(u"time-stamp", u"Display a time stamp (current local time) with each table.");

    args.option(u"ttl", 0, Args::POSITIVE);
    args.help(u"ttl",
              u"With --ip-udp, specifies the TTL (Time-To-Live) socket option. "
              u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
              u"depending on the destination address. Remember that the default "
              u"Multicast TTL is 1 on most systems.");

    args.option(u"xml-output", 0,  Args::STRING);
    args.help(u"xml-output", u"filename",
              u"Save the tables in XML format in the specified file. To output the XML "
              u"text on the standard output, explicitly specify this option with \"-\" "
              u"as output file name.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::TablesLoggerArgs::load(Args& args)
{
    // Type of output, text is the default.
    use_xml = args.present(u"xml-output");
    use_binary = args.present(u"binary-output");
    use_udp = args.present(u"ip-udp");
    use_text = args.present(u"output-file") || args.present(u"text-output") || ( !use_xml && !use_binary && !use_udp);

    // --output-file and --text-output are synonyms.
    if (args.present(u"output-file") && args.present(u"text-output")) {
        args.error(u"--output-file and --text-output are synonyms, do not use both");
    }

    // Output destinations.
    xml_destination = args.value(u"xml-output");
    bin_destination = args.value(u"binary-output");
    udp_destination = args.value(u"ip-udp");
    text_destination = args.value(u"output-file", args.value(u"text-output").c_str());

    // Accept "-" as a specification for standard output (common convention in UNIX world).
    if (text_destination == u"-") {
        text_destination.clear();
    }
    if (xml_destination == u"-") {
        xml_destination.clear();
    }

    multi_files = args.present(u"multiple-files");
    rewrite_binary = args.present(u"rewrite-binary");
    rewrite_xml = args.present(u"rewrite-xml");
    flush = args.present(u"flush");
    udp_local = args.value(u"local-udp");
    udp_ttl = args.intValue(u"ttl", 0);
    pack_all_sections = args.present(u"pack-all-sections");
    pack_and_flush = args.present(u"pack-and-flush");
    fill_eit = args.present(u"fill-eit");
    all_once = args.present(u"all-once");
    all_sections = all_once || pack_all_sections || args.present(u"all-sections");
    max_tables = args.intValue<uint32_t>(u"max-tables", 0);
    time_stamp = args.present(u"time-stamp");
    packet_index = args.present(u"packet-index");
    diversified = args.present(u"diversified-payload");
    logger = args.present(u"log");
    log_size = args.intValue<size_t>(u"log-size", DEFAULT_LOG_SIZE);
    negate_tid = args.present(u"negate-tid");
    negate_tidext = args.present(u"negate-tid-ext");
    no_duplicate = args.present(u"no-duplicate");
    udp_raw = args.present(u"no-encapsulation");
    add_pmt_pids = args.present(u"psi-si");
    use_current = !args.present(u"exclude-current");
    use_next = args.present(u"include-next");

    if (add_pmt_pids || args.present(u"pid")) {
        args.getIntValues(pid, u"pid"); // specific pids
        if (args.present(u"negate-pid")) {
            pid.flip();
        }
        if (add_pmt_pids) { // --psi-si
            pid.set(PID_PAT);
            pid.set(PID_CAT);
            pid.set(PID_SDT); // also BAT
            pid.set(PID_NIT);
        }
    }
    else {
        pid.set(); // all PIDs
    }

    args.getIntValues(tid, u"tid");
    args.getIntValues(tidext, u"tid-ext");

    // Check consistency of options.
    if (rewrite_binary && multi_files) {
        args.error(u"options --rewrite-binary and --multiple-files are incompatible");
        return false;
    }

    // Load XML options.
    return xml_tweaks.load(args);
}
