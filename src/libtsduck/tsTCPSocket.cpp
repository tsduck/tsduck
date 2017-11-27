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
//  TCP Socket
//
//----------------------------------------------------------------------------

#include "tsTCPSocket.h"
#include "tsGuard.h"
#include "tsMemoryUtils.h"
#include "tsException.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TCPSocket::TCPSocket() :
    _mutex(),
    _sock(TS_SOCKET_T_INVALID)
{
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::TCPSocket::~TCPSocket()
{
    close(NULLREP);
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::open(Report& report)
{
    {
        Guard lock(_mutex);
        if (_sock != TS_SOCKET_T_INVALID) {
            report.error(u"socket already open");
            return false;
        }
        else if ((_sock = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == TS_SOCKET_T_INVALID) {
            report.error(u"error creating socket: %s", {SocketErrorCodeMessage()});
            return false;
        }
    }
    handleOpened(report);
    return true;
}


//----------------------------------------------------------------------------
// This method is used by a server to declare that the socket has just become opened.
//----------------------------------------------------------------------------

void ts::TCPSocket::declareOpened(TS_SOCKET_T sock, Report& report)
{
    {
        Guard lock(_mutex);
        if (_sock != TS_SOCKET_T_INVALID) {
            report.fatal(u"implementation error: TCP socket already open");
            throw ImplementationError(u"TCP socket already open");
            return;
        }
        _sock = sock;
    }
    handleOpened(report);
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::close(Report& report)
{
    {
        Guard lock(_mutex);
        if (_sock == TS_SOCKET_T_INVALID) {
            report.error(u"socket already closed");
            return false;
        }
        // Close socket, without proper disconnection
        TS_SOCKET_CLOSE(_sock);
        _sock = TS_SOCKET_T_INVALID;
    }
    handleClosed(report);
    return true;
}


//----------------------------------------------------------------------------
// Set various socket options
//----------------------------------------------------------------------------

bool ts::TCPSocket::setSendBufferSize(size_t bytes, Report& report)
{
    int size = int(bytes); // Actual socket option is an int.
    report.debug(u"setting socket send buffer size to %'d", {bytes});
    if (::setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, TS_SOCKOPT_T(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket send buffer size: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setReceiveBufferSize(size_t bytes, Report& report)
{
    int size = int(bytes); // Actual socket option is an int.
    report.debug(u"setting socket receive buffer size to %'d", {bytes});
    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVBUF, TS_SOCKOPT_T(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket receive buffer size: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::reusePort(bool active, Report& report)
{
    int reuse = int(active); // Actual socket option is an int.
    report.debug(u"setting socket reuse address to %'d", {reuse});
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, TS_SOCKOPT_T(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse port: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setTTL(int ttl, Report& report)
{
    TS_SOCKET_TTL_T uttl = (TS_SOCKET_TTL_T)(ttl);
    report.debug(u"setting socket TTL to %'d", {uttl});
    if (::setsockopt(_sock, IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&uttl), sizeof(uttl)) != 0) {
        report.error(u"socket option TTL: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoLinger(Report& report)
{
    ::linger lin;
    lin.l_onoff = 0;
    lin.l_linger = 0;
    report.debug(u"setting socket linger off");
    if (::setsockopt(_sock, SOL_SOCKET, SO_LINGER, TS_SOCKOPT_T(&lin), sizeof(lin)) != 0) {
        report.error(u"socket option no linger: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setLingerTime(int seconds, Report& report)
{
    ::linger lin;
    lin.l_onoff = 1;
    lin.l_linger = u_short(seconds);
    report.debug(u"setting socket linger time to %'d seconds", {seconds});
    if (::setsockopt(_sock, SOL_SOCKET, SO_LINGER, TS_SOCKOPT_T(&lin), sizeof(lin)) != 0) {
        report.error(u"socket option linger: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setKeepAlive(bool active, Report& report)
{
    int keepalive = int(active); // Actual socket option is an int.
    report.debug(u"setting socket keep-alive to %'d", {keepalive});
    if (::setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, TS_SOCKOPT_T(&keepalive), sizeof(keepalive)) != 0) {
        report.error(u"error setting socket keep alive: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoDelay(bool active, Report& report)
{
    int nodelay = int(active); // Actual socket option is an int.
    report.debug(u"setting socket no-delay to %'d", {nodelay});
    if (::setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, TS_SOCKOPT_T(&nodelay), sizeof(nodelay)) != 0) {
        report.error(u"error setting socket TCP-no-delay: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
//----------------------------------------------------------------------------

bool ts::TCPSocket::bind(const SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"binding socket to %s", {addr.toString()});
    if (::bind(_sock, &sock_addr, sizeof(sock_addr)) != 0) {
        report.error(u"error binding socket to local address: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Get local socket address
//----------------------------------------------------------------------------

bool ts::TCPSocket::getLocalAddress (SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    TS_SOCKET_SOCKLEN_T len = sizeof(sock_addr);
    TS_ZERO (sock_addr);
    if (::getsockname(_sock, &sock_addr, &len) != 0) {
        report.error(u"error getting socket name: %s", {SocketErrorCodeMessage()});
        return false;
    }
    addr = SocketAddress(sock_addr);
    return true;
}
