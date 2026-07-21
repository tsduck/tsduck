//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTCPConnection.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"
#include "tsInitZero.h"
#include "tsException.h"
#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TCPConnection::TCPConnection(Report* report, bool non_blocking) :
    TCPSocket(report, non_blocking)
{
}

ts::TCPConnection::TCPConnection(ReporterBase* delegate, bool non_blocking) :
    TCPSocket(delegate, non_blocking)
{
}


//----------------------------------------------------------------------------
// Called by a server to declare that the socket has just become opened.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareOpened(SysSocketType sock)
{
    // This will be a server-side connection.
    _is_server_side = true;

    SuperClass::declareOpened(sock);
}


//----------------------------------------------------------------------------
// This method is used by specific subclass to declare that the socket
// has just become connected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareConnected()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_is_connected) {
        report().fatal(u"implementation error: TCP socket already connected");
        throw ImplementationError(u"TCP socket already connected");
    }
    else {
        // Declare the socket as connected.
        _is_connected = true;
        _end_of_input = false;

        // Notify all subscribers that the socket is connected.
        callSubscribers<SocketHandlerInterface>([this](SocketHandlerInterface* subs) {
            subs->handleSocketConnected(*this);
        });
    }
}


//----------------------------------------------------------------------------
// Declare that the socket has just become disconnected.
//----------------------------------------------------------------------------

void ts::TCPConnection::declareDisconnected(bool silent)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Silently ignore socket already disconnected.
    if (_is_connected) {
        // Declare the socket as disconnected.
        _is_connected = false;

        // Notify all subscribers that the socket is disconnected.
        callSubscribers<SocketHandlerInterface>([this, silent](SocketHandlerInterface* subs) {
            subs->handleSocketDisconnected(*this, silent);
        });
    }
}


//----------------------------------------------------------------------------
// Close the socket
//----------------------------------------------------------------------------

bool ts::TCPConnection::closeImplementation(bool silent)
{
    // Declare the socket disconnected, if not properly disconnected.
    declareDisconnected(silent);

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return SuperClass::closeImplementation(silent);
}


//----------------------------------------------------------------------------
// Get connected peer.
//----------------------------------------------------------------------------

bool ts::TCPConnection::getPeer(IPSocketAddress& peer)
{
    InitZero<::sockaddr_storage> sock_addr;
    SysSocketLengthType len = sizeof(sock_addr.data);
    if (::getpeername(getSocket(), reinterpret_cast<::sockaddr*>(&sock_addr.data), &len) != 0) {
        report().error(u"error getting socket peer: %s", SysErrorCodeMessage());
        return false;
    }
    peer = IPSocketAddress(sock_addr.data);
    return true;
}

ts::UString ts::TCPConnection::peerName()
{
    // Get peer socket without error message.
    IPSocketAddress peer;
    const bool mute = muteReport(true);
    const bool success = getPeer(peer);
    muteReport(mute);
    return success ? peer.toString() : u"";
}


//----------------------------------------------------------------------------
// Detect end of input stream. Implementation of StreamInterface.
//----------------------------------------------------------------------------

bool ts::TCPConnection::endOfStream()
{
    return !_is_connected || _end_of_input;
}


//----------------------------------------------------------------------------
// Send data. Implementation of StreamInterface.
//----------------------------------------------------------------------------

bool ts::TCPConnection::writeStream(const void* buffer, size_t data_size, IOSB* iosb)
{
    return WriteStreamHelper<TCPConnection>(this, buffer, data_size, iosb);
}

