//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
//----------------------------------------------------------------------------

bool ts::Socket::createSocket(IP gen, int type, int protocol, Report& report)
{
    if (_sock != SYS_SOCKET_INVALID) {
        report.error(u"socket already open");
        return false;
    }

    // Create the socket on IPv6, unless explicitly IPv4.
    _gen = gen == IP::v4 ? IP::v4 : IP::v6;
    report.debug(u"create IPv%d socket, type %d, protocol %d", int(_gen), type, protocol);

    _sock = ::socket(gen == IP::v4 ? AF_INET : AF_INET6, type, protocol);
    if (_sock == SYS_SOCKET_INVALID) {
        report.error(u"error creating socket: %s", SysErrorCodeMessage());
        return false;
    }

    // Set the IPV6_V6ONLY option to zero on IPv6 sockets (can be used in IPv4 or IPv6 communications).
    // Warning: With OpenBSD, IPv6 sockets are always IPv6-only, so the socket option IPV6_V6ONLY is read-only
    // (see "man ip6"). As a consequence, it is impossible to use IPv4 clients on IPv6 sockets.
#if defined(IPV6_V6ONLY) && !defined(TS_OPENBSD)
    if (_gen == IP::v6) {
        SysSocketV6OnlyType opt = 0;
        if (::setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
            // don't fail, just report a warning, will still work on IPv6.
            report.warning(u"error setting option IPV6_V6ONLY: %s", SysErrorCodeMessage());
        }
    }
#endif

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
        // Shutdown should not be necessary here. However, on Linux, not using shutdown makes
        // a blocking receive hangs forever when close() is invoked by another thread. By using
        // shutdown() before close(), the blocking call is released. This is especially true on
        // UDP sockets where shutdown() is normally meaningless.
        ::shutdown(previous, SYS_SOCKET_SHUT_RDWR);
        // Actually close the socket.
        SysCloseSocket(previous);
    }
    return true;
}


//----------------------------------------------------------------------------
// Convert an IP address to make it compatible with the socket IP generation.
//----------------------------------------------------------------------------

bool ts::Socket::convert(IPAddress& addr, Report& report) const
{
    assert(_gen != IP::Any);
    const bool ok = addr.convert(_gen);
    if (!ok) {
        report.error(u"cannot use IPv%d address %s on an IPv%d socket", int(addr.generation()), addr, int(_gen));
    }
    return ok;
}


//----------------------------------------------------------------------------
// Set the send buffer size.
//----------------------------------------------------------------------------

bool ts::Socket::setSendBufferSize(size_t bytes, Report& report)
{
    int size = int(bytes); // Actual socket option is an int.
    report.debug(u"setting socket send buffer size to %'d", bytes);
    if (::setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket send buffer size: %s", SysErrorCodeMessage());
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
    report.debug(u"setting socket receive buffer size to %'d", bytes);
    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report.error(u"error setting socket receive buffer size: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive timeout.
//----------------------------------------------------------------------------

bool ts::Socket::setReceiveTimeout(cn::milliseconds timeout, Report& report)
{
    report.debug(u"setting socket receive timeout to %s", timeout);

    // If negative or zero, receive timeout is not used, reception waits forever.
    // However, setsockopt() requires a non-negative value and zero means no timeout.
    if (timeout < cn::milliseconds::zero()) {
        timeout = cn::milliseconds::zero();
    }

#if defined(TS_WINDOWS)
    ::DWORD param = ::DWORD(timeout.count());
#else
    struct timeval param;
    param.tv_sec = time_t(timeout.count() / 1000);
    param.tv_usec = suseconds_t(timeout.count() % 1000);
#endif

    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, SysSockOptPointer(&param), sizeof(param)) != 0) {
        report.error(u"error setting socket receive timeout: %s", SysErrorCodeMessage());
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
    report.debug(u"setting socket reuse address to %'d", reuse);
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse address: %s", SysErrorCodeMessage());
        return false;
    }
#if defined(TS_MAC)
    // BSD (macOS) also needs SO_REUSEPORT in addition to SO_REUSEADDR.
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEPORT, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report.error(u"error setting socket reuse port: %s", SysErrorCodeMessage());
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Get local socket address
//----------------------------------------------------------------------------

bool ts::Socket::getLocalAddress(IPSocketAddress& addr, Report& report)
{
    ::sockaddr_storage sock_addr;
    SysSocketLengthType len = sizeof(sock_addr);
    TS_ZERO(sock_addr);
    if (::getsockname(_sock, reinterpret_cast<::sockaddr*>(&sock_addr), &len) != 0) {
        report.error(u"error getting socket name: %s", SysErrorCodeMessage());
        addr.clear();
        return false;
    }
    addr.set(sock_addr);
    return true;
}
