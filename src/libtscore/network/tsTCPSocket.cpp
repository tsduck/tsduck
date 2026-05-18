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
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TCPSocket::~TCPSocket()
{
    if (isOpen()) {
        TCPSocket::close(true);
    }
}


//----------------------------------------------------------------------------
// Open the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::openImplementation(IP gen)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return createSocket(gen, SOCK_STREAM, IPPROTO_TCP);
}


//----------------------------------------------------------------------------
// Called by a server to declare that the socket has just become opened.
//----------------------------------------------------------------------------

void ts::TCPSocket::declareOpened(SysSocketType sock)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    SuperClass::declareOpened(sock);
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::TCPSocket::closeImplementation(bool silent)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return SuperClass::closeImplementation(silent);
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
