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
//  Transport stream processor command-line options
//
//----------------------------------------------------------------------------

#include "tspOptions.h"
#include "tsAsyncReport.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

#define DEF_BUFSIZE_MB            16  // mega-bytes
#define DEF_BITRATE_INTERVAL       5  // seconds
#define DEF_MAX_FLUSH_PKT_OFL  10000  // packets
#define DEF_MAX_FLUSH_PKT_RT    1000  // packets
#define DEF_MAX_INPUT_PKT_OFL      0  // packets
#define DEF_MAX_INPUT_PKT_RT    1000  // packets

// Options for --list-processor.
const ts::Enumeration ts::tsp::Options::ListProcessorEnum({
    {u"all",    ts::PluginRepository::LIST_ALL},
    {u"input",  ts::PluginRepository::LIST_INPUT  | ts::PluginRepository::LIST_COMPACT},
    {u"output", ts::PluginRepository::LIST_OUTPUT | ts::PluginRepository::LIST_COMPACT},
    {u"packet", ts::PluginRepository::LIST_PACKET | ts::PluginRepository::LIST_COMPACT},
});


//----------------------------------------------------------------------------
// Constructor from command line options
//----------------------------------------------------------------------------

ts::tsp::Options::Options(int argc, char *argv[]) :
    ArgsWithPlugins(0, 1, 0, UNLIMITED_COUNT, 0, 1),
    timed_log(false),
    list_proc_flags(0),
    monitor(false),
    ignore_jt(false),
    sync_log(false),
    bufsize(0),
    log_msg_count(AsyncReport::MAX_LOG_MESSAGES),
    max_flush_pkt(0),
    max_input_pkt(0),
    instuff_nullpkt(0),
    instuff_inpkt(0),
    instuff_start(0),
    instuff_stop(0),
    bitrate(0),
    bitrate_adj(0),
    realtime(MAYBE)
{
    setDescription(u"MPEG transport stream processor using a chain of plugins");

    setSyntax(u"[tsp-options] \\\n"
              u"    [-I input-name [input-options]] \\\n"
              u"    [-P processor-name [processor-options]] ... \\\n"
              u"    [-O output-name [output-options]]");

    option(u"add-input-stuffing", 'a', STRING);
    help(u"add-input-stuffing", u"nullpkt/inpkt",
         u"Specify that <nullpkt> null TS packets must be automatically inserted "
         u"after every <inpkt> input TS packets. Both <nullpkt> and <inpkt> must "
         u"be non-zero integer values. This option is useful to artificially "
         u"increase the input bitrate by adding stuffing. Example: the option "
         u"\"-a 14/24\" adds 14 null packets every 24 input packets, effectively "
         u"turning a 24 Mb/s input stream (terrestrial) into a 38 Mb/s stream "
         u"(satellite).");

    option(u"add-start-stuffing", 0, UNSIGNED);
    help(u"add-start-stuffing", u"count",
         u"Specify that <count> null TS packets must be automatically inserted "
         u"at the start of the processing, before what comes from the input plugin.");

    option(u"add-stop-stuffing", 0, UNSIGNED);
    help(u"add-stop-stuffing", u"count",
         u"Specify that <count> null TS packets must be automatically inserted "
         u"at the end of the processing, after what comes from the input plugin.");

    option(u"bitrate", 'b', POSITIVE);
    help(u"bitrate",
         u"Specify the input bitrate, in bits/seconds. By default, the input "
         u"bitrate is provided by the input plugin or by analysis of the PCR.");

    option(u"bitrate-adjust-interval", 0, POSITIVE);
    help(u"bitrate-adjust-interval",
         u"Specify the interval in seconds between bitrate adjustments, "
         u"ie. when the output bitrate is adjusted to the input one. "
         u"The default is " TS_USTRINGIFY(DEF_BITRATE_INTERVAL) u" seconds. "
         u"Some output processors ignore this setting. Typically, ASI "
         u"or modulator devices use it, while file devices ignore it. "
         u"This option is ignored if --bitrate is specified. ");

    option(u"buffer-size-mb", 0, POSITIVE);
    help(u"buffer-size-mb",
         u"Specify the buffer size in mega-bytes. This is the size of "
         u"the buffer between the input and output devices. The default "
         u"is " TS_USTRINGIFY(DEF_BUFSIZE_MB) u" MB.");

    option(u"ignore-joint-termination", 'i');
    help(u"ignore-joint-termination",
         u"Ignore all --joint-termination options in plugins. "
         u"The idea behind \"joint termination\" is to terminate tsp when several "
         u"plugins have jointly terminated their processing. Some plugins have "
         u"a --joint-termination option. When set, the plugin executes until some "
         u"plugin-specific condition. When all plugins with --joint-termination set "
         u"have reached their termination condition, tsp terminates."
         u"\n\n"
         u"The option "
         u"--ignore-joint-termination disables the termination of tsp when all "
         u"plugins have reached their joint termination condition.");

    option(u"list-processors", 'l', ListProcessorEnum, 0, 1, true);
    help(u"list-processors", u"List all available processors.");

    option(u"log-message-count", 0, POSITIVE);
    help(u"log-message-count",
         u"Specify the maximum number of buffered log messages. Log messages are "
         u"displayed asynchronously in a low priority thread. This value specifies "
         u"the maximum number of buffered log messages in memory, before being "
         u"displayed. When too many messages are logged in a short period of time, "
         u"while plugins use all CPU power, extra messages are dropped. Increase "
         u"this value if you think that too many messages are dropped. The default "
         u"is " + UString::Decimal(AsyncReport::MAX_LOG_MESSAGES) + u" messages.");

    option(u"max-flushed-packets", 0, POSITIVE);
    help(u"max-flushed-packets",
         u"Specify the maximum number of packets to be processed before flushing "
         u"them to the next processor or the output. When the processing time "
         u"is high and some packets are lost, try decreasing this value. The default "
         u"is " + UString::Decimal(DEF_MAX_FLUSH_PKT_OFL) + u" packets in offline mode and " +
         UString::Decimal(DEF_MAX_FLUSH_PKT_RT) + u" in real-time mode.");

    option(u"max-input-packets", 0, POSITIVE);
    help(u"max-input-packets",
         u"Specify the maximum number of packets to be received at a time from "
         u"the input plug-in. By default, in offline mode, tsp reads as many packets "
         u"as it can, depending on the free space in the buffer. In real-time mode, "
         u"the default is " + UString::Decimal(DEF_MAX_INPUT_PKT_RT) + u" packets.");

    option(u"monitor", 'm');
    help(u"monitor",
         u"Continuously monitor the system resources which are used by tsp. "
         u"This includes CPU load, virtual memory usage. Useful to verify the "
         u"stability of the application.");

    option(u"realtime", 'r', TRISTATE, 0, 1, -255, 256, true);
    help(u"realtime",
         u"Specifies if tsp and all plugins should use default values for real-time "
         u"or offline processing. By default, if any plugin prefers real-time, the "
         u"real-time defaults are used. If no plugin prefers real-time, the offline "
         u"default are used. If -r or --realtime is used alone, the real-time defaults "
         u"are enforced. The explicit values 'no', 'false', 'off' are used to enforce "
         u"the offline defaults and the explicit values 'yes', 'true', 'on' are used "
         u"to enforce the real-time defaults.");

    option(u"synchronous-log", 's');
    help(u"synchronous-log",
         u"Each logged message is guaranteed to be displayed, synchronously, without "
         u"any loss of message. The downside is that a plugin thread may be blocked "
         u"for a short while when too many messages are logged. This option shall be "
         u"used when all log messages are needed and the source and destination are "
         u"not live streams (files for instance). This option is not recommended for "
         u"live streams, when the responsiveness of the application is more important "
         u"than the logged messages.");

    option(u"timed-log", 't');
    help(u"timed-log", u"Each logged message contains a time stamp.");

    // Analyze the command.
    analyze(argc, argv);

    timed_log = present(u"timed-log");
    list_proc_flags = present(u"list-processors") ? intValue<int>(u"list-processors", PluginRepository::LIST_ALL) : 0;
    monitor = present(u"monitor");
    sync_log = present(u"synchronous-log");
    bufsize = 1024 * 1024 * intValue<size_t>(u"buffer-size-mb", DEF_BUFSIZE_MB);
    bitrate = intValue<BitRate>(u"bitrate", 0);
    bitrate_adj = MilliSecPerSec * intValue(u"bitrate-adjust-interval", DEF_BITRATE_INTERVAL);
    max_flush_pkt = intValue<size_t>(u"max-flushed-packets", 0);
    max_input_pkt = intValue<size_t>(u"max-input-packets", 0);
    instuff_start = intValue<size_t>(u"add-start-stuffing", 0);
    instuff_stop = intValue<size_t>(u"add-stop-stuffing", 0);
    log_msg_count = intValue<size_t>(u"log-message-count", AsyncReport::MAX_LOG_MESSAGES);
    ignore_jt = present(u"ignore-joint-termination");
    realtime = tristateValue(u"realtime");

    if (present(u"add-input-stuffing") && !value(u"add-input-stuffing").scan(u"%d/%d", {&instuff_nullpkt, &instuff_inpkt})) {
        error(u"invalid value for --add-input-stuffing, use \"nullpkt/inpkt\" format");
    }

    // The default input is the standard input file.
    if (inputs.empty()) {
        inputs.push_back(PluginOptions(INPUT_PLUGIN, u"file"));
    }

    // The default output is the standard output file.
    if (outputs.empty()) {
        outputs.push_back(PluginOptions(OUTPUT_PLUGIN, u"file"));
    }

    // Debug display
    if (maxSeverity() >= 2) {
        display(std::cerr);
    }

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Apply default values to options which were not specified.
//----------------------------------------------------------------------------

void ts::tsp::Options::applyDefaults(bool rt)
{
    if (max_flush_pkt == 0) {
        max_flush_pkt = rt ? DEF_MAX_FLUSH_PKT_RT : DEF_MAX_FLUSH_PKT_OFL;
    }
    if (max_input_pkt == 0) {
        max_input_pkt = rt ? DEF_MAX_INPUT_PKT_RT: DEF_MAX_INPUT_PKT_OFL;
    }
    debug(u"realtime: %s, using --max-input-packets %'d --max-flushed-packets %'d", {UString::YesNo(rt), max_input_pkt, max_flush_pkt});
}


//----------------------------------------------------------------------------
// Display the content of the object to a stream
//----------------------------------------------------------------------------

std::ostream& ts::tsp::Options::display(std::ostream& strm, const UString& margin) const
{
    strm << margin << "* tsp options:" << std::endl
         << margin << "  --add-input-stuffing: " << UString::Decimal(instuff_nullpkt)
         << "/" << UString::Decimal(instuff_inpkt) << std::endl
         << margin << "  --bitrate: " << UString::Decimal(bitrate) << " b/s" << std::endl
         << margin << "  --bitrate-adjust-interval: " << UString::Decimal(bitrate_adj) << " milliseconds" << std::endl
         << margin << "  --buffer-size-mb: " << UString::Decimal(bufsize) << " bytes" << std::endl
         << margin << "  --debug: " << maxSeverity() << std::endl
         << margin << "  --list-processors: " << list_proc_flags << std::endl
         << margin << "  --max-flushed-packets: " << UString::Decimal(max_flush_pkt) << std::endl
         << margin << "  --max-input-packets: " << UString::Decimal(max_input_pkt) << std::endl
         << margin << "  --realtime: " << UString::TristateTrueFalse(realtime) << std::endl
         << margin << "  --monitor: " << monitor << std::endl
         << margin << "  --verbose: " << verbose() << std::endl
         << margin << "  Number of packet processors: " << plugins.size() << std::endl;
    display(inputs, u"Input plugin", strm, margin + u"  ");
    display(plugins, u"Packet processor plugin", strm, margin + u"  ");
    display(outputs, u"Output plugin", strm, margin + u"  ");
    return strm;
}

std::ostream& ts::tsp::Options::display(const ts::PluginOptionsVector& opts, const ts::UString& name, std::ostream& strm, const ts::UString& margin) const
{
    for (size_t i = 0; i < opts.size(); ++i) {
        strm << margin << name << " " << (i+1) << ":" << std::endl;
        opts[i].display(strm, margin + u"  ");
    }
    return strm;
}
