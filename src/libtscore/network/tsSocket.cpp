//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSocket.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsException.h"
#include "tsInitZero.h"


//----------------------------------------------------------------------------
// Constructors & destructor
//----------------------------------------------------------------------------

ts::Socket::Socket(Report* report, bool non_blocking, Object* owner) :
    NonBlockingDevice(report, non_blocking),
    OwnedObject(owner)
{
}

ts::Socket::Socket(ReporterBase* delegate, bool non_blocking, Object* owner) :
    NonBlockingDevice(delegate, non_blocking),
    OwnedObject(owner)
{
}

ts::Socket::~Socket()
{
    if (isOpen()) {
        Socket::close(true);
    }

    // Tell all registered subscribers to deregister from this object before destroying it.
    callSubscribers([this](SocketHandlerInterface* subs) {
        subs->deregisterSocket(this);
    });
    _subscribers.clear();
}


//----------------------------------------------------------------------------
// Add/remove a subscriber to open/close events.
//----------------------------------------------------------------------------

void ts::Socket::addSubscription(SocketHandlerInterface* handler)
{
    if (handler != nullptr) {
        // Make sure we can call the handler.
        _subscribers.insert(handler);

        // Make sure the handler can cancel its subscription when destructed.
        handler->registerSocket(this);
    }
}

void ts::Socket::cancelSubscription(SocketHandlerInterface* handler)
{
    if (handler != nullptr) {
        // No longer call the handler.
        _subscribers.erase(handler);

        // Make sure the handler no longer cancel its subscription when destructed.
        handler->deregisterSocket(this);
    }
}


//----------------------------------------------------------------------------
// Check that the non-blocking mode can be set.
//----------------------------------------------------------------------------

bool ts::Socket::allowSetNonBlocking() const
{
    // Cannot change the blocking mode after open.
    return !isOpen();
}


//----------------------------------------------------------------------------
// Create the socket
//----------------------------------------------------------------------------

bool ts::Socket::createSocket(IP gen, int type, int protocol)
{
    if (isOpen()) {
        report().error(u"socket already open");
        return false;
    }

    // Nonblocking mode
    // ----------------
    // On Windows, socket() always create a socket in overlapped mode (the asynchronous I/O are used only
    // when an OVERLAPPED structure is specified). By default, the OVERLAPPED is posted to I/O Control Ports
    // in all cases, including when the I/O immediately completes. We keep this default behaviour to ensure
    // that I/O completions are processed in the right order. If we wanted to change this, then use:
    // ::SetFileCompletionNotificationModes(::HANDLE(_sock), FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)
    // On some UNIX systems, the socket can be set in non-blocking mode from the beginning.
    // On macOS, the file descriptor must be manually set in non-blocking mode afterwards.
    int type_flags = 0;
#if defined(TS_UNIX) && !defined(TS_MAC)
    if (isNonBlocking()) {
        type_flags = SOCK_NONBLOCK;
    }
#endif

    // Create the socket on IPv6, unless explicitly IPv4.
    _gen = gen == IP::v4 ? IP::v4 : IP::v6;
    report().debug(u"create IPv%d socket, type %d, protocol %d", int(_gen), type, protocol);

    _sock = ::socket(gen == IP::v4 ? AF_INET : AF_INET6, type | type_flags, protocol);
    if (_sock == SYS_SOCKET_INVALID) {
        report().error(u"error creating socket: %s", SysErrorCodeMessage());
        return false;
    }

    // Set the file descriptor in non-blocking mode if not set when the socket was created.
#if defined(TS_MAC)
    if (isNonBlocking() && !setSystemNonBlocking(_sock, true)) {
        SysCloseSocket(_sock);
        return false;
     }
#endif

    // Set the IPV6_V6ONLY option to zero on IPv6 sockets (can be used in IPv4 or IPv6 communications).
    // Warning: With OpenBSD, IPv6 sockets are always IPv6-only, so the socket option IPV6_V6ONLY is read-only
    // (see "man ip6"). As a consequence, it is impossible to use IPv4 clients on IPv6 sockets.
#if defined(IPV6_V6ONLY) && !defined(TS_OPENBSD)
    if (_gen == IP::v6) {
        SysSocketV6OnlyType opt = 0;
        if (::setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, SysSockOptPointer(&opt), sizeof(opt)) != 0) {
            // don't fail, just report a warning, will still work on IPv6.
            report().warning(u"error setting option IPV6_V6ONLY: %s", SysErrorCodeMessage());
        }
    }
#endif

    return true;
}


//----------------------------------------------------------------------------
// Set an open socket descriptor from a subclass.
//----------------------------------------------------------------------------

void ts::Socket::declareOpened(SysSocketType sock)
{
    if (isOpen()) {
        report().fatal(u"implementation error: socket already open");
        throw ImplementationError(u"socket already open");
    }

    // Notify all subscribers that the socket is about to open.
    callSubscribers([this](SocketHandlerInterface* subs) {
        subs->handleSocketOpenStart(*this);
    });

    // Remember the system socket descriptor.
    _sock = sock;

    // Notify all subscribers of the open success.
    callSubscribers([this](SocketHandlerInterface* subs) {
        subs->handleSocketOpenComplete(*this, true);
    });
}


//----------------------------------------------------------------------------
// Open the socket.
//----------------------------------------------------------------------------

