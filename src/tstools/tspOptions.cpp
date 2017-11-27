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
//  Transport stream processor command-line options
//
//----------------------------------------------------------------------------

#include "tspOptions.h"
#include "tsSysUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

#define DEF_BUFSIZE_MB           16  // mega-bytes
#define DEF_BITRATE_INTERVAL      5  // seconds
#define DEF_MAX_FLUSH_PKT     10000  // packets


//----------------------------------------------------------------------------
// Constructor from command line options
//----------------------------------------------------------------------------

ts::tsp::Options::Options(int argc, char *argv[]) :
    verbose(false),
    debug(0),
    timed_log(false),
    list_proc(false),
    monitor(false),
    ignore_jt(false),
    bufsize(0),
    max_flush_pkt(0),
    max_input_pkt(0),
    instuff_nullpkt(0),
    instuff_inpkt(0),
    bitrate(0),
    bitrate_adj(0),
    input(),
    output(),
    plugins()
{
    option(u"add-input-stuffing",       'a', Args::STRING);
    option(u"bitrate",                  'b', Args::POSITIVE);
    option(u"bitrate-adjust-interval",   0,  Args::POSITIVE);
    option(u"buffer-size-mb",            0,  Args::POSITIVE);
    option(u"debug",                    'd', Args::POSITIVE, 0, 1, 0, 0, true);
    option(u"ignore-joint-termination", 'i');
    option(u"list-processors",          'l');
    option(u"max-flushed-packets",       0,  Args::POSITIVE);
    option(u"max-input-packets",         0,  Args::POSITIVE);
    option(u"no-realtime-clock",         0); // was a temporary workaround, now ignored
    option(u"monitor",                  'm');
    option(u"timed-log",                't');
    option(u"verbose",                  'v');

#if defined(TS_WINDOWS)
#define HELP_SHLIB    "DLL"
#define HELP_SHLIBS   "DLL's"
#define HELP_SHLIBEXT ".dll"
#define HELP_SEP      "'\\'"
#define HELP_SEEMAN   ""
#else
#define HELP_SHLIB    "shared library"
#define HELP_SHLIBS   "shared libraries"
#define HELP_SHLIBEXT ".so"
#define HELP_SEP      "'/'"
#define HELP_SEEMAN   " See the man page of dlopen(3) for more details."
#endif

    setDescription("MPEG Transport Stream Processor: Receive a TS from a user-specified input\n"
                  u"plug-in, apply MPEG packet processing through several user-specified packet\n"
                  u"processor plug-in's and send the processed stream to a user-specified output\n"
                  u"plug-in. All input, processors and output plug-in's are " HELP_SHLIBS ".");

    setSyntax(" [tsp-options] \\\n"
              u"    [-I input-name [input-options]] \\\n"
              u"    [-P processor-name [processor-options]] ... \\\n"
              u"    [-O output-name [output-options]]");

    setHelp(u"All tsp-options must be placed on the command line before the input,\n"
            u"processors and output specifications. The tsp-options are:\n"
            u"\n"
            u"  -a nullpkt/inpkt\n"
            u"  --add-input-stuffing nullpkt/inpkt\n"
            u"      Specify that <nullpkt> null TS packets must be automatically inserted\n"
            u"      after every <inpkt> input TS packets. Both <nullpkt> and <inpkt> must\n"
            u"      be non-zero integer values. This option is useful to artificially\n"
            u"      increase the input bitrate by adding stuffing. Example: the option\n"
            u"      \"-a 14/24\" adds 14 null packets every 24 input packets, effectively\n"
            u"      turning a 24 Mb/s input stream (terrestrial) into a 38 Mb/s stream\n"
            u"      (satellite).\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specify the input bitrate, in bits/seconds. By default, the input\n"
            u"      bitrate is provided by the input plugin or by analysis of the PCR.\n"
            u"\n"
            u"  --bitrate-adjust-interval value\n"
            u"      Specify the interval in seconds between bitrate adjustments,\n"
            u"      ie. when the output bitrate is adjusted to the input one.\n"
            u"      The default is " TS_STRINGIFY(DEF_BITRATE_INTERVAL) " seconds.\n"
            u"      Some output processors ignore this setting. Typically, ASI\n"
            u"      or modulator devices use it, while file devices ignore it.\n"
            u"      This option is ignored if --bitrate is specified.\n"
            u"\n"
            u"  --buffer-size-mb value\n"
            u"      Specify the buffer size in mega-bytes. This is the size of\n"
            u"      the buffer between the input and output devices. The default\n"
            u"      is " TS_STRINGIFY(DEF_BUFSIZE_MB) " MB.\n"
            u"\n"
            u"  -d[N]\n"
            u"  --debug[=N]\n"
            u"      Produce debug output. Specify an optional debug level N.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --ignore-joint-termination\n"
            u"      Ignore all --joint-termination options in plugins.\n"
            u"      The idea behind \"joint termination\" is to terminate tsp when several\n"
            u"      plugins have jointly terminated their processing. Some plugins have\n"
            u"      a --joint-termination option. When set, the plugin executes until some\n"
            u"      plugin-specific condition. When all plugins with --joint-termination set\n"
            u"      have reached their termination condition, tsp terminates. The option\n"
            u"      --ignore-joint-termination disables the termination of tsp when all\n"
            u"      plugins have reached their joint termination condition.\n"
            u"\n"
            u"  -l\n"
            u"  --list-processors\n"
            u"      List all available processors.\n"
            u"\n"
            u"  --max-flushed-packets value\n"
            u"      Specify the maximum number of packets to be processed before flushing\n"
            u"      them to the next processor or the output. When the processing time\n"
            u"      is high and some packets are lost, try decreasing this value.\n"
            u"      The default is " TS_STRINGIFY(DEF_MAX_FLUSH_PKT) " packets.\n"
            u"\n"
            u"  --max-input-packets value\n"
            u"      Specify the maximum number of packets to be received at a time from\n"
            u"      the input plug-in. By default, tsp reads as many packets as it can,\n"
            u"      depending on the free space in the buffer.\n"
            u"\n"
            u"  -m\n"
            u"  --monitor\n"
            u"      Continuously monitor the system resources which are used by tsp.\n"
            u"      This includes CPU load, virtual memory usage. Useful to verify the\n"
            u"      stability of the application.\n"
            u"\n"
            u"  -t\n"
            u"  --timed-log\n"
            u"      Each logged message contains a time stamp.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n"
            u"\n"
            u"The following options activate the user-specified plug-in's.\n"
            u"\n"
            u"  -I name\n"
            u"  --input name\n"
            u"      Designate the " HELP_SHLIB " plug-in for packet input.\n"
            u"      By default, read packets from standard input.\n"
            u"\n"
            u"  -O name\n"
            u"  --output name\n"
            u"      Designate the " HELP_SHLIB " plug-in for packet output.\n"
            u"      By default, write packets to standard output.\n"
            u"\n"
            u"  -P name\n"
            u"  --processor name\n"
            u"      Designate a " HELP_SHLIB " plug-in for packet processing. Several\n"
            u"      packet processors are allowed. Each packet is successively processed\n"
            u"      by each processor, in the order of the command line. By default, there\n"
            u"      is no processor and the packets are directly passed from the input to\n"
            u"      the output.\n"
            u"\n"
            u"The specified <name> is used to locate a " HELP_SHLIB ". It can be designated\n"
            u"in a number of ways, in the following order:\n"
            u"\n"
            u"  . If the name contains a " HELP_SEP ", it is only interpreted as a file path for\n"
            u"    the " HELP_SHLIB ".\n"
            u"  . If not found, the file is searched into the all directories in environment\n"
            u"    variable " TS_PLUGINS_PATH " and in the same directory as the tsp executable\n"
            u"    file. In each directory, file named tsplugin_<name>" HELP_SHLIBEXT " is searched\n"
            u"    first, then the file <name>, with or without " HELP_SHLIBEXT ".\n"
            u"  . Finally, the standard system algorithm is applied to locate the " HELP_SHLIB "\n"
            u"    file." HELP_SEEMAN "\n"
            u"\n"
            u"Input-options, processor-options and output-options are specific to their\n"
            u"corresponding plug-in. Try \"tsp {-I|-O|-P} name --help\" to display the\n"
            u"help text for a specific plug-in.\n");

    // Locate the first processor option. All preceeding options are tsp
    // options and must be analyzed.

    PluginType plugin_type;
    int plugin_index = nextProcOpt(argc, argv, 0, plugin_type);

    // Analyze the tsp command, not including the plugin options

    analyze(plugin_index, argv);

    debug = present(u"debug") ? intValue("debug", 1) : 0;
    verbose = debug > 0 || present(u"verbose");
    setDebugLevel(debug > 0 ? debug : (verbose ? ts::Severity::Verbose : ts::Severity::Info));

    timed_log = present(u"timed-log");
    list_proc = present(u"list-processors");
    monitor = present(u"monitor");
    bufsize = 1024 * 1024 * intValue<size_t>(u"buffer-size-mb", DEF_BUFSIZE_MB);
    bitrate = intValue<BitRate>(u"bitrate", 0);
    bitrate_adj = MilliSecPerSec * intValue("bitrate-adjust-interval", DEF_BITRATE_INTERVAL);
    max_flush_pkt = intValue<size_t>(u"max-flushed-packets", DEF_MAX_FLUSH_PKT);
    max_input_pkt = intValue<size_t>(u"max-input-packets", 0);
    ignore_jt = present(u"ignore-joint-termination");

    if (present(u"add-input-stuffing")) {
        std::string stuff(value(u"add-input-stuffing"));
        std::string::size_type slash = stuff.find("/");
        bool valid = slash != std::string::npos &&
            ToInteger(instuff_nullpkt, stuff.substr(0, slash)) &&
            ToInteger(instuff_inpkt, stuff.substr(slash + 1));
        if (!valid) {
            error("invalid value for --add-input-stuffing, use \"nullpkt/inpkt\" format");
        }
    }

    // The first processor is always the input.
    // The default input is the standard input file.

    input.type = INPUT;
    input.name = "file";
    input.args.clear();

    // The default output is the standard output file.

    output.type = OUTPUT;
    output.name = "file";
    output.args.clear();

    // Locate all plugins

    plugins.reserve(argc);
    bool got_input = false;
    bool got_output = false;

    while (plugin_index < argc) {

        // Locate plugin description, seach for next plugin

        int start = plugin_index;
        PluginType type = plugin_type;
        plugin_index = nextProcOpt(argc, argv, plugin_index, plugin_type);
        PluginOptions* opt = 0;

        if (start >= argc - 1) {
            error(Format("missing plugin name for option %s", argv[start]));
            break;
        }

        switch (type) {
            case PROCESSOR:
                plugins.resize(plugins.size() + 1);
                opt = &plugins[plugins.size() - 1];
                break;
            case INPUT:
                if (got_input) {
                    error("do not specify more than one input plugin");
                }
                got_input = true;
                opt = &input;
                break;
            case OUTPUT:
                if (got_output) {
                    error("do not specify more than one output plugin");
                }
                got_output = true;
                opt = &output;
                break;
            default:
                // Should not get there
                assert(false);
        }

        opt->type = type;
        opt->name = argv[start+1];
        AssignContainer(opt->args, plugin_index - start - 2, argv + start + 2);
    }

    // Debug display

    if (debug >= 2) {
        display(std::cerr);
    }

    // Final checking

    exitOnError();
}


