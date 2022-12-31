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

#include "tsSocket.h"
#include "tsIPUtils.h"
#include "tsException.h"
#include "tsNullReport.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Socket::Socket() :
    _sock(SYS_SOCKET_INVALID)
{
}


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
        report.error(u"error creating socket: %s", {SysSocketErrorCodeMessage()});
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
        report.error(u"error setting socket send buffer size: %s", {SysSocketErrorCodeMessage()});
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
        report.error(u"error setting socket receive buffer size: %s", {SysSocketErrorCodeMessage()});
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
        report.error(u"error setting socket receive timeout: %s", {SysSocketErrorCodeMessage()});
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
        report.error(u"error setting socket reuse address: %s", {SysSocketErrorCodeMessage()});
        return false;
    }
#if defined(TS_MAC)
    // BSD (macOS) also needs SO_REUSEPORT in addition to SO_REUSEADDR.
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEPORT, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse port: %s", {SysSocketErrorCodeMessage()});
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
        report.error(u"error getting socket name: %s", {SysSocketErrorCodeMessage()});
        addr.clear();
        return false;
    }
    addr = IPv4SocketAddress(sock_addr);
    return true;
}
