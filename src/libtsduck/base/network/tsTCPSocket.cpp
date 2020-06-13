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

#include "tsTCPSocket.h"
#include "tsIPUtils.h"
#include "tsGuard.h"
#include "tsMemory.h"
#include "tsException.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TCPSocket::TCPSocket() :
    Socket(),
    _mutex()
{
}

ts::TCPSocket::~TCPSocket()
{
    TCPSocket::close(NULLREP);
}


//----------------------------------------------------------------------------
// Default implementations of handlers.
//----------------------------------------------------------------------------

void ts::TCPSocket::handleOpened(Report& report)
{
}

void ts::TCPSocket::handleClosed(Report& report)
{
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::open(Report& report)
{
    {
        Guard lock(_mutex);
        if (!createSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, report)) {
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
        Socket::declareOpened(sock, report);
    }
    handleOpened(report);
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::close(Report& report)
{
    bool ok = true;
    {
        Guard lock(_mutex);
        // Close socket, without proper disconnection
        ok = Socket::close(report);
    }
    handleClosed(report);
    return ok;
}


//----------------------------------------------------------------------------
// Set various socket options
//----------------------------------------------------------------------------

bool ts::TCPSocket::setTTL(int ttl, Report& report)
{
    TS_SOCKET_TTL_T uttl = TS_SOCKET_TTL_T(ttl);
    report.debug(u"setting socket TTL to %'d", {uttl});
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_TTL, TS_SOCKOPT_T(&uttl), sizeof(uttl)) != 0) {
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
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, TS_SOCKOPT_T(&lin), sizeof(lin)) != 0) {
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
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, TS_SOCKOPT_T(&lin), sizeof(lin)) != 0) {
        report.error(u"socket option linger: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setKeepAlive(bool active, Report& report)
{
    int keepalive = int(active); // Actual socket option is an int.
    report.debug(u"setting socket keep-alive to %'d", {keepalive});
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_KEEPALIVE, TS_SOCKOPT_T(&keepalive), sizeof(keepalive)) != 0) {
        report.error(u"error setting socket keep alive: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoDelay(bool active, Report& report)
{
    int nodelay = int(active); // Actual socket option is an int.
    report.debug(u"setting socket no-delay to %'d", {nodelay});
    if (::setsockopt(getSocket(), IPPROTO_TCP, TCP_NODELAY, TS_SOCKOPT_T(&nodelay), sizeof(nodelay)) != 0) {
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

    report.debug(u"binding socket to %s", {addr});
    if (::bind(getSocket(), &sock_addr, sizeof(sock_addr)) != 0) {
        report.error(u"error binding socket to local address: %s", {SocketErrorCodeMessage()});
        return false;
    }
    return true;
}
