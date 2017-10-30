//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::UDPSocket::UDPSocket(bool auto_open, ReportInterface& report) :
    _sock(TS_SOCKET_T_INVALID),
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
    close();
}


//----------------------------------------------------------------------------
// Open the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::open (ReportInterface& report)
{
    if (_sock != TS_SOCKET_T_INVALID) {
        report.error ("socket already open");
        return false;
    }
    else if ((_sock = ::socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == TS_SOCKET_T_INVALID) {
        report.error ("error creating socket: " + SocketErrorCodeMessage ());
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

void ts::UDPSocket::close()
{
    if (_sock != TS_SOCKET_T_INVALID) {
        // Leave all multicast groups.
        for (MReqSet::const_iterator it = _mcast.begin(); it != _mcast.end(); ++it) {
            ::setsockopt (_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, TS_SOCKOPT_T (&it->req), sizeof(it->req));
        }
        _mcast.clear();
        // Close socket
        TS_SOCKET_CLOSE (_sock);
        _sock = TS_SOCKET_T_INVALID;
    }
}


//----------------------------------------------------------------------------
// Set the send buffer size.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setSendBufferSize (size_t buffer_size, ReportInterface& report)
{
    // Actual socket option is an int.
    int size = int (buffer_size);

    if (::setsockopt (_sock, SOL_SOCKET, SO_SNDBUF, TS_SOCKOPT_T (&size), sizeof(size)) != 0) {
        report.error ("error setting socket send buffer size: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive buffer size.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setReceiveBufferSize (size_t buffer_size, ReportInterface& report)
{
    // Actual socket option is an int.
    int size = int (buffer_size);

    if (::setsockopt (_sock, SOL_SOCKET, SO_RCVBUF, TS_SOCKOPT_T (&size), sizeof(size)) != 0) {
        report.error ("error setting socket receive buffer size: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the "reuse port" option.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::reusePort (bool reuse_port, ReportInterface& report)
{
    // Actual socket option is an int.
    int reuse = int (reuse_port);

    if (::setsockopt (_sock, SOL_SOCKET, SO_REUSEADDR, TS_SOCKOPT_T (&reuse), sizeof(reuse)) != 0) {
        report.error ("error setting socket reuse port: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::bind (const SocketAddress& addr, ReportInterface& report)
{
    ::sockaddr sock_addr;
    addr.copy (sock_addr);

    if (::bind (_sock, &sock_addr, sizeof(sock_addr)) != 0) {
        report.error ("error binding socket to local address: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set outgoing local address for multicast messages.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setOutgoingMulticast (const std::string& name, ReportInterface& report)
{
    IPAddress addr;
    return addr.resolve (name, report) && setOutgoingMulticast (addr, report);
}

bool ts::UDPSocket::setOutgoingMulticast (const IPAddress& addr, ReportInterface& report)
{
    ::in_addr iaddr;
    addr.copy (iaddr);

    if (::setsockopt (_sock, IPPROTO_IP, IP_MULTICAST_IF, TS_SOCKOPT_T (&iaddr), sizeof(iaddr)) != 0) {
        report.error ("error setting outgoing local address: " + SocketErrorCodeMessage ());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set a default destination address and port for outgoing messages.
// Both address and port are mandatory in socket address.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::setDefaultDestination (const std::string& name, ReportInterface& report)
{
    SocketAddress addr;
    return addr.resolve (name, report) && setDefaultDestination (addr, report);
}

bool ts::UDPSocket::setDefaultDestination (const SocketAddress& addr, ReportInterface& report)
{
    if (!addr.hasAddress()) {
        report.error ("missing IP address in UDP destination");
        return false;
    }
    else if (!addr.hasPort()) {
        report.error ("missing port number in UDP destination");
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

bool ts::UDPSocket::setTTL (int ttl, bool multicast, ReportInterface& report)
{
    if (multicast) {
        TS_SOCKET_MC_TTL_T mttl = (TS_SOCKET_MC_TTL_T) (ttl);
        if (::setsockopt (_sock, IPPROTO_IP, IP_MULTICAST_TTL, TS_SOCKOPT_T (&mttl), sizeof(mttl)) != 0) {
            report.error ("socket option multicast TTL: " + SocketErrorCodeMessage ());
            return false;
        }
    }
    else {
        TS_SOCKET_TTL_T uttl = (TS_SOCKET_TTL_T) (ttl);
        if (::setsockopt (_sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T (&uttl), sizeof(uttl)) != 0) {
            report.error ("socket option unicast TTL: " + SocketErrorCodeMessage ());
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Join one multicast group on one local interface.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembership (const IPAddress& multicast, const IPAddress& local, ReportInterface& report)
{
    if (!local.hasAddress()) {
        // No local address specified, use all of them
        return addMembership (multicast, report);
    }
    else {
        // Add one membership
        report.verbose ("joining multicast group " + std::string (multicast) + " from local address " + std::string (local));
        MReq req (multicast, local);
        if (::setsockopt (_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, TS_SOCKOPT_T (&req.req), sizeof(req.req)) != 0) {
            report.error ("error adding multicast membership to " + std::string (multicast) +
                          " from local address " + std::string (local) +
                          SocketErrorCodeMessage ());
            return false;
        }
        else {
            _mcast.insert (req);
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Join one multicast group on all local interfaces.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::UDPSocket::addMembership (const IPAddress& multicast, ReportInterface& report)
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

bool ts::UDPSocket::dropMembership (ReportInterface& report)
{
    bool ok = true;
    for (MReqSet::const_iterator it = _mcast.begin(); it != _mcast.end(); ++it) {
        report.verbose ("leaving multicast group " + std::string (IPAddress (it->req.imr_multiaddr)) +
                        " from local address " + std::string (IPAddress (it->req.imr_interface)));
        if (::setsockopt (_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, TS_SOCKOPT_T (&it->req), sizeof(it->req)) != 0) {
            report.error ("error dropping multicast membership: " + SocketErrorCodeMessage ());
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

bool ts::UDPSocket::send (const void* data, size_t size, const SocketAddress& dest, ReportInterface& report)
{
    ::sockaddr addr;
    dest.copy (addr);

    if (::sendto (_sock, TS_SENDBUF_T (data), TS_SOCKET_SSIZE_T (size), 0, &addr, sizeof(addr)) < 0) {
        report.error ("error sending UDP message: " + SocketErrorCodeMessage ());
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

bool ts::UDPSocket::receive (void* data,
                               size_t max_size,
                               size_t& ret_size,
                               SocketAddress& sender,
                               const AbortInterface* abort,
                               ReportInterface& report)
{
    // Clear returned values
    ret_size = 0;
    sender.clear();

    // Loop on unsollicited interrupts
    for (;;) {

        ::sockaddr sender_sock;
        TS_SOCKET_SOCKLEN_T senderlen = sizeof(sender_sock);
        TS_SOCKET_SSIZE_T insize = ::recvfrom(_sock, TS_RECVBUF_T(data), int(max_size), 0, &sender_sock, &senderlen);

        if (insize >= 0) {
            // Received a message
            ret_size = size_t (insize);
            sender = SocketAddress (sender_sock);
            return true;
        }
        else if (abort != 0 && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
#if !defined (TS_WINDOWS)
        else if (errno == EINTR) {
            // Got a signal, not a user interrupt, will ignore it
            report.debug ("signal, not user interrupt");
        }
#endif
        else {
            // Abort on non-interrupt errors.
            report.error ("error receiving from UDP socket: " + SocketErrorCodeMessage());
            return false;
        }
    }
}