bool ts::Socket::open(IP gen)
{
    // Early filtering of already opened socket.
    if (_sock != SYS_SOCKET_INVALID) {
        report().error(u"socket already open");
        return false;
    }

    // Notify all subscribers that the socket is about to open.
    callSubscribers([this](SocketHandlerInterface* subs) {
        subs->handleSocketOpenStart(*this);
    });

    // Actual implementation.
    const bool success = openImplementation(gen);

    // Notify all subscribers of the open status.
    callSubscribers([this, success](SocketHandlerInterface* subs) {
        subs->handleSocketOpenComplete(*this, success);
    });
    return success;
}

//----------------------------------------------------------------------------
// Close the socket.
//----------------------------------------------------------------------------

bool ts::Socket::close(bool silent)
{
    // Early filtering of already closed socket.
    if (_sock == SYS_SOCKET_INVALID) {
        report().log(SilentLevel(silent), u"socket not open");
        return false;
    }

    // Notify all subscribers that the socket is about to open.
    callSubscribers([this, silent](SocketHandlerInterface* subs) {
        subs->handleSocketCloseStart(*this, silent);
    });

    // Actual implementation.
    const bool success = closeImplementation(silent);

    // Notify all subscribers of the close status.
    callSubscribers([this, silent, success](SocketHandlerInterface* subs) {
        subs->handleSocketCloseComplete(*this, silent, success);
    });
    return success;
}

bool ts::Socket::closeImplementation(bool silent)
{
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
    const int err = SysCloseSocket(previous);
    if (err == SYS_SUCCESS) {
        return true;
    }
    else {
        report().log(SilentLevel(silent), u"error closing socket: %s", SysErrorCodeMessage(err));
        return false;
    }
}


//----------------------------------------------------------------------------
// Convert an IP address to make it compatible with the socket IP generation.
//----------------------------------------------------------------------------

bool ts::Socket::convert(IPAddress& addr) const
{
    assert(_gen != IP::Any);
    const bool ok = addr.convert(_gen);
    if (!ok) {
        report().error(u"cannot use IPv%d address %s on an IPv%d socket", int(addr.generation()), addr, int(_gen));
    }
    return ok;
}


//----------------------------------------------------------------------------
// Set the send buffer size.
//----------------------------------------------------------------------------

bool ts::Socket::setSendBufferSize(size_t bytes)
{
    int size = int(bytes); // Actual socket option is an int.
    report().debug(u"setting socket send buffer size to %'d", bytes);
    if (::setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report().error(u"error setting socket send buffer size: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive buffer size.
//----------------------------------------------------------------------------

bool ts::Socket::setReceiveBufferSize(size_t bytes)
{
    int size = int(bytes); // Actual socket option is an int.
    report().debug(u"setting socket receive buffer size to %'d", bytes);
    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVBUF, SysSockOptPointer(&size), sizeof(size)) != 0) {
        report().error(u"error setting socket receive buffer size: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Set the receive timeout.
//----------------------------------------------------------------------------

bool ts::Socket::setReceiveTimeout(cn::milliseconds timeout)
{
    report().debug(u"setting socket receive timeout to %s", timeout);

    // If negative or zero, receive timeout is not used, reception waits forever.
    // However, setsockopt() requires a non-negative value and zero means no timeout.
    if (timeout < cn::milliseconds::zero()) {
        timeout = cn::milliseconds::zero();
    }

#if defined(TS_WINDOWS)
    ::DWORD param = ::DWORD(timeout.count());
#else
    struct timeval param;
    param.tv_sec = timeval_sec_t(timeout.count() / 1000);
    param.tv_usec = timeval_usec_t(timeout.count() % 1000);
#endif

    if (::setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, SysSockOptPointer(&param), sizeof(param)) != 0) {
        report().error(u"error setting socket receive timeout: %s", SysErrorCodeMessage());
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Set the "reuse port" option.
//----------------------------------------------------------------------------

bool ts::Socket::reusePort(bool active)
{
    int reuse = int(active); // Actual socket option is an int.
    report().debug(u"setting socket reuse address to %'d", reuse);
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report().error(u"error setting socket reuse address: %s", SysErrorCodeMessage());
        return false;
    }
#if defined(TS_MAC)
    // BSD (macOS) also needs SO_REUSEPORT in addition to SO_REUSEADDR.
    if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEPORT, SysSockOptPointer(&reuse), sizeof(reuse)) != 0) {
        report().error(u"error setting socket reuse port: %s", SysErrorCodeMessage());
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Bind to a local address and port.
//----------------------------------------------------------------------------

bool ts::Socket::bind(const IPSocketAddress& addr)
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


//----------------------------------------------------------------------------
// Get local socket address
//----------------------------------------------------------------------------

bool ts::Socket::getLocalAddress(IPSocketAddress& addr) const
{
    InitZero<::sockaddr_storage> sock_addr;
    SysSocketLengthType len = sizeof(sock_addr.data);
    if (::getsockname(_sock, reinterpret_cast<::sockaddr*>(&sock_addr.data), &len) != 0) {
        report().error(u"error getting socket name: %s", SysErrorCodeMessage());
        addr.clear();
        return false;
    }
    addr.set(sock_addr.data);
    return true;
}

ts::UString ts::Socket::localName()
{
    // Get socket address without error message.
    IPSocketAddress addr;
    const bool mute = muteReport(true);
    const bool success = getLocalAddress(addr);
    muteReport(mute);
    return success ? addr.toString() : u"";
}
