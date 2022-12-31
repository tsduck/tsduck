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

#include "tsTCPSocket.h"
#include "tsIPUtils.h"
#include "tsGuardMutex.h"
#include "tsMemory.h"
#include "tsException.h"


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
        GuardMutex lock(_mutex);
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

void ts::TCPSocket::declareOpened(SysSocketType sock, Report& report)
{
    {
        GuardMutex lock(_mutex);
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
        GuardMutex lock(_mutex);
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
    SysSocketTTLType uttl = SysSocketTTLType(ttl);
    report.debug(u"setting socket TTL to %'d", {uttl});
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_TTL, SysSockOptPointer(&uttl), sizeof(uttl)) != 0) {
        report.error(u"socket option TTL: %s", {SysSocketErrorCodeMessage()});
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
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, SysSockOptPointer(&lin), sizeof(lin)) != 0) {
        report.error(u"socket option no linger: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setLingerTime(int seconds, Report& report)
{
    ::linger lin;
    lin.l_onoff = 1;
    lin.l_linger = SysSocketLingerType(seconds);
    report.debug(u"setting socket linger time to %'d seconds", {seconds});
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, SysSockOptPointer(&lin), sizeof(lin)) != 0) {
        report.error(u"socket option linger: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setKeepAlive(bool active, Report& report)
{
    int keepalive = int(active); // Actual socket option is an int.
    report.debug(u"setting socket keep-alive to %'d", {keepalive});
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_KEEPALIVE, SysSockOptPointer(&keepalive), sizeof(keepalive)) != 0) {
        report.error(u"error setting socket keep alive: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoDelay(bool active, Report& report)
{
    int nodelay = int(active); // Actual socket option is an int.
    report.debug(u"setting socket no-delay to %'d", {nodelay});
    if (::setsockopt(getSocket(), IPPROTO_TCP, TCP_NODELAY, SysSockOptPointer(&nodelay), sizeof(nodelay)) != 0) {
        report.error(u"error setting socket TCP-no-delay: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
//----------------------------------------------------------------------------

bool ts::TCPSocket::bind(const IPv4SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    addr.copy(sock_addr);

    report.debug(u"binding socket to %s", {addr});
    if (::bind(getSocket(), &sock_addr, sizeof(sock_addr)) != 0) {
        report.error(u"error binding socket to local address: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
    return true;
}
