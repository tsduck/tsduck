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
//  UDP Socket
//
//----------------------------------------------------------------------------

#include "tsUDPSocket.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::UDPSocket::UDPSocket(bool auto_open, Report& report) :
    Socket(),
    _local_address(),
    _default_destination(),
    _mcast()
{
    if (auto_open) {
        // Returned value ignored on purpose, the socket is marked as closed in the object on error.
        // coverity[CHECKED_RETURN]
        open(report);
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::UDPSocket::~UDPSocket()
{
    UDPSocket::close(NULLREP);
}


//----------------------------------------------------------------------------
// Open the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::open(Report& report)
{
    // Create a datagram socket.
    if (!createSocket(PF_INET, SOCK_DGRAM, IPPROTO_UDP, report)) {
        return false;
    }

    // Set the IP_PKTINFO option. This option is used to get the destination address of all
    // UDP packets arriving on this socket. Actual socket option is an int.
    int opt = 1;
    if (::setsockopt(getSocket(), SOL_IP, IP_PKTINFO, TS_SOCKOPT_T(&opt), sizeof(opt)) != 0) {
        report.error(u"error setting socket IP_PKTINFO option: %s", {SocketErrorCodeMessage()});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::UDPSocket::close(Report& report)
{
    // Leave all multicast groups.
    if (isOpen()) {
        for (MReqSet::const_iterator it = _mcast.begin(); it != _mcast.end(); ++it) {
            ::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_MEMBERSHIP, TS_SOCKOPT_T(&it->req), sizeof(it->req));
        }
        _mcast.clear();
    }

    // Close socket
    return Socket::close(report);
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::bind(const SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"binding socket to %s", {addr.toString()});
    if (::bind(getSocket(), &sock_addr, sizeof(sock_addr)) != 0) {
        report.error(u"error binding socket to local address: %s", {SocketErrorCodeMessage()});
        return false;
    }

    // Keep a cached value of the bound local address.
    return getLocalAddress(_local_address, report);
}


//----------------------------------------------------------------------------
// Set outgoing local address for multicast messages.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setOutgoingMulticast(const UString& name, Report& report)
{
    IPAddress addr;
    return addr.resolve(name, report) && setOutgoingMulticast(addr, report);
}

bool ts::UDPSocket::setOutgoingMulticast(const IPAddress& addr, Report& report)
{
    ::in_addr iaddr;
    addr.copy(iaddr);

    if (::setsockopt(getSocket(), IPPROTO_IP, IP_MULTICAST_IF, TS_SOCKOPT_T(&iaddr), sizeof(iaddr)) != 0) {
        report.error(u"error setting outgoing local address: " + SocketErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set a default destination address and port for outgoing messages.
// Both address and port are mandatory in socket address.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setDefaultDestination(const UString& name, Report& report)
{
    SocketAddress addr;
    return addr.resolve(name, report) && setDefaultDestination(addr, report);
}

bool ts::UDPSocket::setDefaultDestination(const SocketAddress& addr, Report& report)
{
    if (!addr.hasAddress()) {
        report.error(u"missing IP address in UDP destination");
        return false;
    }
    else if (!addr.hasPort()) {
        report.error(u"missing port number in UDP destination");
        return false;
    }
    else {
        _default_destination = addr;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set the Time To Live (TTL) option.
// If multicast is true, set "multicast TTL, otherwise set "unicast TTL".
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setTTL (int ttl, bool multicast, Report& report)
{
    if (multicast) {
        TS_SOCKET_MC_TTL_T mttl = (TS_SOCKET_MC_TTL_T) (ttl);
        if (::setsockopt (getSocket(), IPPROTO_IP, IP_MULTICAST_TTL, TS_SOCKOPT_T (&mttl), sizeof(mttl)) != 0) {
            report.error(u"socket option multicast TTL: " + SocketErrorCodeMessage ());
            return false;
        }
    }
    else {
        TS_SOCKET_TTL_T uttl = (TS_SOCKET_TTL_T) (ttl);
        if (::setsockopt (getSocket(), IPPROTO_IP, IP_TTL, TS_SOCKOPT_T (&uttl), sizeof(uttl)) != 0) {
            report.error(u"socket option unicast TTL: " + SocketErrorCodeMessage ());
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Join one multicast group on one local interface.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembership(const IPAddress& multicast, const IPAddress& local, Report& report)
{
    if (!local.hasAddress()) {
        // No local address specified, use all of them
        return addMembership(multicast, report);
    }
    else {
        // Add one membership
        report.verbose(u"joining multicast group %s from local address %s", {multicast.toString(), local.toString()});
        MReq req(multicast, local);
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, TS_SOCKOPT_T(&req.req), sizeof(req.req)) != 0) {
            report.error(u"error adding multicast membership to %s from local address %s: %s", {multicast.toString(), local.toString(), SocketErrorCodeMessage()});
            return false;
        }
        else {
            _mcast.insert(req);
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Join one multicast group on all local interfaces.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembership(const IPAddress& multicast, Report& report)
{
    // There is no implicit way to listen on all interfaces.
    // If no local address is specified, we must get the list
    // of all local interfaces and send a multicast membership
    // request on each of them.

    // Get all local interfaces.
    IPAddressVector loc_if;
    if (!GetLocalIPAddresses (loc_if, report)) {
        return false;
    }

    // Add all memberships
    bool ok = true;
    for (size_t i = 0; i < loc_if.size(); ++i) {
        if (loc_if[i].hasAddress()) {
            ok = addMembership (multicast, loc_if[i], report) && ok;
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Leave all multicast groups.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::dropMembership (Report& report)
{
    bool ok = true;
    for (MReqSet::const_iterator it = _mcast.begin(); it != _mcast.end(); ++it) {
        report.verbose(u"leaving multicast group %s from local address %s", {IPAddress(it->req.imr_multiaddr).toString(), IPAddress(it->req.imr_interface).toString()});
        if (::setsockopt(getSocket(), IPPROTO_IP, IP_DROP_MEMBERSHIP, TS_SOCKOPT_T(&it->req), sizeof(it->req)) != 0) {
            report.error(u"error dropping multicast membership: " + SocketErrorCodeMessage());
            ok = false;
        }
    }
    _mcast.clear();
    return ok;
}


//----------------------------------------------------------------------------
// Send a message to a destination address and port.
// Address and port are mandatory in SocketAddress.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::send(const void* data, size_t size, const SocketAddress& dest, Report& report)
{
    ::sockaddr addr;
    dest.copy(addr);

    if (::sendto(getSocket(), TS_SENDBUF_T(data), TS_SOCKET_SSIZE_T(size), 0, &addr, sizeof(addr)) < 0) {
        report.error(u"error sending UDP message: " + SocketErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Receive a message.
// If abort interface is non-zero, invoke it when I/O is interrupted
// (in case of user-interrupt, return, otherwise retry).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::receive(void* data,
                            size_t max_size,
                            size_t& ret_size,
                            SocketAddress& sender,
                            SocketAddress& destination,
                            const AbortInterface* abort,
                            Report& report)
{
    // Clear returned values
    ret_size = 0;
    sender.clear();
    destination.clear();

    // Loop on unsollicited interrupts
    for (;;) {

        // Reserve a socket address to receive the sender address.
        ::sockaddr sender_sock;
        TS_ZERO(sender_sock);

        // Build an iovec pointing to the message.
        ::iovec vec;
        TS_ZERO(vec);
        vec.iov_base = data;
        vec.iov_len = max_size;

        // Reserve a buffer to receive packet ancillary data.
        uint8_t ancil_data[1024];
        TS_ZERO(ancil_data);

        // Build a msghdr structure for recvmsg().
        ::msghdr hdr;
        TS_ZERO(hdr);
        hdr.msg_name = &sender_sock;
        hdr.msg_namelen = sizeof(sender_sock);
        hdr.msg_iov = &vec;
        hdr.msg_iovlen = 1; // number of iovec structures
        hdr.msg_control = ancil_data;
        hdr.msg_controllen = sizeof(ancil_data);

        // Wait for a message.
        TS_SOCKET_SSIZE_T insize = ::recvmsg(getSocket(), &hdr, 0);

        if (insize >= 0) {
            // Received a message
            ret_size = size_t(insize);
            sender = SocketAddress(sender_sock);

            // Browse returned ancillary data.
            for (::cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr); cmsg != 0; cmsg = CMSG_NXTHDR(&hdr, cmsg)) {
                report.debug(u"UDP recvmsg, ancillary message %d, level %d, %d bytes", {cmsg->cmsg_type, cmsg->cmsg_level, cmsg->cmsg_len});
                if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_PKTINFO && cmsg->cmsg_len >= sizeof(::in_pktinfo)) {
                    const ::in_pktinfo* info = reinterpret_cast<const ::in_pktinfo*>(CMSG_DATA(cmsg));
                    destination = SocketAddress(info->ipi_addr, _local_address.port());
                }
            }

            return true;
        }
        else if (abort != 0 && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
#if !defined (TS_WINDOWS)
        else if (errno == EINTR) {
            // Got a signal, not a user interrupt, will ignore it
            report.debug(u"signal, not user interrupt");
        }
#endif
        else {
            // Abort on non-interrupt errors.
            report.error(u"error receiving from UDP socket: %s", {SocketErrorCodeMessage()});
            return false;
        }
    }
}
