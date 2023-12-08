//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPSocket.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
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
        report.error(u"socket option TTL: %s", {SysErrorCodeMessage()});
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
        report.error(u"socket option no linger: %s", {SysErrorCodeMessage()});
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
        report.error(u"socket option linger: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setKeepAlive(bool active, Report& report)
{
    int keepalive = int(active); // Actual socket option is an int.
    report.debug(u"setting socket keep-alive to %'d", {keepalive});
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_KEEPALIVE, SysSockOptPointer(&keepalive), sizeof(keepalive)) != 0) {
        report.error(u"error setting socket keep alive: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoDelay(bool active, Report& report)
{
    int nodelay = int(active); // Actual socket option is an int.
    report.debug(u"setting socket no-delay to %'d", {nodelay});
    if (::setsockopt(getSocket(), IPPROTO_TCP, TCP_NODELAY, SysSockOptPointer(&nodelay), sizeof(nodelay)) != 0) {
        report.error(u"error setting socket TCP-no-delay: %s", {SysErrorCodeMessage()});
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
        report.error(u"error binding socket to local address: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}
