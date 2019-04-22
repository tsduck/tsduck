//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsswitchOptions.h"
#include "tsAsyncReport.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_INPUT_PACKETS   128
#define DEFAULT_MAX_OUTPUT_PACKETS  128
#define DEFAULT_BUFFERED_PACKETS    512
#define DEFAULT_RECEIVE_TIMEOUT    2000


//----------------------------------------------------------------------------
// Constructor from command line options
//----------------------------------------------------------------------------

ts::tsswitch::Options::Options(int argc, char *argv[]) :
    ArgsWithPlugins(1, UNLIMITED_COUNT, 0, 0, 0, 1),
    fastSwitch(false),
    delayedSwitch(false),
    terminate(false),
    monitor(false),
    logTimeStamp(false),
    logSynchronous(false),
    reusePort(false),
    firstInput(0),
    primaryInput(NPOS),
    cycleCount(1),
    logMaxBuffer(AsyncReport::MAX_LOG_MESSAGES),
    bufferedPackets(0),
    maxInputPackets(0),
    maxOutputPackets(0),
    sockBuffer(0),
    remoteServer(),
    allowedRemote(),
    receiveTimeout(0)
{
    setDescription(u"TS input source switch using remote control");

    setSyntax(u"[tsswitch-options] -I input-name [input-options] ... [-O output-name [output-options]]");

    option(u"allow", 'a', STRING);
    help(u"allow",
        u"Specify an IP address or host name which is allowed to send remote commands. "
        u"Several --allow options are allowed . By default, all remote commands are accepted.");

    option(u"buffer-packets", 'b', POSITIVE);
    help(u"buffer-packets",
         u"Specify the size in TS packets of each input plugin buffer. "
         u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" packets.");

    option(u"cycle", 'c', POSITIVE);
    help(u"cycle",
         u"Specify how many times to repeat the cycle through all input plugins in sequence. "
         u"By default, all input plugins are executed in sequence only once (--cycle 1). "
         u"The options --cycle, --infinite and --terminate are mutually exclusive.");

    option(u"delayed-switch", 'd');
    help(u"delayed-switch",
         u"Perform delayed input switching. When switching from one input plugin to another one, "
         u"the second plugin is started first. Packets from the first plugin continue to be "
         u"output while the second plugin is starting. Then, after the second plugin starts to "
         u"receive packets, the switch occurs: packets are now fetched from the second plugin. "
         u"Finally, after the switch, the first plugin is stopped.");

    option(u"fast-switch", 'f');
    help(u"fast-switch",
         u"Perform fast input switching. All input plugins are started at once and they "
         u"continuously receive packets in parallel. Packets are dropped, except for the "
         u"current input plugin. This option is typically used when all inputs are live "
         u"streams on distinct devices (not the same DVB tuner for instance).\n\n"
         u"By default, only one input plugin is started at a time. When switching, "
         u"the current input is first stopped and then the next one is started.");

    option(u"first-input", 0, UNSIGNED);
    help(u"first-input",
         u"Specify the index of the first input plugin to start. "
         u"By default, the first plugin (index 0) is used.");

    option(u"infinite", 'i');
    help(u"infinite", u"Infinitely repeat the cycle through all input plugins in sequence.");

    option(u"log-message-count", 0, POSITIVE);
    help(u"log-message-count",
         u"Specify the maximum number of buffered log messages. Log messages are "
         u"displayed asynchronously in a low priority thread. This value specifies "
         u"the maximum number of buffered log messages in memory, before being "
         u"displayed. When too many messages are logged in a short period of time, "
         u"while plugins use all CPU power, extra messages are dropped. Increase "
         u"this value if you think that too many messages are dropped. The default "
         u"is " + UString::Decimal(AsyncReport::MAX_LOG_MESSAGES) + u" messages.");

    option(u"max-input-packets", 0, POSITIVE);
    help(u"max-input-packets",
         u"Specify the maximum number of TS packets to read at a time. "
         u"This value may impact the switch response time. "
         u"The default is " + UString::Decimal(DEFAULT_MAX_INPUT_PACKETS) + u" packets. "
         u"The actual value is never more than half the --buffer-packets value.");

    option(u"max-output-packets", 0, POSITIVE);
    help(u"max-output-packets",
         u"Specify the maximum number of TS packets to write at a time. "
         u"The default is " + UString::Decimal(DEFAULT_MAX_OUTPUT_PACKETS) + u" packets.");

    option(u"monitor", 'm');
    help(u"monitor",
         u"Continuously monitor the system resources which are used by tsswitch. "
         u"This includes CPU load, virtual memory usage. Useful to verify the "
         u"stability of the application.");

    option(u"primary-input", 'p', UNSIGNED);
    help(u"primary-input",
         u"Specify the index of the input plugin which is considered as primary "
         u"or preferred. This input plugin is always started, never stopped, even "
         u"without --fast-switch. When no packet is received on this plugin, the "
         u"normal switching rules apply. However, as soon as packets are back on "
         u"the primary input, the reception is immediately switched back to it. "
         u"By default, there is no primary input, all input plugins are equal.");

    option(u"no-reuse-port");
    help(u"no-reuse-port",
         u"Disable the reuse port socket option for the remote control. "
         u"Do not use unless completely necessary.");

    option(u"receive-timeout", 0, UNSIGNED);
    help(u"receive-timeout",
         u"Specify a receive timeout in milliseconds. "
         u"When the current input plugin has received no packet within "
         u"this timeout, automatically switch to the next plugin. "
         u"By default, without --primary-input, there is no automatic switch "
         u"when the current input plugin is waiting for packets. With "
         u"--primary-input, the default is " + UString::Decimal(DEFAULT_RECEIVE_TIMEOUT) + u" ms.");

    option(u"remote", 'r', STRING);
    help(u"remote", u"[address:]port",
         u"Specify the local UDP port which is used to receive remote commands. "
         u"If an optional address is specified, it must be a local IP address of the system. "
         u"By default, there is no remote control.");

    option(u"synchronous-log", 's');
    help(u"synchronous-log",
         u"Each logged message is guaranteed to be displayed, synchronously, without "
         u"any loss of message. The downside is that a plugin thread may be blocked "
         u"for a short while when too many messages are logged. This option shall be "
         u"used when all log messages are needed and the source and destination are "
         u"not live streams (files for instance). This option is not recommended for "
         u"live streams, when the responsiveness of the application is more important "
         u"than the logged messages.");

    option(u"timed-log");
    help(u"timed-log", u"Each logged message contains a time stamp.");

    option(u"terminate", 't');
    help(u"terminate", u"Terminate execution when the current input plugin terminates.");

    option(u"udp-buffer-size", 0, UNSIGNED);
    help(u"udp-buffer-size",
         u"Specifies the UDP socket receive buffer size (socket option).");

    // Analyze the command.
    analyze(argc, argv);

    fastSwitch = present(u"fast-switch");
    delayedSwitch = present(u"delayed-switch");
    terminate = present(u"terminate");
    cycleCount = intValue<size_t>(u"cycle", present(u"infinite") ? 0 : 1);
    monitor = present(u"monitor");
    logTimeStamp = present(u"timed-log");
    logSynchronous = present(u"synchronous-log");
    logMaxBuffer = intValue<size_t>(u"log-message-count", AsyncReport::MAX_LOG_MESSAGES);
    bufferedPackets = intValue<size_t>(u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    maxInputPackets = std::min(intValue<size_t>(u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS), bufferedPackets / 2);
    maxOutputPackets = intValue<size_t>(u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    const UString remoteName(value(u"remote"));
    reusePort = !present(u"no-reuse-port");
    sockBuffer = intValue<size_t>(u"udp-buffer-size");
    firstInput = intValue<size_t>(u"first-input", 0);
    primaryInput = intValue<size_t>(u"primary-input", NPOS);
    receiveTimeout = intValue<MilliSecond>(u"receive-timeout", primaryInput >= inputs.size() ? 0 : DEFAULT_RECEIVE_TIMEOUT);

    if (firstInput >= inputs.size()) {
        error(u"invalid input index for --first-input %d", {firstInput});
    }

    if (primaryInput != NPOS && primaryInput >= inputs.size()) {
        error(u"invalid input index for --primary-input %d", {primaryInput});
    }

    if (present(u"cycle") + present(u"infinite") + present(u"terminate") > 1) {
        error(u"options --cycle, --infinite and --terminate are mutually exclusive");
    }

    if (fastSwitch && delayedSwitch) {
        error(u"options --delayed-switch and --fast-switch are mutually exclusive");
    }

    // Resolve remote control name.
    if (!remoteName.empty() && remoteServer.resolve(remoteName, *this) && !remoteServer.hasPort()) {
        error(u"missing UDP port number in --remote");
    }

    // Resolve all allowed remote.
    UStringVector remotes;
    getValues(remotes, u"allow");
    allowedRemote.clear();
    for (auto it = remotes.begin(); it != remotes.end(); ++it) {
        const IPAddress addr(*it, *this);
        if (addr.hasAddress()) {
            allowedRemote.insert(addr);
        }
    }

    // The default output is the standard output file.
    if (outputs.empty()) {
        outputs.push_back(PluginOptions(OUTPUT_PLUGIN, u"file"));
    }

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::tsswitch::Options::~Options()
{
}
