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

#include "tsInputSwitcherArgs.h"
#include "tsArgsWithPlugins.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::InputSwitcherArgs::DEFAULT_MAX_INPUT_PACKETS;
constexpr size_t ts::InputSwitcherArgs::MIN_INPUT_PACKETS;
constexpr size_t ts::InputSwitcherArgs::DEFAULT_MAX_OUTPUT_PACKETS;
constexpr size_t ts::InputSwitcherArgs::MIN_OUTPUT_PACKETS;
constexpr size_t ts::InputSwitcherArgs::DEFAULT_BUFFERED_PACKETS;
constexpr size_t ts::InputSwitcherArgs::MIN_BUFFERED_PACKETS;
constexpr ts::MilliSecond ts::InputSwitcherArgs::DEFAULT_RECEIVE_TIMEOUT;
#endif


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::InputSwitcherArgs::InputSwitcherArgs() :
    appName(),
    fastSwitch(false),
    delayedSwitch(false),
    terminate(false),
    reusePort(false),
    firstInput(0),
    primaryInput(NPOS),
    cycleCount(1),
    bufferedPackets(0),
    maxInputPackets(0),
    maxOutputPackets(0),
    eventCommand(),
    eventUDP(),
    eventLocalAddress(),
    eventTTL(0),
    eventUserData(),
    sockBuffer(0),
    remoteServer(),
    allowedRemote(),
    receiveTimeout(0),
    inputs(),
    output()
{
}


//----------------------------------------------------------------------------
// Enforce default or minimum values.
//----------------------------------------------------------------------------

