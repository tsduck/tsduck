//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPServer.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsInitZero.h"
#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
#endif


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TCPServer::listen(int backlog)
{
    report().debug(u"server listen, backlog is %d", backlog);
    if (::listen(getSocket(), backlog) != 0) {
        report().error(u"error starting TCP server: %s", SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Wait for a client
//----------------------------------------------------------------------------

bool ts::TCPServer::accept(TCPConnection& client, IPSocketAddress& client_address, IOSB* iosb)
{
    client_address.clear();

    // Check that the application uses the right blocking mode.
    if (!checkNonBlocking(iosb, u"TCPServer::accept")) {
        return false;
    }
    if (client.isConnected()) {
        report().error(u"invalid client in accept(): already connected");
        return false;
    }
    if (client.isOpen()) {
        report().error(u"invalid client in accept(): already open");
        return false;
    }
    report().debug(u"server accepting clients");

#if defined(TS_WINDOWS)
    // On Windows with asynchronous I/O, use overlapped I/O.
    // With standard blocking I/O, use the same standard socket calls as UNIX.
    if (isNonBlocking()) {
        assert(iosb != nullptr);

        // Get the address of AcceptEx the first time we use it.
        // Thread-safe init-safe static data pattern.
        static const ::GUID accept_ex_guid = WSAID_ACCEPTEX;
        static int accept_ex_error = 0;
        static const ::LPFN_ACCEPTEX accept_ex = reinterpret_cast<::LPFN_ACCEPTEX>(GetWSAFunction(accept_ex_guid, accept_ex_error));
        if (accept_ex == nullptr) {
            report().error(u"error fetching AcceptEx: %s", SysErrorCodeMessage(accept_ex_error));
            return false;
        }

        // With AcceptEx, the client connection must be already created.
        // Use the same IP generation as the server socket.
        if (!client.open(generation())) {
            return false;
        }

        // The reception parameters are stored in the IOSB.
        auto params = std::make_shared<AcceptAsyncBuffers>();
        TS_ZERO(params->buf);
        iosb->async_data = params;

        // Start an asynchronous accept. Don't accept any early incoming data with the connection.
        // Consider that the I/O is pending if it immediately completed because an asynchronous I/O completion will be posted.
        int err = SYS_SUCCESS;
        ::DWORD ret_size = 0;
        if (!accept_ex(getSocket(), client.getSocket(), params->buf, 0, AcceptAsyncBuffers::ADDR_BUFLEN, AcceptAsyncBuffers::ADDR_BUFLEN, &ret_size, &iosb->overlap)) {
            err = LastSysErrorCode();
        }
        iosb->pending = SysSuccess(err) || IsPendingStatus(err);
        if (!iosb->pending) {
            report().error(u"error accepting TCP client: %s", SysErrorCodeMessage(err));
            client.close(true);
        }
        return iosb->pending;
    }
#endif

    // With the standard socket API.
    InitZero<::sockaddr_storage> sock_addr;
    SysSocketLengthType len = sizeof(sock_addr.data);
    SysSocketType client_sock = ::accept(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr.data), &len);
    const int err = LastSysErrorCode();

    if (client_sock != SYS_SOCKET_INVALID) {
        // Successful accept.
        client_address = IPSocketAddress(sock_addr.data);
        report().debug(u"received connection from %s", client_address);

        // On Linux, when the server socket is in non-blocking mode, the newly accepted socket is not. It must be
        // explicitely set. On macOS, it seems to be in non-blocking mode, although not documented. On Windows,
        // all sockets have the "overlapped" capability by default, they do not use non-blocking mode. Let's force
        // non-blocking mode on all operating systems when the server is also in that mode. At worse, it does nothing.
#if defined(TS_UNIX)
        if (isNonBlocking() && !setSystemNonBlocking(client_sock, true)) {
            return false;
        }
#endif

        // Initialize the client socket.
        client.declareOpened(client_sock);
        client.declareConnected();
        return true;
    }
    else if (isNonBlocking() && IsPendingStatus(err)) {
        // UNIX: Non-blocking server socket with no immediate connection.
        assert(iosb != nullptr);
        iosb->pending = true;
        return true;
    }
    else {
        // Actual error.
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (isOpen()) {
            report().error(u"error accepting TCP client: %s", SysErrorCodeMessage());
        }
        return false;
    }
}


//----------------------------------------------------------------------------
// Update the status of an asynchronous accept().
//----------------------------------------------------------------------------

bool ts::TCPServer::setAcceptStatus(TCPConnection& client, IPSocketAddress& client_address, IOSB* iosb)
{
#if defined(TS_WINDOWS)

    // Get the reception buffer from the IOSB.
    std::shared_ptr<AcceptAsyncBuffers> params;
    if (iosb != nullptr) {
        params = std::dynamic_pointer_cast<AcceptAsyncBuffers>(iosb->async_data);
    }
    if (params == nullptr) {
        iosb->error_code = SYS_ERROR;
        report().error(u"asynchronous I/O not used");
        return false;
    }

    // Get the address of GetAcceptExSockaddrs the first time we use it.
    // Thread-safe init-safe static data pattern.
    static const ::GUID getacceptex_guid = WSAID_GETACCEPTEXSOCKADDRS;
    static int getacceptex_error = 0;
    static const ::LPFN_GETACCEPTEXSOCKADDRS getacceptex = reinterpret_cast<::LPFN_GETACCEPTEXSOCKADDRS>(GetWSAFunction(getacceptex_guid, getacceptex_error));
    if (getacceptex == nullptr) {
        iosb->error_code = getacceptex_error;
        report().error(u"error fetching GetAcceptExSockaddrs: %s", SysErrorCodeMessage(getacceptex_error));
        return false;
    }

    // Complete the state of the client socket.
    ::SOCKET listener = getSocket();
    if (::setsockopt(client.getSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&listener), int(sizeof(listener))) != 0) {
        iosb->error_code = LastSysErrorCode();
        report().error(u"setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed: %s", SysErrorCodeMessage(iosb->error_code));
        return false;
    }

    // Extract address information from the returned buffer using GetAcceptExSockaddrs.
    ::sockaddr* local_addr = nullptr;
    ::sockaddr* remote_addr = nullptr;
    int local_len = 0;
    int remote_len = 0;
    getacceptex(params->buf, 0, AcceptAsyncBuffers::ADDR_BUFLEN, AcceptAsyncBuffers::ADDR_BUFLEN, &local_addr, &local_len, &remote_addr, &remote_len);
    if (remote_addr == nullptr || !client_address.set(*remote_addr)) {
        iosb->error_code = SYS_ERROR;
        report().error(u"cannot find client address after GetAcceptExSockaddrs");
        return false;
    }
    report().debug(u"received connection from %s", client_address);

    // Initialize the client socket.
    client.declareConnected();
    return true;

#else

    report().error(u"asynchronous I/O are not supported on this system");
    return false;

#endif
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TCPServer::closeImplementation(bool silent)
{
    // Shutdown server socket.
    // Do not report "not connected" errors since they are normal when the client disconnects first.
    if (::shutdown(getSocket(), SYS_SOCKET_SHUT_RDWR) != 0) {
        const int errcode = LastSysErrorCode();
        if (errcode != SYS_SOCKET_ERR_NOTCONN) {
            report().log(SilentLevel(silent), u"error shutdowning server socket: %s", SysErrorCodeMessage(errcode));
        }
    }

    // Then invoke superclass
    return SuperClass::closeImplementation(silent);
}


//----------------------------------------------------------------------------
// Windows asynchronous I/O parameters
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

// Virtual destructor.
ts::TCPServer::AcceptAsyncBuffers::~AcceptAsyncBuffers()
{
}

#endif