//----------------------------------------------------------------------------
// Search the next plugin option.
//----------------------------------------------------------------------------

int ts::tsp::Options::nextProcOpt(int argc, char *argv[], int index, PluginType& type)
{
    while (++index < argc) {
        const std::string arg(argv[index]);
        if (arg == "-I" || arg == "--input") {
            type = INPUT;
            return index;
        }
        if (arg == "-O" || arg == "--output") {
            type = OUTPUT;
            return index;
        }
        if (arg == "-P" || arg == "--processor") {
            type = PROCESSOR;
            return index;
        }
    }
    return std::min(argc, index);
}


//----------------------------------------------------------------------------
// Display the content of the object to a stream
//----------------------------------------------------------------------------

std::ostream& ts::tsp::Options::display(std::ostream& strm, int indent) const
{
    const std::string margin(indent, ' ');

    strm << margin << "* tsp options:" << std::endl
         << margin << "  --add-input-stuffing: " << Decimal(instuff_nullpkt)
         << "/" << Decimal(instuff_inpkt) << std::endl
         << margin << "  --bitrate: " << Decimal(bitrate) << " b/s" << std::endl
         << margin << "  --bitrate-adjust-interval: " << Decimal(bitrate_adj) << " milliseconds" << std::endl
         << margin << "  --buffer-size-mb: " << Decimal(bufsize) << " bytes" << std::endl
         << margin << "  --debug: " << debug << std::endl
         << margin << "  --list-processors: " << list_proc << std::endl
         << margin << "  --max-flushed-packets: " << Decimal(max_flush_pkt) << std::endl
         << margin << "  --max-input-packets: " << Decimal(max_input_pkt) << std::endl
         << margin << "  --monitor: " << monitor << std::endl
         << margin << "  --verbose: " << verbose << std::endl
         << margin << "  Number of packet processors: " << plugins.size() << std::endl
         << margin << "  Input plugin:" << std::endl;
    input.display(strm, indent + 4);
    for (size_t i = 0; i < plugins.size(); ++i) {
        strm << margin << "  Packet processor plugin " << (i+1) << ":" << std::endl;
        plugins[i].display(strm, indent + 4);
    }
    strm << margin << "  Output plugin:" << std::endl;
    output.display(strm, indent + 4);

    return strm;
}


//----------------------------------------------------------------------------
// Default constructor for plugin options.
//----------------------------------------------------------------------------

ts::tsp::Options::PluginOptions::PluginOptions() :
    type(PROCESSOR),
    name(),
    args()
{
}


//----------------------------------------------------------------------------
// Display the content of the object to a stream
//----------------------------------------------------------------------------

std::ostream& ts::tsp::Options::PluginOptions::display(std::ostream& strm, int indent) const
{
    const std::string margin(indent, ' ');

    strm << margin << "Name: " << name << std::endl
         << margin << "Type: " << PluginTypeName(type) << std::endl;
    for (size_t i = 0; i < args.size(); ++i) {
        strm << margin << "Arg[" << i << "]: \"" << args[i] << "\"" << std::endl;
    }

    return strm;
}


//----------------------------------------------------------------------------
// Displayable name of plugin type
//----------------------------------------------------------------------------

std::string ts::tsp::Options::PluginTypeName(PluginType type)
{
    switch (type) {
        case INPUT:     return "input";
        case OUTPUT:    return "output";
        case PROCESSOR: return "packet processor";
        default:        return Format("%d (invalid)", int(type));
    }
}
