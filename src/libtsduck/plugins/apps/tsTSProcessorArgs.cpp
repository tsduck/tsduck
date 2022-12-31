//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsTSProcessorArgs.h"
#include "tsPluginRepository.h"
#include "tsArgsWithPlugins.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::TSProcessorArgs::DEFAULT_BUFFER_SIZE;
constexpr size_t ts::TSProcessorArgs::MIN_BUFFER_SIZE;
#endif

#define DEF_BITRATE_INTERVAL               5  // seconds
#define DEF_INIT_BITRATE_PKT_INTERVAL   1000  // packets
#define DEF_MAX_FLUSH_PKT_OFL          10000  // packets
#define DEF_MAX_FLUSH_PKT_RT            1000  // packets
#define DEF_MAX_INPUT_PKT_OFL              0  // packets
#define DEF_MAX_INPUT_PKT_RT            1000  // packets
#define DEF_CONTROL_TIMEOUT             5000  // milliseconds


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSProcessorArgs::TSProcessorArgs() :
    app_name(),
    ignore_jt(false),
    log_plugin_index(false),
    ts_buffer_size(DEFAULT_BUFFER_SIZE),
    max_flush_pkt(0),
    max_input_pkt(0),
    max_output_pkt(NPOS), // unlimited
    init_input_pkt(0),
    instuff_nullpkt(0),
    instuff_inpkt(0),
    instuff_start(0),
    instuff_stop(0),
    fixed_bitrate(0),
    bitrate_adj(DEF_BITRATE_INTERVAL * MilliSecPerSec),
    init_bitrate_adj(DEF_INIT_BITRATE_PKT_INTERVAL),
    realtime(Tristate::Maybe),
    receive_timeout(0),
    final_wait(-1),
    control_port(0),
    control_local(),
    control_reuse(false),
    control_sources(),
    control_timeout(DEF_CONTROL_TIMEOUT),
    duck_args(),
    input(),
    plugins(),
    output()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSProcessorArgs::defineArgs(Args& args)
{
    args.option(u"add-input-stuffing", 'a', Args::STRING);
    args.help(u"add-input-stuffing", u"nullpkt/inpkt",
              u"Specify that <nullpkt> null TS packets must be automatically inserted "
              u"after every <inpkt> input TS packets. Both <nullpkt> and <inpkt> must "
              u"be non-zero integer values. This option is useful to artificially "
              u"increase the input bitrate by adding stuffing. Example: the option "
              u"\"-a 14/24\" adds 14 null packets every 24 input packets, effectively "
              u"turning a 24 Mb/s input stream (terrestrial) into a 38 Mb/s stream "
              u"(satellite).");

    args.option(u"add-start-stuffing", 0, Args::UNSIGNED);
    args.help(u"add-start-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically inserted "
              u"at the start of the processing, before what comes from the input plugin.");

    args.option(u"add-stop-stuffing", 0, Args::UNSIGNED);
    args.help(u"add-stop-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically inserted "
              u"at the end of the processing, after what comes from the input plugin.");

    args.option<BitRate>(u"bitrate", 'b');
    args.help(u"bitrate",
              u"Specify the input bitrate, in bits/seconds. By default, the input "
              u"bitrate is provided by the input plugin or by analysis of the PCR.");

    args.option(u"bitrate-adjust-interval", 0, Args::POSITIVE);
    args.help(u"bitrate-adjust-interval",
              u"Specify the interval in seconds between bitrate adjustments, "
              u"ie. when the output bitrate is adjusted to the input one. "
              u"The default is " TS_USTRINGIFY(DEF_BITRATE_INTERVAL) u" seconds. "
              u"Some output processors ignore this setting. Typically, ASI "
              u"or modulator devices use it, while file devices ignore it. "
              u"This option is ignored if --bitrate is specified. ");

    args.option(u"buffer-size-mb", 0, Args::POSITIVE, 0, 1, 0, 0, false, 6);
    args.help(u"buffer-size-mb",
              u"Specify the buffer size in mega-bytes. This is the size of "
              u"the buffer between the input and output devices. The default "
              u"is " + UString::Decimal(DEFAULT_BUFFER_SIZE / 1000000) + u" MB.");

    args.option(u"control-port", 0, Args::UINT16);
    args.help(u"control-port",
              u"Specify the TCP port on which tsp listens for control commands. "
              u"If unspecified, no control commands are expected.");

    args.option(u"control-local", 0, Args::STRING);
    args.help(u"control-local", u"address",
              u"Specify the IP address of the local interface on which to listen for control commands. "
              u"It can be also a host name that translates to a local address. "
              u"By default, listen on all local interfaces.");

    args.option(u"control-reuse-port");
    args.help(u"control-reuse-port",
              u"Set the 'reuse port' socket option on the control TCP server port. "
              u"This option is not enabled by default to avoid accidentally running "
              u"two tsp commands with the same control port.");

    args.option(u"control-source", 0, Args::STRING);
    args.help(u"control-source", u"address",
              u"Specify a remote IP address which is allowed to send control commands. "
              u"By default, as a security precaution, only the local host is allowed to connect. "
              u"Several --control-source options are allowed.");

    args.option(u"control-timeout", 0, Args::UNSIGNED);
    args.help(u"control-timeout", u"milliseconds",
              u"Specify the reception timeout in milliseconds for control commands. "
              u"The default timeout is " TS_STRINGIFY(DEF_CONTROL_TIMEOUT) u" ms.");

    args.option(u"final-wait", 0, Args::INT64);
    args.help(u"final-wait", u"milliseconds",
              u"Wait the specified number of milliseconds after the last input packet. "
              u"Zero means wait forever.");

    args.option(u"ignore-joint-termination", 'i');
    args.help(u"ignore-joint-termination",
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

    args.option(u"initial-input-packets", 0, Args::POSITIVE);
    args.help(u"initial-input-packets",
              u"Specify the number of packets to initially read in the buffer before starting the processing. "
              u"The initial load is used to evaluate the bitrate so that all subsequent plugins can have "
              u"a valid bitrate value from the beginning. "
              u"The default initial load is half the size of the global buffer.");

    args.option(u"log-plugin-index");
    args.help(u"log-plugin-index",
              u"In log messages, add the plugin index to the plugin name. "
              u"This can be useful if the same plugin is used several times "
              u"and all instances log many messages.");

    args.option(u"receive-timeout", 0, Args::POSITIVE);
    args.help(u"receive-timeout", u"milliseconds",
              u"Specify a timeout in milliseconds for all input operations. "
              u"Equivalent to the same --receive-timeout options in some plugins. "
              u"By default, there is no input timeout.");

    args.option(u"max-flushed-packets", 0, Args::POSITIVE);
    args.help(u"max-flushed-packets",
              u"Specify the maximum number of packets to be processed before flushing "
              u"them to the next processor or the output. When the processing time "
              u"is high and some packets are lost, try decreasing this value. The default "
              u"is " + UString::Decimal(DEF_MAX_FLUSH_PKT_OFL) + u" packets in offline mode and " +
              UString::Decimal(DEF_MAX_FLUSH_PKT_RT) + u" in real-time mode.");

    args.option(u"max-input-packets", 0, Args::POSITIVE);
    args.help(u"max-input-packets",
              u"Specify the maximum number of packets to be received at a time from "
              u"the input plug-in. By default, in offline mode, tsp reads as many packets "
              u"as it can, depending on the free space in the buffer. In real-time mode, "
              u"the default is " + UString::Decimal(DEF_MAX_INPUT_PKT_RT) + u" packets.");

    args.option(u"max-output-packets", 0, Args::POSITIVE);
    args.help(u"max-output-packets",
              u"Specify the maximum number of packets to be sent at a time by the output plugin. "
              u"By default, tsp sends as many packets as available. "
              u"This option is useful only when an output plugin or device has problems with large output requests. "
              u"This option forces multiple smaller send operations.");

    args.option(u"realtime", 'r', Args::TRISTATE, 0, 1, -255, 256, true);
    args.help(u"realtime",
              u"Specifies if tsp and all plugins should use default values for real-time "
              u"or offline processing. By default, if any plugin prefers real-time, the "
              u"real-time defaults are used. If no plugin prefers real-time, the offline "
              u"default are used. If -r or --realtime is used alone, the real-time defaults "
              u"are enforced. The explicit values 'no', 'false', 'off' are used to enforce "
              u"the offline defaults and the explicit values 'yes', 'true', 'on' are used "
              u"to enforce the real-time defaults.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSProcessorArgs::loadArgs(DuckContext& duck, Args& args)
{
    app_name = args.appName();
    log_plugin_index = args.present(u"log-plugin-index");
    ts_buffer_size = args.intValue<size_t>(u"buffer-size-mb", DEFAULT_BUFFER_SIZE);
    args.getValue(fixed_bitrate, u"bitrate", 0);
    bitrate_adj = MilliSecPerSec * args.intValue(u"bitrate-adjust-interval", DEF_BITRATE_INTERVAL);
    args.getIntValue(max_flush_pkt, u"max-flushed-packets", 0);
    args.getIntValue(max_input_pkt, u"max-input-packets", 0);
    args.getIntValue(max_output_pkt, u"max-output-packets", NPOS); // unlimited by default
    args.getIntValue(init_input_pkt, u"initial-input-packets", 0);
    args.getIntValue(instuff_start, u"add-start-stuffing", 0);
    args.getIntValue(instuff_stop, u"add-stop-stuffing", 0);
    ignore_jt = args.present(u"ignore-joint-termination");
    args.getTristateValue(realtime, u"realtime");
    args.getIntValue(receive_timeout, u"receive-timeout", 0);
    args.getIntValue(final_wait, u"final-wait", -1);
    args.getIntValue(control_port, u"control-port", 0);
    args.getIntValue(control_timeout, u"control-timeout", DEF_CONTROL_TIMEOUT);
    control_reuse = args.present(u"control-reuse-port");

    // Convert MB in MiB for buffer size for compatibility with original versions.
    ts_buffer_size = size_t((uint64_t(ts_buffer_size) * 1024 * 1024) / 1000000);

    // Get and resolve optional local address.
    if (!args.present(u"control-local")) {
        control_local.clear();
    }
    else {
        control_local.resolve(args.value(u"control-local"), args);
    }

    // Get and resolve optional allowed remote addresses.
    control_sources.clear();
    if (!args.present(u"control-source")) {
        // By default, the local host is the only allowed address.
        control_sources.push_back(IPv4Address::LocalHost);
    }
    else {
        for (size_t i = 0; i < args.count(u"control-source"); ++i) {
            IPv4Address addr;
            if (addr.resolve(args.value(u"control-source", u"", i), args)) {
                control_sources.push_back(addr);
            }
        }
    }

    // Decode --add-input-stuffing nullpkt/inpkt.
    instuff_nullpkt = instuff_inpkt = 0;
    if (args.present(u"add-input-stuffing") && !args.value(u"add-input-stuffing").scan(u"%d/%d", {&instuff_nullpkt, &instuff_inpkt})) {
        args.error(u"invalid value for --add-input-stuffing, use \"nullpkt/inpkt\" format");
    }

    // Load all plugin descriptions.
    // The default input and output are the standard input and output files.
    ArgsWithPlugins* pargs = dynamic_cast<ArgsWithPlugins*>(&args);
    if (pargs != nullptr) {
        pargs->getPlugin(input, PluginType::INPUT, u"file");
        pargs->getPlugin(output, PluginType::OUTPUT, u"file");
        pargs->getPlugins(plugins, PluginType::PROCESSOR);
    }
    else {
        input.set(u"file");
        output.set(u"file");
        plugins.clear();
    }

    // Get default options for TSDuck contexts in each plugin.
    duck.saveArgs(duck_args);

    return args.valid();
}


//----------------------------------------------------------------------------
// Apply default values to options which were not specified.
//----------------------------------------------------------------------------

void ts::TSProcessorArgs::applyDefaults(bool rt)
{
    if (max_flush_pkt == 0) {
        max_flush_pkt = rt ? DEF_MAX_FLUSH_PKT_RT : DEF_MAX_FLUSH_PKT_OFL;
    }
    if (max_input_pkt == 0) {
        max_input_pkt = rt ? DEF_MAX_INPUT_PKT_RT: DEF_MAX_INPUT_PKT_OFL;
    }
}
