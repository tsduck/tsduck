//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
    udp_local(),
    udp_ttl(0),
    udp_raw(false),
    all_sections(false),
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
    tid(),
    tidext()
{
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TablesLoggerArgs::addHelp(Args& args) const
{
    UString help =
        u"\n"
        u"Tables and sections logging options:\n"
        u"\n"
        u"  By default, the tables are interpreted and formatted as text on the standard\n"
        u"  output. Several destinations can be specified at the same time: human-readable\n"
        u"  text output, binary output, UDP/IP messages.\n"
        u"\n"
        u"  -a\n"
        u"  --all-sections\n"
        u"      Display/save all sections, as they appear in the stream. By default,\n"
        u"      collect complete tables, with all sections of the tables grouped and\n"
        u"      ordered and collect each version of a table only once. Note that this\n"
        u"      mode is incompatible with --xml-output since valid XML structures may\n"
        u"      contain complete tables only.\n"
        u"\n"
        u"  -b filename\n"
        u"  --binary-output filename\n"
        u"      Save sections in the specified binary output file.\n"
        u"      See also option -m, --multiple-files.\n"
        u"\n"
        u"  -d\n"
        u"  --diversified-payload\n"
        u"      Select only sections with \"diversified\" payload. This means that\n"
        u"      section payloads containing the same byte value (all 0x00 or all 0xFF\n"
        u"      for instance) are ignored. Typically, such sections are stuffing and\n"
        u"      can be ignored that way.\n"
        u"\n"
        u"  -f\n"
        u"  --flush\n"
        u"      Flush output after each display.\n"
        u"\n"
        u"  --help\n"
        u"      Display this help text.\n"
        u"\n"
        u"  -i address:port\n"
        u"  --ip-udp address:port\n"
        u"      Send binary tables over UDP/IP to the specified destination.\n"
        u"      The 'address' specifies an IP address which can be either unicast\n"
        u"      or multicast. It can be also a host name that translates to an IP\n"
        u"      address. The 'port' specifies the destination UDP port.\n"
        u"\n"
        u"  --local-udp address\n"
        u"      With --ip-udp, when the destination is a multicast address, specify\n"
        u"      the IP address of the outgoing local interface. It can be also a host\n"
        u"      name that translates to a local address.\n"
        u"\n"
        u"  --log\n"
        u"      Short one-line log of each table instead of full table display.\n"
        u"\n"
        u"  --log-size value\n"
        u"      With option --log, specify how many bytes are displayed at the\n"
        u"      beginning of the table payload (the header is not displayed).\n"
        u"      The default is 8 bytes.\n"
        u"\n"
        u"  -x value\n"
        u"  --max-tables value\n"
        u"      Maximum number of tables to dump. Stop logging tables when this\n"
        u"      limit is reached.\n"
        u"\n"
        u"  -m\n"
        u"  --multiple-files\n"
        u"      Create multiple binary output files, one per section. A binary\n"
        u"      output file name must be specified (option -b or --binary-output).\n"
        u"      Assuming that the specified file name has the form 'base.ext',\n"
        u"      each file is created with the name 'base_pXXXX_tXX.ext' for\n"
        u"      short sections and 'base_pXXXX_tXX_eXXXX_vXX_sXX.ext' for long\n"
        u"      sections, where the XX specify the hexadecimal values of the\n"
        u"      PID, TID (table id), TIDext (table id extension), version and\n"
        u"      section index.\n"
        u"\n"
        u"  --negate-pid\n"
        u"      Negate the PID filter: specified PID's are excluded.\n"
        u"      Warning: this can be a dangerous option on complete transport\n"
        u"      streams since PID's not containing sections can be accidentally\n"
        u"      selected.\n"
        u"\n"
        u"  -n\n"
        u"  --negate-tid\n"
        u"      Negate the TID filter: specified TID's are excluded.\n"
        u"\n"
        u"  --negate-tid-ext\n"
        u"      Negate the TID extension filter: specified TID extensions are\n"
        u"      excluded.\n"
        u"\n"
        u"  --no-duplicate\n"
        u"      Do not report consecutive identical tables with a short section in the\n"
        u"      same PID. This can be useful for ECM's. This is the way to display new\n"
        u"      ECM's only. By default, tables with long sections are reported only when\n"
        u"      a new version is detected but tables with a short section are all reported.\n"
        u"\n"
        u"  --no-encapsulation\n"
        u"      With --ip-udp, send the tables as raw binary messages in UDP packets.\n"
        u"      By default, the tables are formatted into TLV messages.\n"
        u"\n"
        u"  -o filename\n"
        u"  --output-file filename\n"
        u"  --text-output filename\n"
        u"      Save the tables or sections in human-readable text format in the specified\n"
        u"      file. By default, when no output option is specified, text is produced on\n"
        u"      the standard output. If you need text formatting on the standard output in\n"
        u"      addition to other output like binary files or UPD/IP, explicitly specify\n"
        u"      this option with \"-\" as output file name.\n"
        u"\n"
        u"  --packet-index\n"
        u"      Display the index of the first and last TS packet of each displayed\n"
        u"      section or table.\n"
        u"\n"
        u"  -p value\n"
        u"  --pid value\n"
        u"      PID filter: select packets with this PID value,\n"
        u"      Several -p or --pid options may be specified.\n"
        u"      Without -p or --pid option, all PID's are used (this can be a\n"
        u"      dangerous option on complete transport streams since PID's not\n"
        u"      containing sections can be accidentally selected).\n"
        u"\n"
        u"  --psi-si\n"
        u"      Add all PID's containing PSI/SI tables, ie. PAT, CAT, PMT, NIT, SDT\n"
        u"      and BAT. Note that EIT, TDT and TOT are not included. Use --pid 18\n"
        u"      to get EIT and --pid 20 to get TDT and TOT.\n"
        u"\n"
        u"  -t value\n"
        u"  --tid value\n"
        u"      TID filter: select sections with this TID (table id) value.\n"
        u"      Several -t or --tid options may be specified.\n"
        u"      Without -t or --tid option, all tables are saved.\n"
        u"\n"
        u"  -e value\n"
        u"  --tid-ext value\n"
        u"      TID extension filter: select sections with this table id\n"
        u"      extension value (apply to long sections only).\n"
        u"      Several -e or --tid-ext options may be specified.\n"
        u"      Without -e or --tid-ext option, all tables are saved.\n"
        u"\n"
        u"  --time-stamp\n"
        u"      Display a time stamp (current local time) with each table.\n"
        u"\n"
        u"  --ttl value\n"
        u"      With --ip-udp, specifies the TTL (Time-To-Live) socket option.\n"
        u"      The actual option is either \"Unicast TTL\" or \"Multicast TTL\",\n"
        u"      depending on the destination address. Remember that the default\n"
        u"      Multicast TTL is 1 on most systems.\n"
        u"\n"
        u"  -v\n"
        u"  --verbose\n"
        u"      Produce verbose output.\n"
        u"\n"
        u"  --version\n"
        u"      Display the version number.\n"
        u"\n"
        u"  --xml-output filename\n"
        u"      Save the tables in XML format in the specified file. To output the XML\n"
        u"      text on the standard output, explicitly specify this option with \"-\"\n"
        u"      as output file name.\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesLoggerArgs::defineOptions(Args& args) const
{
    args.option(u"all-sections",        'a');
    args.option(u"binary-output",       'b', Args::STRING);
    args.option(u"diversified-payload", 'd');
    args.option(u"flush",               'f');
    args.option(u"ip-udp",              'i', Args::STRING);
    args.option(u"local-udp",            0,  Args::STRING);
    args.option(u"log",                  0);
    args.option(u"log-size",             0,  Args::UNSIGNED);
    args.option(u"max-tables",          'x', Args::POSITIVE);
    args.option(u"multiple-files",      'm');
    args.option(u"negate-pid",           0);
    args.option(u"negate-tid",          'n');
    args.option(u"negate-tid-ext",       0);
    args.option(u"no-duplicate",         0);
    args.option(u"no-encapsulation",     0);
    args.option(u"output-file",         'o', Args::STRING);
    args.option(u"packet-index",         0);
    args.option(u"pid",                 'p', Args::PIDVAL, 0, Args::UNLIMITED_COUNT);
    args.option(u"psi-si",               0);
    args.option(u"text-output",          0,  Args::STRING); // synonym for --output-file
    args.option(u"tid",                 't', Args::UINT8,  0, Args::UNLIMITED_COUNT);
    args.option(u"tid-ext",             'e', Args::UINT16, 0, Args::UNLIMITED_COUNT);
    args.option(u"time-stamp",           0);
    args.option(u"ttl",                  0,  Args::POSITIVE);
    args.option(u"xml-output",           0,  Args::STRING);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::TablesLoggerArgs::load(Args& args)
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
    flush = args.present(u"flush");
    udp_local = args.value(u"local-udp");
    udp_ttl = args.intValue(u"ttl", 0);
    all_sections = args.present(u"all-sections");
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

    if (add_pmt_pids || args.present(u"pid")) {
        args.getPIDSet(pid, u"pid"); // specific pids
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
}