bool ts::TCPConnection::writeStream(const void* buffer, size_t size, size_t& written_size, IOSB* iosb)
{
    written_size = 0;

    // Check that the application uses the right blocking mode.
    if (!checkNonBlocking(iosb, u"TCPConnection::writeStream")) {
        return false;
    }

#if defined(TS_WINDOWS)
    // On Windows with asynchronous I/O, use overlapped I/O.
    // With standard blocking I/O, use the same standard socket calls as UNIX.
    if (isNonBlocking()) {
        assert(iosb != nullptr);

        // The reception parameters are stored in the IOSB.
        auto params = std::make_shared<TCPAsyncBuffers>(SocketOp::SEND);
        TS_ZERO(params->buf);
        params->buf.buf = reinterpret_cast<CHAR*>(const_cast<void*>(buffer));
        params->buf.len = ::ULONG(size);
        iosb->async_data = params;

        // Start an asynchronous I/O.
        // Consider that the I/O is pending if it immediately completed because an asynchronous I/O completion will be posted.
        int err = SYS_SUCCESS;
        if (::WSASend(getSocket(), &params->buf, 1, nullptr, 0, &iosb->overlap, nullptr) != 0) {
            err = LastSysErrorCode();
        }
        iosb->pending = SysSuccess(err) || IsPendingStatus(err);
        if (!iosb->pending) {
            report().error(u"error sending data to socket: %s", SysErrorCodeMessage(err));
        }
        return iosb->pending;
    }
#endif

    // Standard socket API.
    const char* data = reinterpret_cast<const char*>(buffer);
    size_t remain = size;

    while (remain > 0) {
        SysSocketSignedSizeType gone = ::send(getSocket(), SysSendBufferPointer(data), int(remain), 0);
        const int err_code = LastSysErrorCode();
        if (gone > 0) {
            assert(size_t(gone) <= remain);
            data += gone;
            written_size += gone;
            remain -= gone;
        }
#if defined(TS_UNIX)
        else if (err_code == EINTR) {
            // Ignore signal, retry
            report().debug(u"send() interrupted by signal, retrying");
        }
#endif
        else if (isNonBlocking() && IsPendingStatus(err_code)) {
            // UNIX: Non-blocking socket with no enough available outgoing buffer space.
            // Windows: Asynchronous I/O in progress.
            assert(iosb != nullptr);
            iosb->pending = true;
            iosb->sent_size = written_size;
            return true;
        }
        else {
            report().error(u"error sending data to socket: %s", SysErrorCodeMessage(err_code));
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Receive data. Implementation of StreamInterface.
//----------------------------------------------------------------------------

bool ts::TCPConnection::readStream(void* buffer, size_t size, const AbortInterface* abort)
{
    return ReadStreamHelper<TCPConnection>(this, buffer, size, abort);
}

bool ts::TCPConnection::readStream(void* data, size_t max_size, size_t& ret_size, const AbortInterface* abort, IOSB* iosb)
{
    // Clear returned values
    ret_size = 0;

    // Check that the application uses the right blocking mode.
    if (!checkNonBlocking(iosb, u"TCPConnection::receive")) {
        return false;
    }

#if defined(TS_WINDOWS)
    // On Windows with asynchronous I/O, use overlapped I/O.
    // With standard blocking I/O, use the same standard socket calls as UNIX.
    if (isNonBlocking()) {
        assert(iosb != nullptr);

        // The reception parameters are stored in the IOSB.
        auto params = std::make_shared<TCPAsyncBuffers>(SocketOp::RECEIVE);
        TS_ZERO(params->buf);
        params->buf.buf = reinterpret_cast<CHAR*>(data);
        params->buf.len = ::ULONG(max_size);
        params->flags = 0;
        iosb->async_data = params;

        // Start an asynchronous I/O.
        // Consider that the I/O is pending if it immediately completed because an asynchronous I/O completion will be posted.
        int err = SYS_SUCCESS;
        ::DWORD io_size = 0;
        if (::WSARecv(getSocket(), &params->buf, 1, nullptr, &params->flags, &iosb->overlap, nullptr) != 0) {
            err = LastSysErrorCode();
        }
        iosb->pending = SysSuccess(err) || IsPendingStatus(err);
        if (!iosb->pending) {
            static const std::set<::DWORD> eof_status {WSAEDISCON, WSAECONNRESET, WSAECONNABORTED, WSAENETRESET, WSA_OPERATION_ABORTED};
            if (eof_status.contains(err)) {
                // End of connection (graceful or aborted). Do not report an error.
                declareDisconnected(true);
                SetLastSysErrorCode(SYS_EOF);
            }
            else {
                // Actual error.
                report().error(u"error receiving data from socket: %s", SysErrorCodeMessage(err));
            }
            // In all cases, input error means end of input.
            _end_of_input = true;
        }
        return iosb->pending;
    }
#endif

    // Standard socket API.
    // Loop on unsollicited interrupts
    for (;;) {
        SysSocketSignedSizeType got = ::recv(getSocket(), SysRecvBufferPointer(data), int(max_size), 0);
        const int err_code = LastSysErrorCode();
        if (got > 0) {
            // Received some data
            assert(size_t(got) <= max_size);
            ret_size = size_t(got);
            return true;
        }
        else if (got == 0 || err_code == SYS_SOCKET_ERR_RESET) {
            // End of connection (graceful or aborted). Do not report an error.
            declareDisconnected(true);
            SetLastSysErrorCode(SYS_EOF);
            return false;
        }
        else if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return false;
        }
#if defined(TS_UNIX)
        else if (err_code == EINTR) {
            // Ignore signal, retry.
            report().debug(u"recv() interrupted by signal, retrying");
        }
#endif
        else if (isNonBlocking() && IsPendingStatus(err_code)) {
            // UNIX: Non-blocking socket with no available incoming data.
            assert(iosb != nullptr);
            iosb->pending = true;
            return true;
        }
        else {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            if (isOpen()) {
                // Report the error only if the error does not result from a close in another thread.
                report().error(u"error receiving data from socket: %s", SysErrorCodeMessage(err_code));
                // In all cases, input error means end of input.
                _end_of_input = true;
            }
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
//----------------------------------------------------------------------------

bool ts::TCPConnection::connect(const IPSocketAddress& addr, IOSB* iosb)
{
    // Check that the application uses the right blocking mode.
    if (!checkNonBlocking(iosb, u"TCPConnection::connect")) {
        return false;
    }

    // Convert server address in preferred IP generation.
    IPSocketAddress server_addr(addr);
    if (!convert(server_addr)) {
        return false;
    }

    // This will be a client-side connection.
    _is_server_side = false;

#if defined(TS_WINDOWS)
    // On Windows with asynchronous I/O, use overlapped I/O.
    // With standard blocking I/O, use the same standard socket calls as UNIX.
    if (isNonBlocking()) {
        assert(iosb != nullptr);

        // Get the address of ConnectEx the first time we use it.
        // Thread-safe init-safe static data pattern.
        static const ::GUID connect_ex_guid = WSAID_CONNECTEX;
        static int connect_ex_error = 0;
        static const ::LPFN_CONNECTEX connect_ex = reinterpret_cast<::LPFN_CONNECTEX>(GetWSAFunction(connect_ex_guid, connect_ex_error));
        if (connect_ex == nullptr) {
            report().error(u"error fetching ConnectEx: %s", SysErrorCodeMessage(connect_ex_error));
            return false;
        }

        // The reception parameters are stored in the IOSB.
        auto params = std::make_shared<TCPAsyncBuffers>(SocketOp::CONNECT);
        TS_ZERO(params->peer_sock);
        params->peer_sock_len = int(server_addr.get(params->peer_sock));
        iosb->async_data = params;

        // Start an asynchronous connect. Don't send any data with the connection.
        // Consider that the I/O is pending if it immediately completed because an asynchronous I/O completion will be posted.
        int err = SYS_SUCCESS;
        if (!connect_ex(getSocket(), reinterpret_cast<::sockaddr*>(&params->peer_sock), params->peer_sock_len, nullptr, 0, nullptr, &iosb->overlap)) {
            err = LastSysErrorCode();
        }
        iosb->pending = SysSuccess(err) || IsPendingStatus(err);
        if (!iosb->pending) {
            report().error(u"error connecting socket: %s", SysErrorCodeMessage(err));
        }
        return iosb->pending;
    }
#endif

    // Standard socket API.
    // Loop on unsollicited interrupts
    for (;;) {
        ::sockaddr_storage sock_addr;
        const size_t sock_size = server_addr.get(sock_addr);
        report().debug(u"connecting to %s", server_addr);
        const bool success = ::connect(getSocket(), reinterpret_cast<const ::sockaddr*>(&sock_addr), socklen_t(sock_size)) == 0;
        const int err = LastSysErrorCode();

        if (success) {
            declareConnected();
            return true;
        }
#if defined(TS_UNIX)
        else if (err == EINTR) {
            // Ignore signal, retry
            report().debug(u"connect() interrupted by signal, retrying");
        }
#endif
        else if (isNonBlocking() && IsPendingStatus(err)) {
            // UNIX: Non-blocking socket with no immediate connection.
            // Windows: Asynchronous I/O in progress.
            assert(iosb != nullptr);
            iosb->pending = true;
            return true;
        }
        else {
            report().error(u"error connecting socket: %s", SysErrorCodeMessage(err));
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Update the status of an asynchronous receive().
//----------------------------------------------------------------------------

bool ts::TCPConnection::setConnectStatus(IOSB* iosb, int error_code)
{
#if defined(TS_WINDOWS)

    std::shared_ptr<TCPAsyncBuffers> params;
    if (iosb != nullptr) {
        params = std::dynamic_pointer_cast<TCPAsyncBuffers>(iosb->async_data);
    }
    if (params == nullptr) {
        report().error(u"asynchronous I/O not used");
        return false;
    }
    else if (setsockopt(getSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) != 0) {
        report().error(u"setsockopt(SO_UPDATE_CONNECT_CONTEXT) failed: %s", SysErrorCodeMessage());
        return false;
    }
    else {
        declareConnected();
        return true;
    }

#else

    if (error_code == SYS_SUCCESS) {
        declareConnected();
        return true;
    }
    else {
        return false;
    }

#endif
}


//----------------------------------------------------------------------------
// Shutdown the socket.
//----------------------------------------------------------------------------

bool ts::TCPConnection::shutdownSocket(int how, bool silent)
{
    if (::shutdown(getSocket(), how) != 0) {
        const int errcode = LastSysErrorCode();
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        // Do not report "not connected" errors since they are normal when the peer disconnects first.
        if (isOpen() && errcode != SYS_SOCKET_ERR_NOTCONN) {
            report().log(SilentLevel(silent), u"error shutting down socket: %s", SysErrorCodeMessage(errcode));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TCPConnection::closeWriter(bool silent)
{
    report().debug(u"closing socket writer");
    return shutdownSocket(SYS_SOCKET_SHUT_WR, silent);
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TCPConnection::disconnect(bool silent)
{
    declareDisconnected(silent);
    report().debug(u"disconnecting socket");
    return shutdownSocket(SYS_SOCKET_SHUT_RDWR, silent);
}


//----------------------------------------------------------------------------
// Windows asynchronous I/O parameters
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

// Virtual destructor.
ts::TCPConnection::TCPAsyncBuffers::~TCPAsyncBuffers()
{
}

#endif
