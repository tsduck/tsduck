//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInputSwitcherArgs.h"
#include "tsArgsWithPlugins.h"


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
    if (receive_timeout.count() <= 0 && primary_input != NPOS) {
        receive_timeout = DEFAULT_RECEIVE_TIMEOUT;
    }

    first_input = std::min(first_input, inputs.size() - 1);
    buffered_packets = std::max(buffered_packets, MIN_BUFFERED_PACKETS);
    max_input_packets = std::max(max_input_packets, MIN_INPUT_PACKETS);
    max_output_packets = std::max(max_output_packets, MIN_OUTPUT_PACKETS);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::InputSwitcherArgs::defineArgs(Args& args)
{
    remote_control.defineServerArgs(args);

    args.option(u"allow", 'a', Args::IPADDR);
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

    args.option(u"event-udp", 0, Args::IPSOCKADDR);
    args.help(u"event-udp",
              u"When a switch event occurs, send a short JSON description over UDP/IP to the specified destination. "
              u"This can be used to notify some external system of the event. "
              u"The 'address' specifies an IP address which can be either unicast or multicast. "
              u"It can be also a host name that translates to an IP address. "
              u"The 'port' specifies the destination UDP port.");

    args.option(u"event-local-address", 0, Args::IPADDR);
    args.help(u"event-local-address",
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

    args.option<cn::milliseconds>(u"receive-timeout");
    args.help(u"receive-timeout",
              u"Specify a receive timeout in milliseconds. "
              u"When the current input plugin has received no packet within "
              u"this timeout, automatically switch to the next plugin. "
              u"By default, without --primary-input, there is no automatic switch "
              u"when the current input plugin is waiting for packets. With "
              u"--primary-input, the default is " + UString::Chrono(DEFAULT_RECEIVE_TIMEOUT, true) + u".");

    args.option(u"remote", 'r', Args::IPSOCKADDR_OA);
    args.help(u"remote",
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
    bool success =
        remote_control.loadServerArgs(args, u"remote") &&
        remote_control.loadAllowedClients(args, u"allow");

    app_name = args.appName();
    fast_switch = args.present(u"fast-switch");
    delayed_switch = args.present(u"delayed-switch");
    terminate = args.present(u"terminate");
    args.getIntValue(cycle_count, u"cycle", args.present(u"infinite") ? 0 : 1);
    args.getIntValue(buffered_packets, u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    max_input_packets = std::min(args.intValue<size_t>(u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS), buffered_packets / 2);
    args.getIntValue(max_output_packets, u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    remote_control.reuse_port = !args.present(u"no-reuse-port");
    args.getIntValue(sock_buffer_size, u"udp-buffer-size");
    args.getIntValue(first_input, u"first-input", 0);
    args.getIntValue(primary_input, u"primary-input", NPOS);
    args.getChronoValue(receive_timeout, u"receive-timeout", primary_input >= inputs.size() ? cn::milliseconds::zero() : DEFAULT_RECEIVE_TIMEOUT);

    // Event reporting.
    args.getValue(event_command, u"event-command");
    args.getSocketValue(event_udp, u"event-udp");
    args.getIPValue(event_local_address, u"event-local-address");
    args.getIntValue(event_ttl, u"event-ttl", 0);
    args.getValue(event_user_data, u"event-user-data");

    // Check conflicting modes.
    if (args.present(u"cycle") + args.present(u"infinite") + args.present(u"terminate") > 1) {
        args.error(u"options --cycle, --infinite and --terminate are mutually exclusive");
        success = false;
    }
    if (fast_switch && delayed_switch) {
        args.error(u"options --delayed-switch and --fast-switch are mutually exclusive");
        success = false;
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
    if (first_input >= inputs.size()) {
        args.error(u"invalid input index for --first-input %d", first_input);
        success = false;
    }

    if (primary_input != NPOS && primary_input >= inputs.size()) {
        args.error(u"invalid input index for --primary-input %d", primary_input);
        success = false;
    }

    return success;
}
