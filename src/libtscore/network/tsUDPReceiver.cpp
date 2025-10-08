//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUDPReceiver.h"


//----------------------------------------------------------------------------
// Set reception timeout.
//----------------------------------------------------------------------------

void ts::UDPReceiver::setReceiveTimeoutArg(cn::milliseconds timeout)
{
    if (timeout > cn::milliseconds::zero()) {
        _args.receive_timeout = timeout;
    }
}


//----------------------------------------------------------------------------
// Open the socket. Override UDPSocket::open().
//----------------------------------------------------------------------------

bool ts::UDPReceiver::open(Report& report)
{
    // Ignore generation parameter, will be derived from UDPReceiverArgs.
    return open(IP::Any, report);
}

bool ts::UDPReceiver::open(IP gen, Report& report)
{
    // Check if UDP parameters were specified.
    if (!_args.destination.hasPort()) {
        report.error(u"no UDP receiver address specified");
        return false;
    }

    // If a destination address is specified, it must be a multicast address.
    if (_args.destination.hasAddress() && !_args.destination.isMulticast()) {
        report.error(u"address %s is not multicast", _args.destination);
        return false;
    }

    // Clear collection of source address information.
    _first_source.clear();
    _sources.clear();

    // The local socket address to bind is the optional local IP address and the destination port.
    // Except on Linux, macOS and probably most UNIX, when listening to a multicast group.
    // In that case, we bind to the multicast group, not the local interface.
    // Note that if _args.destination has an address, it must be a multicast one.
    IPSocketAddress local_addr;
#if defined(TS_UNIX)
    local_addr.setAddress(_args.destination.hasAddress() ? IPAddress(_args.destination) : _args.local_address);
#else
    local_addr.setAddress(_args.local_address);
#endif
    local_addr.setPort(_args.destination.port());

    // Determine IP generation of the socket (ignore input value).
    if (!local_addr.hasAddress()) {
        gen = _args.destination.generation();
    }
    else if (_args.destination.hasAddress() && local_addr.generation() != _args.destination.generation()) {
        gen = IP::v6;
    }
    else {
        gen = local_addr.generation();
    }

    // Create UDP socket from the superclass.
    // Note: On Windows, bind must be done *before* joining multicast groups.
    bool ok =
        UDPSocket::open(gen, report) &&
        reusePort(_args.reuse_port, report) &&
        setReceiveTimestamps(_args.receive_timestamps, report) &&
        setMulticastLoop(_args.mc_loopback, report) &&
        (_args.receive_bufsize <= 0 || setReceiveBufferSize(_args.receive_bufsize, report)) &&
        (_args.receive_timeout < cn::milliseconds::zero() || setReceiveTimeout(_args.receive_timeout, report)) &&
        bind(local_addr, report);

    // Optional SSM source address.
    IPAddress ssm_source;
    if (_args.use_ssm) {
        ssm_source = _args.source;
    }

    // Join multicast group.
    if (ok && _args.destination.hasAddress()) {
        if (_args.default_interface) {
            ok = addMembershipDefault(_args.destination, ssm_source, report);
        }
        else if (_args.local_address.hasAddress()) {
            ok = addMembership(_args.destination, _args.local_address, ssm_source, report);
        }
        else {
            // By default, listen on all interfaces.
            ok = addMembershipAll(_args.destination, ssm_source, !_args.no_link_local, report);
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
                              ts::IPSocketAddress& sender,
                              ts::IPSocketAddress& destination,
                              const ts::AbortInterface* abort,
                              ts::Report& report,
                              cn::microseconds* timestamp,
                              TimeStampType* timestamp_type)
{
    // Loop on packet reception until one matching filtering criteria is found.
    for (;;) {

        // Wait for a UDP message from the superclass.
        if (!UDPSocket::receive(data, max_size, ret_size, sender, destination, abort, report, timestamp, timestamp_type)) {
            return false;
        }

        // Debug (level 2) message for each message.
        if (report.maxSeverity() >= 2) {
            // Prior report level checking to avoid evaluating parameters when not necessary.
            report.log(2, u"received UDP packet, source: %s, destination: %s, timestamp: %'d", sender, destination, timestamp != nullptr ? timestamp->count() : -1);
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

        if (destination.hasAddress() && ((_args.destination.hasAddress() && destination != _args.destination) || (!_args.destination.hasAddress() && destination.isMulticast()))) {
            // This is a spurious packet.
            if (report.maxSeverity() >= Severity::Debug) {
                // Prior report level checking to avoid evaluating parameters when not necessary.
                report.debug(u"rejecting packet, destination: %s, expecting: %s", destination, _args.destination);
            }
            continue;
        }

        // Keep track of the first sender address.
        if (!_first_source.hasAddress()) {
            // First packet, keep address of the sender.
            _first_source = sender;
            _sources.insert(sender);

            // With option --first-source, use this one to filter packets.
            if (_args.use_first_source) {
                _args.source = sender;
                report.verbose(u"now filtering on source address %s", sender);
            }
        }

        // Keep track of senders (sources) to detect or filter multiple sources.
        if (_sources.count(sender) == 0) {
            // Detected an additional source, warn the user that distinct streams are potentially mixed.
            // If no source filtering is applied, this is a warning since this may affect the resulting stream.
            // With source filtering, this is just an informational verbose-level message.
            const int level = _args.source.hasAddress() ? Severity::Verbose : Severity::Warning;
            if (_sources.size() == 1) {
                report.log(level, u"detected multiple sources for the same destination %s with potentially distinct streams", destination);
                report.log(level, u"detected source: %s", _first_source);
            }
            report.log(level, u"detected source: %s", sender);
            _sources.insert(sender);
        }

        // Filter packets based on source address if requested.
        if (!sender.match(_args.source)) {
            // Not the expected source, this is a spurious packet.
            if (report.maxSeverity() >= Severity::Debug) {
                // Prior report level checking to avoid evaluating parameters when not necessary.
                report.debug(u"rejecting packet, source: %s, expecting: %s", sender, _args.source);
            }
            continue;
        }

        // Now found a packet matching all criteria.
        return true;
    }
}
