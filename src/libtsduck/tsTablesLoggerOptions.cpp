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
//  Options for the class TablesLogger.
//
//----------------------------------------------------------------------------

#include "tsTablesLoggerOptions.h"
#include "tsException.h"
#include "tsHexa.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::TablesLoggerOptions::DEFAULT_LOG_SIZE;
#endif


//----------------------------------------------------------------------------
// Set help: application specific help + generic help
//----------------------------------------------------------------------------

void ts::TablesLoggerOptions::setHelp(const std::string& help)
{
    Args::setHelp(help +
        "Options:\n"
        "\n"
        "  -a\n"
        "  --all-sections\n"
        "      Display/save all sections, as they appear in the stream.\n"
        "      By default, collect complete tables, with all sections of\n"
        "      the tables grouped and ordered and collect each version\n"
        "      of a table only once.\n"
        "\n"
        "  -b filename\n"
        "  --binary-output filename\n"
        "      Binary output file name where the table sections are saved.\n"
        "      By default, the tables are interpreted and formatted as text.\n"
        "      See also option -m, --multiple-files.\n"
        "\n"
        "  -c\n"
        "  --c-style\n"
        "      Same as --raw-dump (no interpretation of section) but dump the\n"
        "      bytes in C-language style.\n"
        "\n"
        "  -d\n"
        "  --diversified-payload\n"
        "      Select only sections with \"diversified\" payload. This means that\n"
        "      section payloads containing the same byte value (all 0x00 or all 0xFF\n"
        "      for instance) are ignored. Typically, such sections are stuffing and\n"
        "      can be ignored that way.\n"
        "\n"
        "  -f\n"
        "  --flush\n"
        "      Flush output after each display.\n"
        "\n"
        "  -g value\n"
        "  --group value\n"
        "      When the table is an EMM, select only shared EMM with the specified\n"
        "      group number. Meaningful only if --safeaccess is specified.\n"
        "      Several -g or --group options may be specified.\n"
        "\n"
        "  --help\n"
        "      Display this help text.\n"
        "\n"
        "  -i address:port\n"
        "  --ip-udp address:port\n"
        "      Send binary tables over UDP/IP to the specified destination.\n"
        "      The 'address' specifies an IP address which can be either unicast\n"
        "      or multicast. It can be also a host name that translates to an IP\n"
        "      address. The 'port' specifies the destination UDP port.\n"
        "\n"
        "  --local-udp address\n"
        "      With --ip-udp, when the destination is a multicast address, specify\n"
        "      the IP address of the outgoing local interface. It can be also a host\n"
        "      name that translates to a local address.\n"
        "\n"
        "  --log\n"
        "      Short one-line log of each table instead of full table display.\n"
        "      When --safeaccess is specified and the table is an EMM, log only the\n"
        "      type and address of the EMM.\n"
        "\n"
        "  --log-size value\n"
        "      With option --log, specify how many bytes are displayed at the\n"
        "      beginning of the table payload (the header is not displayed).\n"
        "      The default is 8 bytes.\n"
        "\n"
        "  -x value\n"
        "  --max-tables value\n"
        "      Maximum number of tables to dump. Stop logging tables when this\n"
        "      limit is reached.\n"
        "\n"
        "  -m\n"
        "  --multiple-files\n"
        "      Create multiple binary output files, one per section. A binary\n"
        "      output file name must be specified (option -b or --binary-output).\n"
        "      Assuming that the specified file name has the form 'base.ext',\n"
        "      each file is created with the name 'base_pXXXX_tXX.ext' for\n"
        "      short sections and 'base_pXXXX_tXX_eXXXX_vXX_sXX.ext' for long\n"
        "      sections, where the XX specify the hexadecimal values of the\n"
        "      PID, TID (table id), TIDext (table id extension), version and\n"
        "      section index.\n"
        "\n"
        "  --negate-pid\n"
        "      Negate the PID filter: specified PID's are excluded.\n"
        "      Warning: this can be a dangerous option on complete transport\n"
        "      streams since PID's not containing sections can be accidentally\n"
        "      selected.\n"
        "\n"
        "  -n\n"
        "  --negate-tid\n"
        "      Negate the TID filter: specified TID's are excluded.\n"
        "\n"
        "  --negate-tid-ext\n"
        "      Negate the TID extension filter: specified TID extensions are\n"
        "      excluded.\n"
        "\n"
        "  -o filename\n"
        "  --output-file filename\n"
        "      File name for text output.\n"
        "\n"
        "  --packet-index\n"
        "      Display the index of the first and last TS packet of each displayed\n"
        "      section or table.\n"
        "\n"
        "  -p value\n"
        "  --pid value\n"
        "      PID filter: select packets with this PID value,\n"
        "      Several -p or --pid options may be specified.\n"
        "      Without -p or --pid option, all PID's are used (this can be a\n"
        "      dangerous option on complete transport streams since PID's not\n"
        "      containing sections can be accidentally selected).\n"
        "\n"
        "  --psi-si\n"
        "      Add all PID's containing PSI/SI tables, ie. PAT, CAT, PMT, NIT, SDT\n"
        "      and BAT. Note that EIT, TDT and TOT are not included. Use --pid 18\n"
        "      to get EIT and --pid 20 to get TDT and TOT.\n"
        "\n"
        "  -r\n"
        "  --raw-dump\n"
        "      Raw dump of section, no interpretation. With --ip-udp, the tables\n"
        "      are sent as raw binary messages in UDP packets (by default, they\n"
        "      are formatted into TLV messages).\n"
        "\n"
        "  -s\n"
        "  --safeaccess\n"
        "      Interpret ECM and EMM according to SafeAccess CAS.\n"
        "\n"
        "  -t value\n"
        "  --tid value\n"
        "      TID filter: select sections with this TID (table id) value.\n"
        "      Several -t or --tid options may be specified.\n"
        "      Without -t or --tid option, all tables are saved.\n"
        "\n"
        "  -e value\n"
        "  --tid-ext value\n"
        "      TID extension filter: select sections with this table id\n"
        "      extension value (apply to long sections only).\n"
        "      Several -e or --tid-ext options may be specified.\n"
        "      Without -e or --tid-ext option, all tables are saved.\n"
        "\n"
        "  --time-stamp\n"
        "      Display a time stamp (current local time) with each table.\n"
        "\n"
        "  --ttl value\n"
        "      With --ip-udp, specifies the TTL (Time-To-Live) socket option.\n"
        "      The actual option is either \"Unicast TTL\" or \"Multicast TTL\",\n"
        "      depending on the destination address. Remember that the default\n"
        "      Multicast TTL is 1 on most systems.\n"
        "\n"
        "  -u value\n"
        "  --ua value\n"
        "      When the table is an EMM, select only individual EMM with the specified\n"
        "      unique address. Meaningful only if --safeaccess is specified.\n"
        "      Several -u or --ua options may be specified.\n"
        "\n"
        "  -v\n"
        "  --verbose\n"
        "      Produce verbose output.\n"
        "\n"
        "  --version\n"
        "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesLoggerOptions::TablesLoggerOptions(const std::string& description,
                                             const std::string& syntax,
                                             const std::string& help,
                                             int flags) :
    Args(description, syntax, "", flags),
    mode(TEXT),
    destination(),
    multi_files(false),
    flush(false),
    udp_local(),
    udp_ttl(0),
    all_sections(false),
    max_tables(0),
    raw_dump(false),
    raw_flags(hexa::HEXA),
    time_stamp(false),
    packet_index(false),
    cas(CAS_OTHER),
    diversified(false),
    logger(false),
    log_size(DEFAULT_LOG_SIZE),
    negate_tid(false),
    negate_tidext(false),
    pid(),
    add_pmt_pids(false),
    tid(),
    tidext(),
    emm_group(),
    emm_ua()
{
    setHelp(help);

    option("all-sections",        'a');
    option("binary-output",       'b', STRING);
    option("c-style",             'c');
    option("diversified-payload", 'd');
    option("flush",               'f');
    option("group",               'g', INTEGER, 0, 1, 0, 0x00FFFFFF);
    option("ip-udp",              'i', STRING);
    option("local-udp",            0,  STRING);
    option("log",                  0);
    option("log-size",             0,  UNSIGNED);
    option("max-tables",          'x', POSITIVE);
    option("multiple-files",      'm');
    option("negate-pid",           0);
    option("negate-tid",          'n');
    option("negate-tid-ext",       0);
    option("output-file",         'o', STRING);
    option("packet-index",         0);
    option("pid",                 'p', PIDVAL, 0, UNLIMITED_COUNT);
    option("psi-si",               0);
    option("raw-dump",            'r');
    option("safeaccess",          's');
    option("tid",                 't', UINT8, 0, UNLIMITED_COUNT);
    option("tid-ext",             'e', UINT16, 0, UNLIMITED_COUNT);
    option("time-stamp",           0);
    option("ttl",                  0,  POSITIVE);
    option("ua",                  'u', POSITIVE, 0, UNLIMITED_COUNT);
    option("verbose",             'v');
}


//----------------------------------------------------------------------------
// Get option values (the public fields) after analysis of another
// ts::Args object defining the same options.
//----------------------------------------------------------------------------

void ts::TablesLoggerOptions::getOptions(Args& args)
{
    multi_files = args.present("multiple-files");
    flush = args.present("flush");
    udp_local = args.value("local-udp");
    udp_ttl = args.intValue("ttl", 0);
    all_sections = args.present("all-sections");
    max_tables = args.intValue<uint32_t>("max-tables", 0);
    time_stamp = args.present("time-stamp");
    packet_index = args.present("packet-index");
    cas = args.present("safeaccess") ? CAS_SAFEACCESS : CAS_OTHER;
    diversified = args.present("diversified-payload");
    logger = args.present("log");
    log_size = args.intValue<size_t>("log-size", DEFAULT_LOG_SIZE);
    negate_tid = args.present("negate-tid");
    negate_tidext = args.present("negate-tid-ext");

    if (args.present("verbose")) {
        args.setDebugLevel(Severity::Verbose);
        this->setDebugLevel(Severity::Verbose);
    }

    raw_dump = args.present("raw-dump");
    raw_flags = hexa::HEXA;
    if (args.present("c-style")) {
        raw_dump = true;
        raw_flags |= hexa::C_STYLE;
    }

    if (args.present("ip-udp")) {
        mode = UDP;
        destination = args.value("ip-udp");
    }
    else if (args.present("binary-output")) {
        mode = BINARY;
        destination = args.value("binary-output");
    }
    else {
        mode = TEXT;
        destination = args.value("output-file");
    }

    add_pmt_pids = args.present("psi-si");

    if (add_pmt_pids || args.present("pid")) {
        args.getPIDSet(pid, "pid"); // specific pids
        if (args.present("negate-pid")) {
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

    args.getIntValues(tid, "tid");
    args.getIntValues(tidext, "tid-ext");
    args.getIntValues(emm_group, "group");
    args.getIntValues(emm_ua, "ua");
}


//----------------------------------------------------------------------------
// Overriden analysis methods.
//----------------------------------------------------------------------------

bool ts::TablesLoggerOptions::analyze(int argc, char* argv[])
{
    bool ok = Args::analyze(argc, argv);
    if (ok) {
        getOptions(*this);
    }
    return ok;
}

bool ts::TablesLoggerOptions::analyze(const std::string& app_name, const StringVector& arguments)
{
    bool ok = Args::analyze(app_name, arguments);
    if (ok) {
        getOptions(*this);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Inaccessible operation. Throw exception when invoked through virtual table.
//----------------------------------------------------------------------------

bool ts::TablesLoggerOptions::analyze(const char* app_name, const char* arg1, ...)
{
    throw UnimplementedMethod("analyze with variable args not implemented for ts::TablesLoggerOptions");
}
