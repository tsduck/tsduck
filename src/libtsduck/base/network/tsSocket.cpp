//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSocket.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsException.h"
#include "tsNullReport.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Socket::~Socket()
{
    Socket::close(NULLREP);
}


//----------------------------------------------------------------------------
// Create the socket
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Socket::createSocket(int domain, int type, int protocol, Report& report)
{
    if (_sock != SYS_SOCKET_INVALID) {
        report.error(u"socket already open");
        return false;
    }

    // Create a datagram socket.
    _sock = ::socket(domain, type, protocol);
    if (_sock == SYS_SOCKET_INVALID) {
        report.error(u"error creating socket: %s", {SysErrorCodeMessage()});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Set an open socket descriptor from a subclass.
//----------------------------------------------------------------------------

void ts::Socket::declareOpened(SysSocketType sock, Report& report)
{
    if (isOpen()) {
        report.fatal(u"implementation error: socket already open");
        throw ImplementationError(u"socket already open");
    }
    _sock = sock;
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::Socket::close(Report& report)
{
    if (_sock != SYS_SOCKET_INVALID) {
        // Mark the socket as invalid. If the close generates reception errors in other threads,
        // these threads can immediately check if this is a real error or the result of a close.
        const SysSocketType previous = _sock;
        _sock = SYS_SOCKET_INVALID;
        // Shutdown should not be necessary here. However, on Linux, no using
        // shutdown makes a blocking receive hangs forever when close() is
        // invoked by another thread. By using shutdown() before close(),
        // the blocking call is released. This is especially true on UDP sockets
        // where shutdown() is normally meaningless.
        ::shutdown(previous, SYS_SOCKET_SHUT_RDWR);
        // Actually close the socket.
        SysCloseSocket(previous);
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the send buffer size.
//----------------------------------------------------------------------------

bool ts::Socket::setSendBufferSize(size_t bytes, Report& report)
{
    int size = int(bytes); // Actual socket option is an int.
    report.debug(u"setting socket send buffer size to %'d", {bytes});
    if (::setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket send buffer size: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive buffer size.
//----------------------------------------------------------------------------

bool ts::Socket::setReceiveBufferSize(size_t bytes, Report& report)
{
    int size = int(bytes); // Actual socket option is an int.
    report.debug(u"setting socket receive buffer size to %'d", {bytes});
    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket receive buffer size: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive timeout.
//----------------------------------------------------------------------------

bool ts::Socket::setReceiveTimeout(ts::MilliSecond timeout, ts::Report& report)
{
    report.debug(u"setting socket receive timeout to %'d ms", {timeout});

#if defined(TS_WINDOWS)
    ::DWORD param = ::DWORD(timeout);
#else
    struct timeval param;
    param.tv_sec = time_t(timeout / MilliSecPerSec);
    param.tv_usec = suseconds_t(timeout % MilliSecPerSec);
#endif

    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, SysSockOptPointer(&param), sizeof(param)) != 0) {
        report.error(u"error setting socket receive timeout: %s", {SysErrorCodeMessage()});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Set the "reuse port" option.
//----------------------------------------------------------------------------

bool ts::Socket::reusePort(bool active, Report& report)
{
    int reuse = int(active); // Actual socket option is an int.
    report.debug(u"setting socket reuse address to %'d", {reuse});
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse address: %s", {SysErrorCodeMessage()});
        return false;
    }
#if defined(TS_MAC)
    // BSD (macOS) also needs SO_REUSEPORT in addition to SO_REUSEADDR.
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEPORT, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse port: %s", {SysErrorCodeMessage()});
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Get local socket address
//----------------------------------------------------------------------------

bool ts::Socket::getLocalAddress(IPv4SocketAddress& addr, Report& report)
{
    ::sockaddr sock_addr;
    SysSocketLengthType len = sizeof(sock_addr);
    TS_ZERO(sock_addr);
    if (::getsockname(_sock, &sock_addr, &len) != 0) {
        report.error(u"error getting socket name: %s", {SysErrorCodeMessage()});
        addr.clear();
        return false;
    }
    addr = IPv4SocketAddress(sock_addr);
    return true;
}
