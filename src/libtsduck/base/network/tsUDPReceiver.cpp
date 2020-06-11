//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsUDPReceiver.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::UDPReceiver::UDPReceiver(ts::Report& report, bool with_short_options, bool dest_as_param) :
    UDPSocket(false, report),
    _with_short_options(with_short_options),
    _dest_as_param(dest_as_param),
    _receiver_specified(false),
    _use_ssm(false),
    _dest_addr(),
    _local_address(),
    _reuse_port(false),
    _default_interface(false),
    _use_first_source(false),
    _recv_timestamps(true), // currently hardcoded, is there a reason to disable it?
    _recv_bufsize(0),
    _recv_timeout(-1),
    _use_source(),
    _first_source(),
    _sources()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::UDPReceiver::defineArgs(ts::Args& args) const
{
    // [[source@]address:]port can be either a parameter or an option.
    const UChar* const dest_name = _dest_as_param ? u"" : u"ip-udp";
    const UChar dest_short = _dest_as_param || !_with_short_options ? 0 : 'i';
    const size_t dest_min = _dest_as_param ? 1 : 0;

    args.option(dest_name, dest_short, Args::STRING, dest_min, 1);
    args.help(dest_name, u"[address:]port",
              u"The [address:]port describes the destination of UDP packets to receive. "
              u"The 'port' part is mandatory and specifies the UDP port to listen on. "
              u"The 'address' part is optional. It specifies an IP multicast address to listen on. "
              u"It can be also a host name that translates to a multicast address. "
              u"An optional source address can be specified as 'source@address:port' in the case of SSM.");

    args.option(u"buffer-size", _with_short_options ? 'b' : 0, Args::UNSIGNED);
    args.help(u"buffer-size", u"Specify the UDP socket receive buffer size (socket option).");

    args.option(u"default-interface");
    args.help(u"default-interface",
              u"Let the system find the appropriate local interface on which to listen. "
              u"By default, listen on all local interfaces.");

    args.option(u"first-source", _with_short_options ? 'f' : 0);
    args.help(u"first-source",
              u"Filter UDP packets based on the source address. Use the sender address of "
              u"the first received packet as only allowed source. This option is useful "
              u"when several sources send packets to the same destination address and port. "
              u"Accepting all packets could result in a corrupted stream and only one "
              u"sender shall be accepted. To allow a more precise selection of the sender, "
              u"use option --source. Options --first-source and --source are mutually "
              u"exclusive.");

    args.option(u"local-address", _with_short_options ? 'l' : 0, Args::STRING);
    args.help(u"local-address", u"address",
              u"Specify the IP address of the local interface on which to listen. "
              u"It can be also a host name that translates to a local address. "
              u"By default, listen on all local interfaces.");

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"Disable the reuse port socket option. Do not use unless completely necessary.");

    args.option(u"reuse-port", _with_short_options ? 'r' : 0);
    args.help(u"reuse-port",
              u"Set the reuse port socket option. This is now enabled by default, the option "
              u"is present for legacy only.");

    args.option(u"receive-timeout", 0, Args::UNSIGNED);
    args.help(u"receive-timeout",
              u"Specify the UDP reception timeout in milliseconds. "
              u"This timeout applies to each receive operation, individually. "
              u"By default, receive operations wait for data, possibly forever.");

    args.option(u"source", _with_short_options ? 's' : 0, Args::STRING);
    args.help(u"source", u"address[:port]",
              u"Filter UDP packets based on the specified source address. This option is "
              u"useful when several sources send packets to the same destination address "
              u"and port. Accepting all packets could result in a corrupted stream and "
              u"only one sender shall be accepted. Options --first-source and --source "
              u"are mutually exclusive.");

    args.option(u"ssm");
    args.help(u"ssm",
              u"Force the usage of Source-Specific Multicast (SSM) using the source which "
              u"is specified by the option --source. The --ssm option is implicit when the "
              u"syntax 'source@address:port' is used.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::UDPReceiver::loadArgs(DuckContext& duck, Args& args)
{
    // Get destination address.
    UString destination(args.value(_dest_as_param ? u"" : u"ip-udp"));
    _receiver_specified = !destination.empty();

    // When --ip-udp is specified as an option, the presence of a UDP received is optional.
    // Option UDP-related parameters are ignored when not specified.
    if (!_dest_as_param && !_receiver_specified) {
        return true;
    }

    // General UDP options.
    _reuse_port = !args.present(u"no-reuse-port");
    _default_interface = args.present(u"default-interface");
    _use_ssm = args.present(u"ssm");
    _use_first_source = args.present(u"first-source");
    _recv_bufsize = args.intValue<size_t>(u"buffer-size", 0);
    _recv_timeout = args.intValue<MilliSecond>(u"receive-timeout", _recv_timeout); // preserve previous value

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
    if (!args.present(u"local-address")) {
        _local_address.clear();
    }
    else if (!_local_address.resolve(args.value(u"local-address"), args)) {
        return false;
    }

    // Either specify a local address or let the system decide, but not both.
    if (_default_interface && _local_address.hasAddress()) {
        args.error(u"--default-interface and --local-address are mutually exclusive");
        return false;
    }

    // Translate optional source address.
    UString source(args.value(u"source"));
    if (_use_source.hasAddress() && !source.empty()) {
        args.error(u"SSM source address specified twice");
        return false;
    }
    else if (source.empty()) {
        // Keep optional source address in source@address:port
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

void ts::UDPReceiver::setParameters(const SocketAddress& localAddress, bool reusePort, size_t bufferSize)
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
    SocketAddress local_addr(
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
        (_recv_bufsize <= 0 || setReceiveBufferSize(_recv_bufsize, report)) &&
        (_recv_timeout < 0 || setReceiveTimeout(_recv_timeout, report)) &&
        bind(local_addr, report);

    // Optional SSM source address.
    IPAddress ssm_source;
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
                              ts::SocketAddress& sender,
                              ts::SocketAddress& destination,
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
