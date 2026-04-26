//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPSocket.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TCPSocket::~TCPSocket()
{
    TCPSocket::close(true);
}


//----------------------------------------------------------------------------
// Default implementations of handlers.
//----------------------------------------------------------------------------

void ts::TCPSocket::handleOpened()
{
}

void ts::TCPSocket::handleClosed()
{
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::open(IP gen)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!createSocket(gen, SOCK_STREAM, IPPROTO_TCP)) {
            return false;
        }
    }
    handleOpened();
    return true;
}


//----------------------------------------------------------------------------
// This method is used by a server to declare that the socket has just become opened.
//----------------------------------------------------------------------------

void ts::TCPSocket::declareOpened(SysSocketType sock)
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        Socket::declareOpened(sock);
    }
    handleOpened();
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::close(bool silent)
{
    bool ok = true;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        // Close socket, without proper disconnection
        ok = Socket::close(silent);
    }
    handleClosed();
    return ok;
}


//----------------------------------------------------------------------------
// Set various socket options
//----------------------------------------------------------------------------

bool ts::TCPSocket::setTTL(int ttl)
{
    SysSocketTTLType uttl = SysSocketTTLType(ttl);
    report().debug(u"setting socket TTL to %'d", uttl);
    if (::setsockopt(getSocket(), IPPROTO_IP, IP_TTL, SysSockOptPointer(&uttl), sizeof(uttl)) != 0) {
        report().error(u"socket option TTL: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoLinger()
{
    ::linger lin;
    lin.l_onoff = 0;
    lin.l_linger = 0;
    report().debug(u"setting socket linger off");
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, SysSockOptPointer(&lin), sizeof(lin)) != 0) {
        report().error(u"socket option no linger: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


bool ts::TCPSocket::setLingerTime(int seconds)
{
    ::linger lin;
    lin.l_onoff = 1;
    lin.l_linger = SysSocketLingerType(seconds);
    report().debug(u"setting socket linger time to %'d seconds", seconds);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_LINGER, SysSockOptPointer(&lin), sizeof(lin)) != 0) {
        report().error(u"socket option linger: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


bool ts::TCPSocket::setKeepAlive(bool active)
{
    int keepalive = int(active); // Actual socket option is an int.
    report().debug(u"setting socket keep-alive to %'d", keepalive);
    if (::setsockopt(getSocket(), SOL_SOCKET, SO_KEEPALIVE, SysSockOptPointer(&keepalive), sizeof(keepalive)) != 0) {
        report().error(u"error setting socket keep alive: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


bool ts::TCPSocket::setNoDelay(bool active)
{
    int nodelay = int(active); // Actual socket option is an int.
    report().debug(u"setting socket no-delay to %'d", nodelay);
    if (::setsockopt(getSocket(), IPPROTO_TCP, TCP_NODELAY, SysSockOptPointer(&nodelay), sizeof(nodelay)) != 0) {
        report().error(u"error setting socket TCP-no-delay: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
//----------------------------------------------------------------------------

bool ts::TCPSocket::bind(const IPSocketAddress& addr)
{
    IPSocketAddress addr2(addr);
    if (!convert(addr2)) {
        return false;
    }

    ::sockaddr_storage sock_addr;
    const size_t sock_size = addr2.get(sock_addr);

    report().debug(u"binding socket to %s", addr2);
    if (::bind(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr), socklen_t(sock_size)) != 0) {
        report().error(u"error binding socket to local address %s: %s", addr2, SysErrorCodeMessage());
        return false;
    }
    return true;
}
