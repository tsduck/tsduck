//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUDPReceiver.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::UDPReceiver::defineArgs(ts::Args& args, bool with_short_options, bool destination_is_parameter, bool multiple_receivers)
{
    // [[source@]address:]port can be either a parameter or an option.
    _dest_is_parameter = destination_is_parameter;
    const UChar dest_short = _dest_is_parameter || !with_short_options ? 0 : 'i';
    const size_t dest_min = _dest_is_parameter ? 1 : 0;

    // [[source@]address:]port can be specified multiple times.
    const size_t max_count = multiple_receivers ? Args::UNLIMITED_COUNT : 1;
    const UChar* const dest_display = _dest_is_parameter ? u"[address:]port parameters" : u"--ip-udp options";
    UString help;

    help = u"The [address:]port describes the destination of UDP packets to receive. "
           u"The 'port' part is mandatory and specifies the UDP port to listen on. "
           u"The 'address' part is optional. It specifies an IP multicast address to listen on. "
           u"It can be also a host name that translates to a multicast address. "
           u"An optional source address can be specified as 'source@address:port' in the case of SSM.";
    if (multiple_receivers) {
        help.format(u"\nSeveral %s can be specified to receive multiple UDP streams. "
                    u"If distinct receivers use the same port, this may work or not, depending on the operating system.",
                    {dest_display});
    }
    args.option(destinationOptionName(), dest_short, Args::STRING, dest_min, max_count);
    args.help(destinationOptionName(), u"[address:]port", help);

    args.option(u"buffer-size", with_short_options ? 'b' : 0, Args::UNSIGNED);
    args.help(u"buffer-size", u"Specify the UDP socket receive buffer size in bytes (socket option).");

    args.option(u"default-interface");
    args.help(u"default-interface",
              u"Let the system find the appropriate local interface on which to listen. "
              u"By default, listen on all local interfaces.");

    args.option(u"disable-multicast-loop");
    args.help(u"disable-multicast-loop",
              u"Disable multicast loopback. By default, incoming multicast packets are looped back on local interfaces, "
              u"if an application sends packets to the same group from the same system. This option disables this.\n"
              u"Warning: On input sockets, this option is effective only on Windows systems. "
              u"On Unix systems (Linux, macOS, BSD), this option applies only to output sockets.");

    args.option(u"first-source", with_short_options ? 'f' : 0);
    args.help(u"first-source",
              u"Filter UDP packets based on the source address. Use the sender address of "
              u"the first received packet as only allowed source. This option is useful "
              u"when several sources send packets to the same destination address and port. "
              u"Accepting all packets could result in a corrupted stream and only one "
              u"sender shall be accepted. To allow a more precise selection of the sender, "
              u"use option --source. Options --first-source and --source are mutually "
              u"exclusive.");

    help = u"Specify the IP address of the local interface on which to listen. "
           u"It can be also a host name that translates to a local address. "
           u"By default, listen on all local interfaces.";
    if (multiple_receivers) {
        help.format(u"\nIf several %s are specified, several --local-address options can be specified, "
                    u"one for each receiver, in the same order. It there are less --local-address "
                    u"options than receivers, the last --local-address applies for all remaining receivers.",
                    {dest_display});
    }
    args.option(u"local-address", with_short_options ? 'l' : 0, Args::IPADDR, 0, max_count);
    args.help(u"local-address", help);

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"Disable the reuse port socket option. Do not use unless completely necessary.");

    args.option(u"reuse-port", with_short_options ? 'r' : 0);
    args.help(u"reuse-port",
              u"Set the reuse port socket option. This is now enabled by default, the option "
              u"is present for legacy only.");

    args.option(u"receive-timeout", 0, Args::UNSIGNED);
    args.help(u"receive-timeout",
              u"Specify the UDP reception timeout in milliseconds. "
              u"This timeout applies to each receive operation, individually. "
              u"By default, receive operations wait for data, possibly forever.");

    help = u"Filter UDP packets based on the specified source address. This option is "
           u"useful when several sources send packets to the same destination address "
           u"and port. Accepting all packets could result in a corrupted stream and "
           u"only one sender shall be accepted. Options --first-source and --source "
           u"are mutually exclusive.";
    if (multiple_receivers) {
        help.format(u"\nIf several %s are specified, several --source options can be specified, "
                    u"one for each receiver, in the same order. It there are less --source "
                    u"options than receivers, the last --source applies for all remaining receivers.",
                    {dest_display});
    }
    args.option(u"source", with_short_options ? 's' : 0, Args::STRING, 0, max_count);
    args.help(u"source", u"address[:port]", help);

    args.option(u"ssm");
    args.help(u"ssm",
              u"Force the usage of Source-Specific Multicast (SSM) using the source which "
              u"is specified by the option --source. The --ssm option is implicit when the "
              u"syntax 'source@address:port' is used.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::UDPReceiver::loadArgs(DuckContext& duck, Args& args, size_t index)
{
    // Get destination address.
    _receiver_count = args.count(destinationOptionName());
    _receiver_index = index;
    UString destination(args.value(destinationOptionName(), u"", _receiver_index));
    _receiver_specified = !destination.empty();

    // When --ip-udp is specified as an option, the presence of a UDP received is optional.
    // Option UDP-related parameters are ignored when not specified.
    if (!_dest_is_parameter && !_receiver_specified) {
        return true;
    }

    // General UDP options.
    _reuse_port = !args.present(u"no-reuse-port");
    _default_interface = args.present(u"default-interface");
    _use_ssm = args.present(u"ssm");
    _use_first_source = args.present(u"first-source");
    _mc_loopback = !args.present(u"disable-multicast-loop");
    args.getIntValue(_recv_bufsize, u"buffer-size", 0);
    args.getIntValue(_recv_timeout, u"receive-timeout", _recv_timeout); // preserve previous value

    // Check the presence of the '@' indicating a source address.
    const size_t sep = destination.find(u'@');
    _use_source.clear();
    if (sep != NPOS) {
        // Resolve source address.
        if (!_use_source.resolve(destination.substr(0, sep), args)) {
            return false;
        }
        // Force SSM.
        _use_ssm = true;
        // Remove the source from the string.
        destination.erase(0, sep + 1);
    }

    // Resolve destination address.
    if (!_dest_addr.resolve(destination, args)) {
        return false;
    }

    // If a destination address is specified, it must be a multicast address.
    if (_dest_addr.hasAddress() && !_dest_addr.isMulticast()) {
        args.error(u"address %s is not multicast", {_dest_addr});
        return false;
    }

    // In case of SSM, it should be in the SSM range, but let it a warning only.
    if (_use_ssm && !_dest_addr.hasAddress()) {
        args.error(u"multicast group address is missing with SSM");
        return false;
    }
    if (_use_ssm && !_dest_addr.isSSM()) {
        args.warning(u"address %s is not an SSM address", {_dest_addr});
    }
    if (_use_ssm && _use_first_source) {
        args.error(u"SSM and --first-source are mutually exclusive");
        return false;
    }

    // The destination port is mandatory
    if (!_dest_addr.hasPort()) {
        args.error(u"no UDP port specified in %s", {destination});
        return false;
    }

    // Get and resolve optional local address.
    const size_t laddr_count = args.count(u"local-address");
    if (laddr_count > _receiver_count) {
        args.error(u"too many --local-address options");
        return false;
    }
    if (laddr_count == 0) {
        _local_address.clear();
    }
    else {
        args.getIPValue(_local_address, u"local-address", IPv4Address(), std::min(_receiver_index, laddr_count - 1));
    }

    // Either specify a local address or let the system decide, but not both.
    if (_default_interface && _local_address.hasAddress()) {
        args.error(u"--default-interface and --local-address are mutually exclusive");
        return false;
    }

    // Translate optional source address.
    UString source;
    const size_t source_count = args.count(u"source");
    if (source_count > _receiver_count) {
        args.error(u"too many --source options");
        return false;
    }
    // If _use_source is already set, it comes from source@destination SSM format.
    if (source_count > 0 && (!_use_source.hasAddress() || _receiver_index < source_count)) {
        args.getValue(source, u"source", u"", std::min(_receiver_index, source_count - 1));
    }
    if (_use_source.hasAddress() && _receiver_index < source_count) {
        args.error(u"SSM source address specified twice");
        return false;
    }
    if (source.empty()) {
        // No --source specified, no additional check.
    }
    else if (!_use_source.resolve(source, args)) {
        return false;
    }
    else if (!_use_source.hasAddress()) {
        // If source is specified, the port is optional but the address is mandatory.
        args.error(u"missing IP address in --source %s", {source});
        return false;
    }
    else if (_use_first_source) {
        args.error(u"--first-source and --source are mutually exclusive");
        return false;
    }
    if (_use_ssm && !_use_source.hasAddress()) {
        args.error(u"missing source address with --ssm");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Set reception timeout.
//----------------------------------------------------------------------------

void ts::UDPReceiver::setReceiveTimeoutArg(MilliSecond timeout)
{
    if (timeout > 0) {
        _recv_timeout = timeout;
    }
}


//----------------------------------------------------------------------------
// Set application-specified parameters to receive unicast traffic.
//----------------------------------------------------------------------------

void ts::UDPReceiver::setParameters(const IPv4SocketAddress& localAddress, bool reusePort, size_t bufferSize)
{
    _receiver_specified = true;
    _use_ssm = false;
    _dest_addr.clear();
    _dest_addr.setPort(localAddress.port());
    _local_address = localAddress;
    _reuse_port = reusePort;
    _recv_bufsize = bufferSize;
}


//----------------------------------------------------------------------------
// Open the socket. Override UDPSocket::open().
//----------------------------------------------------------------------------

bool ts::UDPReceiver::open(ts::Report& report)
{
    // Check if UDP parameters were specified.
    if (!_receiver_specified) {
        report.error(u"no UDP receiver address specified");
        return false;
    }

    // Clear collection of source address information.
    _first_source.clear();
    _sources.clear();

    // The local socket address to bind is the optional local IP address and the destination port.
    // Except on Linux, macOS and probably most Unix, when listening to a multicast group.
    // In that case, we bind to the multicast group, not the local interface.
    // Note that if _dest_addr has an address, it is a multicast one (checked in load()).
    IPv4SocketAddress local_addr(
#if defined(TS_UNIX)
        _dest_addr.hasAddress() ? _dest_addr.address() : _local_address,
#else
        _local_address,
#endif
        _dest_addr.port());

    // Create UDP socket from the superclass.
    // Note: On Windows, bind must be done *before* joining multicast groups.
    bool ok =
        UDPSocket::open(report) &&
        reusePort(_reuse_port, report) &&
        setReceiveTimestamps(_recv_timestamps, report) &&
        setMulticastLoop(_mc_loopback, report) &&
        (_recv_bufsize <= 0 || setReceiveBufferSize(_recv_bufsize, report)) &&
        (_recv_timeout < 0 || setReceiveTimeout(_recv_timeout, report)) &&
        bind(local_addr, report);

    // Optional SSM source address.
    IPv4Address ssm_source;
    if (_use_ssm) {
        ssm_source = _use_source;
    }

    // Join multicast group.
    if (ok && _dest_addr.hasAddress()) {
        if (_default_interface) {
            ok = addMembershipDefault(_dest_addr, ssm_source, report);
        }
        else if (_local_address.hasAddress()) {
            ok = addMembership(_dest_addr, _local_address, ssm_source, report);
        }
        else {
            // By default, listen on all interfaces.
            ok = addMembershipAll(_dest_addr, ssm_source, report);
        }
    }

    if (!ok) {
        close(report);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Receive a message. Override UDPSocket::receive().
//----------------------------------------------------------------------------

bool ts::UDPReceiver::receive(void* data,
                              size_t max_size,
                              size_t& ret_size,
                              ts::IPv4SocketAddress& sender,
                              ts::IPv4SocketAddress& destination,
                              const ts::AbortInterface* abort,
                              ts::Report& report,
                              MicroSecond* timestamp)
{
    // Loop on packet reception until one matching filtering criteria is found.
    for (;;) {

        // Wait for a UDP message from the superclass.
        if (!UDPSocket::receive(data, max_size, ret_size, sender, destination, abort, report, timestamp)) {
            return false;
        }

        // Debug (level 2) message for each message.
        if (report.maxSeverity() >= 2) {
            // Prior report level checking to avoid evaluating parameters when not necessary.
            report.log(2, u"received UDP packet, source: %s, destination: %s, timestamp: %'d", {sender, destination, timestamp != nullptr ? *timestamp : -1});
        }

        // Check the destination address to exclude packets from other streams.
        // When several multicast streams use the same destination port and several
        // applications on the same system listen to these distinct streams,
        // the multicast MAC address management is such that any socket which
        // is bound to the common port will receive the traffic for all streams.
        // This is why we need to check the destination address and exclude
        // packets which are not from the intended stream.
        //
        // We accept a packet in any of:
        // 1) Actual packet destination is unknown. Probably, the system cannot
        //    report the destination address.
        // 2) We listen to a multicast address and the actual destination is the same.
        // 3) If we listen to unicast traffic and the actual destination is unicast.
        //    In that case, unicast is by definition sent to us.

        if (destination.hasAddress() && ((_dest_addr.hasAddress() && destination != _dest_addr) || (!_dest_addr.hasAddress() && destination.isMulticast()))) {
            // This is a spurious packet.
            if (report.maxSeverity() >= Severity::Debug) {
                // Prior report level checking to avoid evaluating parameters when not necessary.
                report.debug(u"rejecting packet, destination: %s, expecting: %s", {destination, _dest_addr});
            }
            continue;
        }

        // Keep track of the first sender address.
        if (!_first_source.hasAddress()) {
            // First packet, keep address of the sender.
            _first_source = sender;
            _sources.insert(sender);

            // With option --first-source, use this one to filter packets.
            if (_use_first_source) {
                assert(!_use_source.hasAddress());
                _use_source = sender;
                report.verbose(u"now filtering on source address %s", {sender});
            }
        }

        // Keep track of senders (sources) to detect or filter multiple sources.
        if (_sources.count(sender) == 0) {
            // Detected an additional source, warn the user that distinct streams are potentially mixed.
            // If no source filtering is applied, this is a warning since this may affect the resulting stream.
            // With source filtering, this is just an informational verbose-level message.
            const int level = _use_source.hasAddress() ? Severity::Verbose : Severity::Warning;
            if (_sources.size() == 1) {
                report.log(level, u"detected multiple sources for the same destination %s with potentially distinct streams", {destination});
                report.log(level, u"detected source: %s", {_first_source});
            }
            report.log(level, u"detected source: %s", {sender});
            _sources.insert(sender);
        }

        // Filter packets based on source address if requested.
        if (!sender.match(_use_source)) {
            // Not the expected source, this is a spurious packet.
            if (report.maxSeverity() >= Severity::Debug) {
                // Prior report level checking to avoid evaluating parameters when not necessary.
                report.debug(u"rejecting packet, source: %s, expecting: %s", {sender, _use_source});
            }
            continue;
        }

        // Now found a packet matching all criteria.
        return true;
    }
}
