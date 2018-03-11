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

#include "tsUDPReceiver.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::UDPReceiver::UDPReceiver(ts::Report& report) :
    UDPSocket(false, report),
    _dest_addr(),
    _local_address(),
    _reuse_port(false),
    _use_first_source(false),
    _recv_bufsize(0),
    _use_source(),
    _first_source(),
    _sources()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::UDPReceiver::defineOptions(ts::Args& args) const
{
    args.option(u"",               0,  Args::STRING, 1, 1);
    args.option(u"buffer-size",   'b', Args::UNSIGNED);
    args.option(u"first-source",  'f');
    args.option(u"local-address", 'l', Args::STRING);
    args.option(u"reuse-port",    'r');
    args.option(u"source",        's', Args::STRING);
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args.
//----------------------------------------------------------------------------

void ts::UDPReceiver::addHelp(ts::Args& args) const
{
    UString help =
            u"Parameter:\n"
            u"\n"
            u"  The parameter [address:]port describes the destination of UDP packets.\n"
            u"  The 'port' part is mandatory and specifies the UDP port to listen on.\n"
            u"  The 'address' part is optional. It specifies an IP multicast address\n"
            u"  to listen on. It can be also a host name that translates to a multicast\n"
            u"  address.\n"
            u"\n"
            u"UDP reception options:\n"
            u"\n"
            u"  -b value\n"
            u"  --buffer-size value\n"
            u"      Specify the UDP socket receive buffer size (socket option).\n"
            u"\n"
            u"  -f\n"
            u"  --first-source\n"
            u"      Filter UDP packets based on the source address. Use the sender address of\n"
            u"      the first received packet as only allowed source. This option is useful\n"
            u"      when several sources send packets to the same destination address and port.\n"
            u"      Accepting all packets could result in a corrupted stream and only one\n"
            u"      sender shall be accepted. To allow a more precise selection of the sender,\n"
            u"      use option --source. Options --first-source and --source are mutually\n"
            u"      exclusive.\n"
            u"\n"
            u"  -l address\n"
            u"  --local-address address\n"
            u"      Specify the IP address of the local interface on which to listen.\n"
            u"      It can be also a host name that translates to a local address.\n"
            u"      By default, listen on all local interfaces.\n"
            u"\n"
            u"  -r\n"
            u"  --reuse-port\n"
            u"      Set the reuse port socket option.\n"
            u"\n"
            u"  -s address[:port]\n"
            u"  --source address[:port]\n"
            u"      Filter UDP packets based on the specified source address. This option is\n"
            u"      useful when several sources send packets to the same destination address\n"
            u"      and port. Accepting all packets could result in a corrupted stream and\n"
            u"      only one sender shall be accepted. Options --first-source and --source\n"
            u"      are mutually exclusive.\n";

    args.setHelp(help + args.getHelp());
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::UDPReceiver::load(ts::Args& args)
{
    // General options.
    _reuse_port = args.present(u"reuse-port");
    _use_first_source = args.present(u"first-source");
    _recv_bufsize = args.intValue<size_t>(u"buffer-size", 0);

    // Get and resolve destination address.
    const UString destination(args.value(u""));
    if (!_dest_addr.resolve(destination, args)) {
        return false;
    }

    // If a destination address is specified, it must be a multicast address
    if (_dest_addr.hasAddress() && !_dest_addr.isMulticast()) {
        args.error(u"address %s is not multicast", {_dest_addr.toString()});
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

    // Translate optional source address.
    UString source(args.value(u"source"));
    if (source.empty()) {
        _use_source.clear();
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

    return true;
}


//----------------------------------------------------------------------------
// Open the socket. Override UDPSocket::open().
//----------------------------------------------------------------------------

bool ts::UDPReceiver::open(ts::Report& report)
{
    // Clear collection of source address information.
    _first_source.clear();
    _sources.clear();

    // The local socket address to bind is the optional local IP address and the destination port
    SocketAddress local_addr(_local_address, _dest_addr.port());

    // Create UDP socket from the superclass.
    // Note: On Windows, bind must be done *before* joining multicast groups.
    const bool ok =
        UDPSocket::open(report) &&
        reusePort(_reuse_port, report) &&
        (_recv_bufsize <= 0 || setReceiveBufferSize(_recv_bufsize, report)) &&
        bind(local_addr, report) &&
        (!_dest_addr.hasAddress() || addMembership(_dest_addr, _local_address, report));

    if (!ok) {
        close();
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
                              ts::Report& report)
{
    // Loop on packet reception until one matching filtering criteria is found.
    for (;;) {

        // Wait for a UDP message from the superclass.
        if (!UDPSocket::receive(data, max_size, ret_size, sender, destination, abort, report)) {
            return false;
        }

        // Debug (level 2) message for each message.
        report.log(2, u"received UDP packet, source: %s, destination: %s", {sender.toString(), destination.toString()});

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
            report.debug(u"rejecting packet, destination: %s, expecting: %s", {destination.toString(), _dest_addr.toString()});
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
                report.verbose(u"now filtering on source address %s", {sender.toString()});
            }
        }

        // Keep track of senders (sources) to detect or filter multiple sources.
        if (_sources.count(sender) == 0) {
            // Detected an additional source, warn the user that distinct streams are potentially mixed.
            // If no source filtering is applied, this is a warning since this may affect the resulting stream.
            // With source filtering, this is just an informational verbose-level message.
            const int level = _use_source.hasAddress() ? Severity::Verbose : Severity::Warning;
            if (_sources.size() == 1) {
                report.log(level, u"detected multiple sources for the same destination %s with potentially distinct streams", {destination.toString()});
                report.log(level, u"detected source: %s", {_first_source.toString()});
            }
            report.log(level, u"detected source: %s", {sender.toString()});
            _sources.insert(sender);
        }

        // Filter packets based on source address if requested.
        if (!sender.match(_use_source)) {
            // Not the expected source, this is a spurious packet.
            report.debug(u"rejecting packet, source: %s, expecting: %s", {sender.toString(), _use_source.toString()});
            continue;
        }

        // Now found a packet matching all criteria.
        return true;
    }
}