void ts::InputSwitcherArgs::enforceDefaults()
{
    if (inputs.empty()) {
        // If no input plugin is used, used only standard input.
        inputs.push_back(PluginOptions(u"file"));
    }
    if (output.name.empty()) {
        output.set(u"file");
    }
    if (receiveTimeout <= 0 && primaryInput != NPOS) {
        receiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
    }

    firstInput = std::min(firstInput, inputs.size() - 1);
    bufferedPackets = std::max(bufferedPackets, MIN_BUFFERED_PACKETS);
    maxInputPackets = std::max(maxInputPackets, MIN_INPUT_PACKETS);
    maxOutputPackets = std::max(maxOutputPackets, MIN_OUTPUT_PACKETS);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::InputSwitcherArgs::defineArgs(Args& args)
{
    args.option(u"allow", 'a', Args::STRING);
    args.help(u"allow",
              u"Specify an IP address or host name which is allowed to send remote commands. "
              u"Several --allow options are allowed. By default, all remote commands are accepted.");

    args.option(u"buffer-packets", 'b', Args::POSITIVE);
    args.help(u"buffer-packets",
              u"Specify the size in TS packets of each input plugin buffer. "
              u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" packets.");

    args.option(u"cycle", 'c', Args::POSITIVE);
    args.help(u"cycle",
              u"Specify how many times to repeat the cycle through all input plugins in sequence. "
              u"By default, all input plugins are executed in sequence only once (--cycle 1). "
              u"The options --cycle, --infinite and --terminate are mutually exclusive.");

    args.option(u"delayed-switch", 'd');
    args.help(u"delayed-switch",
              u"Perform delayed input switching. When switching from one input plugin to another one, "
              u"the second plugin is started first. Packets from the first plugin continue to be "
              u"output while the second plugin is starting. Then, after the second plugin starts to "
              u"receive packets, the switch occurs: packets are now fetched from the second plugin. "
              u"Finally, after the switch, the first plugin is stopped.");

    args.option(u"event-command", 0, Args::STRING);
    args.help(u"event-command", u"'command'",
              u"When a switch event occurs, run this external shell command. "
              u"This can be used to notify some external system of the event. "
              u"The command receives additional parameters:\n\n"
              u"1. Event name, currently only \"newinput\" is defined.\n"
              u"2. The input index before the event.\n"
              u"3. The input index after the event.\n"
              u"4. Optional: the user data string from --event-user-data option.");

    args.option(u"event-udp", 0, Args::STRING);
    args.help(u"event-udp", u"address:port",
              u"When a switch event occurs, send a short JSON description over UDP/IP to the specified destination. "
              u"This can be used to notify some external system of the event. "
              u"The 'address' specifies an IP address which can be either unicast or multicast. "
              u"It can be also a host name that translates to an IP address. "
              u"The 'port' specifies the destination UDP port.");

    args.option(u"event-local-address", 0, Args::STRING);
    args.help(u"event-local-address", u"address",
              u"With --event-udp, when the destination is a multicast address, specify "
              u"the IP address of the outgoing local interface. It can be also a host "
              u"name that translates to a local address.");

    args.option(u"event-ttl", 0, Args::POSITIVE);
    args.help(u"event-ttl",
              u"With --event-udp, specifies the TTL (Time-To-Live) socket option. "
              u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
              u"depending on the destination address. Remember that the default "
              u"Multicast TTL is 1 on most systems.");

    args.option(u"event-user-data", 0, Args::STRING);
    args.help(u"event-user-data", u"'string'",
              u"A user-defined string which is passed to the --event-command as last "
              u"parameter and to the --event-udp message as \"user-data\" JSON value.");

    args.option(u"fast-switch", 'f');
    args.help(u"fast-switch",
              u"Perform fast input switching. All input plugins are started at once and they "
              u"continuously receive packets in parallel. Packets are dropped, except for the "
              u"current input plugin. This option is typically used when all inputs are live "
              u"streams on distinct devices (not the same DVB tuner for instance).\n\n"
              u"By default, only one input plugin is started at a time. When switching, "
              u"the current input is first stopped and then the next one is started.");

    args.option(u"first-input", 0, Args::UNSIGNED);
    args.help(u"first-input",
              u"Specify the index of the first input plugin to start. "
              u"By default, the first plugin (index 0) is used.");

    args.option(u"infinite", 'i');
    args.help(u"infinite", u"Infinitely repeat the cycle through all input plugins in sequence.");

    args.option(u"max-input-packets", 0, Args::POSITIVE);
    args.help(u"max-input-packets",
              u"Specify the maximum number of TS packets to read at a time. "
              u"This value may impact the switch response time. "
              u"The default is " + UString::Decimal(DEFAULT_MAX_INPUT_PACKETS) + u" packets. "
              u"The actual value is never more than half the --buffer-packets value.");

    args.option(u"max-output-packets", 0, Args::POSITIVE);
    args.help(u"max-output-packets",
              u"Specify the maximum number of TS packets to write at a time. "
              u"The default is " + UString::Decimal(DEFAULT_MAX_OUTPUT_PACKETS) + u" packets.");

    args.option(u"primary-input", 'p', Args::UNSIGNED);
    args.help(u"primary-input",
              u"Specify the index of the input plugin which is considered as primary "
              u"or preferred. This input plugin is always started, never stopped, even "
              u"without --fast-switch. When no packet is received on this plugin, the "
              u"normal switching rules apply. However, as soon as packets are back on "
              u"the primary input, the reception is immediately switched back to it. "
              u"By default, there is no primary input, all input plugins are equal.");

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"Disable the reuse port socket option for the remote control. "
              u"Do not use unless completely necessary.");

    args.option(u"receive-timeout", 0, Args::UNSIGNED);
    args.help(u"receive-timeout",
              u"Specify a receive timeout in milliseconds. "
              u"When the current input plugin has received no packet within "
              u"this timeout, automatically switch to the next plugin. "
              u"By default, without --primary-input, there is no automatic switch "
              u"when the current input plugin is waiting for packets. With "
              u"--primary-input, the default is " + UString::Decimal(DEFAULT_RECEIVE_TIMEOUT) + u" ms.");

    args.option(u"remote", 'r', Args::STRING);
    args.help(u"remote", u"[address:]port",
              u"Specify the local UDP port which is used to receive remote commands. "
              u"If an optional address is specified, it must be a local IP address of the system. "
              u"By default, there is no remote control.");

    args.option(u"terminate", 't');
    args.help(u"terminate", u"Terminate execution when the current input plugin terminates.");

    args.option(u"udp-buffer-size", 0, Args::UNSIGNED);
    args.help(u"udp-buffer-size",
              u"Specifies the UDP socket receive buffer size (socket option).");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::InputSwitcherArgs::loadArgs(DuckContext& duck, Args& args)
{
    appName = args.appName();
    fastSwitch = args.present(u"fast-switch");
    delayedSwitch = args.present(u"delayed-switch");
    terminate = args.present(u"terminate");
    args.getIntValue(cycleCount, u"cycle", args.present(u"infinite") ? 0 : 1);
    args.getIntValue(bufferedPackets, u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    maxInputPackets = std::min(args.intValue<size_t>(u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS), bufferedPackets / 2);
    args.getIntValue(maxOutputPackets, u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    const UString remoteName(args.value(u"remote"));
    reusePort = !args.present(u"no-reuse-port");
    args.getIntValue(sockBuffer, u"udp-buffer-size");
    args.getIntValue(firstInput, u"first-input", 0);
    args.getIntValue(primaryInput, u"primary-input", NPOS);
    args.getIntValue(receiveTimeout, u"receive-timeout", primaryInput >= inputs.size() ? 0 : DEFAULT_RECEIVE_TIMEOUT);

    // Event reporting.
    args.getValue(eventCommand, u"event-command");
    setEventUDP(args.value(u"event-udp"), args.value(u"event-local-address"), args);
    args.getIntValue(eventTTL, u"event-ttl", 0);
    args.getValue(eventUserData, u"event-user-data");

    // Check conflicting modes.
    if (args.present(u"cycle") + args.present(u"infinite") + args.present(u"terminate") > 1) {
        args.error(u"options --cycle, --infinite and --terminate are mutually exclusive");
    }
    if (fastSwitch && delayedSwitch) {
        args.error(u"options --delayed-switch and --fast-switch are mutually exclusive");
    }

    // Resolve network names. The resolve() method reports error and set the args error state.
    if (!remoteName.empty() && remoteServer.resolve(remoteName, args) && !remoteServer.hasPort()) {
        args.error(u"missing UDP port number in --remote");
    }

    // Resolve all allowed remote.
    UStringVector remotes;
    args.getValues(remotes, u"allow");
    allowedRemote.clear();
    for (const auto& it : remotes) {
        const IPv4Address addr(it, args);
        if (addr.hasAddress()) {
            allowedRemote.insert(addr);
        }
    }

    // Load all plugin descriptions. Default output is the standard output file.
    ArgsWithPlugins* pargs = dynamic_cast<ArgsWithPlugins*>(&args);
    if (pargs != nullptr) {
        pargs->getPlugins(inputs, PluginType::INPUT);
        pargs->getPlugin(output, PluginType::OUTPUT, u"file");
    }
    else {
        inputs.clear();
        output.set(u"file");
    }
    if (inputs.empty()) {
        // If no input plugin is used, used only standard input.
        inputs.push_back(PluginOptions(u"file"));
    }

    // Check validity of input indexes.
    if (firstInput >= inputs.size()) {
        args.error(u"invalid input index for --first-input %d", {firstInput});
    }

    if (primaryInput != NPOS && primaryInput >= inputs.size()) {
        args.error(u"invalid input index for --primary-input %d", {primaryInput});
    }

    return args.valid();
}


//----------------------------------------------------------------------------
// Set the UDP destination for event reporting using strings.
//----------------------------------------------------------------------------

bool ts::InputSwitcherArgs::setEventUDP(const UString& destination, const UString& local, Report& report)
{
    if (destination.empty()) {
        eventUDP.clear();
    }
    else if (!eventUDP.resolve(destination, report)) {
        return false;
    }
    else if (!eventUDP.hasAddress() || !eventUDP.hasPort()) {
        report.error(u"event reporting through UDP requires an IP address and a UDP port");
        return false;
    }

    if (local.empty()) {
        eventLocalAddress.clear();
    }
    else if (!eventLocalAddress.resolve(local, report)) {
        return false;
    }

    return true;
}
