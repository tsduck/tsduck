//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTCPConnection.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTCPConnection::ReactiveTCPConnection(Reactor& reactor, TCPConnection& socket) :
    ReactiveStream(reactor, socket, socket),
    _socket(socket)
{
    // Subscribe to our socket's events.
    _socket.addSubscription(this);
}

ts::ReactiveTCPConnection::~ReactiveTCPConnection()
{
}

ts::ReactiveTCPConnection::ConnectRequest::~ConnectRequest()
{
}

ts::ReactiveTCPConnection::CloseRequest::~CloseRequest()
{
}


//----------------------------------------------------------------------------
// Redirect all events from the underlying Socket to our own subscribers,
// except handleSocketCloseComplete().
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::handleSocketOpenStart(Socket& sock)
{
    callSubscribers<SocketHandlerInterface>([&sock](SocketHandlerInterface* subs) {
        subs->handleSocketOpenStart(sock);
    });
}

void ts::ReactiveTCPConnection::handleSocketOpenComplete(Socket& sock, bool success)
{
    callSubscribers<SocketHandlerInterface>([&sock, success](SocketHandlerInterface* subs) {
        subs->handleSocketOpenComplete(sock, success);
    });
}

void ts::ReactiveTCPConnection::handleSocketConnected(TCPConnection& sock)
{
    callSubscribers<SocketHandlerInterface>([&sock](SocketHandlerInterface* subs) {
        subs->handleSocketConnected(sock);
    });
}

void ts::ReactiveTCPConnection::handleSocketDisconnected(TCPConnection& sock, bool silent)
{
    callSubscribers<SocketHandlerInterface>([&sock, silent](SocketHandlerInterface* subs) {
        subs->handleSocketDisconnected(sock, silent);
    });
}

void ts::ReactiveTCPConnection::handleSocketCloseStart(Socket& sock, bool silent)
{
    callSubscribers<SocketHandlerInterface>([&sock, silent](SocketHandlerInterface* subs) {
        subs->handleSocketCloseStart(sock, silent);
    });
}


//----------------------------------------------------------------------------
// Try to interpret an IOSB as a valid completed I/O request and process.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::tryCompletedIO(const std::shared_ptr<IOSB>& iosb)
{
    // At the level of this class, we only handle connect operations.
    const std::shared_ptr<ConnectRequest> conn = std::dynamic_pointer_cast<ConnectRequest>(iosb->react_data);
    if (conn == nullptr) {
        // Don't know this request, try superclass.
        return ReactiveStream::tryCompletedIO(iosb);
    }
    else {
        // Completed connect request.
        if (conn->handler != nullptr) {
            conn->handler->handleTCPConnected(*this, iosb->error_code, iosb->app_data);
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Implement the close of the write direction of the stream.
//----------------------------------------------------------------------------

int ts::ReactiveTCPConnection::processCloseWriteStream(bool silent)
{
    return _socket.closeWriter(silent) ? SYS_SUCCESS : LastSysErrorCode();
}


//----------------------------------------------------------------------------
// Process completed I/O operations.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::processQueuedOperations()
{
    // Process completed I/O in superclass.
    ReactiveStream::processQueuedOperations();

    // If a close request is pending and no more I/O is pending, close.
    if (!hasPendingIO() && _pending_connect == nullptr && _pending_close != nullptr) {
        reactor().trace(u"ReactiveTCPConnection: completed close %X, socket %d", uintptr_t(this), _socket.getSocket());

        // Get a copy of the handler, req is going to be destroyed.
        auto req = std::dynamic_pointer_cast<CloseRequest>(_pending_close->react_data);
        assert(req != nullptr);
        ReactiveTCPConnectionHandlerInterface* handler = req->handler;
        bool silent = req->silent;
        auto app_data = _pending_close->app_data;

        // Close the underlying socket. This must be done in asynchronous I/O only. With non-blocking I/O,
        // this was done in startWriteStream() because of potential connect in progress.
        if constexpr (ReactorSupport::UseAsynchronousIO()) {
            _socket.close(silent);
        }

        // Clear all states. Don't notify of anything else.
        _pending_close.reset();
        deactivateAll(silent);

        // Notify application.
        if (handler != nullptr) {
            handler->handleTCPClosed(*this, app_data);
        }

        // Notify our subscribers.
        callSubscribers<SocketHandlerInterface>([this, silent](SocketHandlerInterface* subs) {
            subs->handleSocketCloseComplete(_socket, silent, true);
        });
    }
}


//----------------------------------------------------------------------------
// Handler to call when accepted as a client session by a server.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::whenAccepted(ReactiveTCPConnectionHandlerInterface* handler, const ObjectPtr& user_data)
{
    _accept_handler = handler;
    _accept_user_data = user_data;
}

void ts::ReactiveTCPConnection::declareConnected(ReactiveTCPServer& server, int error_code)
{
    if (_accept_handler != nullptr) {
        _accept_handler->handleTCPAccepted(server, *this, error_code, _accept_user_data);
    }
}


//----------------------------------------------------------------------------
// Start the operation of connecting to a TCP server.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startConnect(ReactiveTCPConnectionHandlerInterface* handler, const IPSocketAddress& addr, const ObjectPtr& user_data)
{
    if (_pending_connect != nullptr) {
        report().error(u"connection already in progress");
        return false;
    }

    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Allocate a send request structure.
    auto req = std::make_shared<ConnectRequest>();
    req->handler = handler;
    req->server = addr;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    // With non-blocking I/O (UNIX), connect() is different from send() and receive(). This is an asynchronous I/O.
    // It must be started now and its completion will be received when "writing" is possible on the socket (connect
    // is signaled as write ready with epoll and kqueue).

    // Try to start the connect operation now.
    if (!_socket.connect(req->server, iosb.get())) {
        // Error when trying to connect.
        return false;
    }

    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        // With non-blocking I/O, the connection may have immediately succeeded.
        if (iosb->pending) {
            // Connection in progress. Register the request.
            _pending_connect = iosb;
            // Make sure to be notified when a send operation is possible.
            return activateWriteReady();
        }
        else {
            // Connection already completed. Enqueue for processing.
            return enqueueCompletedIO(iosb, false);
        }
    }

    if constexpr (ReactorSupport::UseAsynchronousIO()) {
        // Connection in progress or completed, there will be an asynchronous I/O completion. Register the request.
        _pending_connect = iosb;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startCloseWriter(ReactiveStreamHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    return startCloseWriteStream(handler, silent, user_data);
}


//----------------------------------------------------------------------------
// Called in the Reactor context when a send operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::handleWriteReady(Reactor& reactor, EventId id, int error_code)
{
    // Process pending connection requests.
    if (_pending_connect != nullptr) {
        // The error code is valid with kqueue. With epoll, we need to fetch the error from the socket.
#if defined(TS_USE_EPOLL)
        if (error_code == SYS_ERROR) {
            ::socklen_t len = sizeof(error_code);
            if (::getsockopt(_socket.getSocket(), SOL_SOCKET, SO_ERROR, &error_code, &len) < 0) {
                error_code = errno; // error of getsockopt
            }
        }
#endif
        // Move the connect request to the completed queue.
        auto req = std::dynamic_pointer_cast<ConnectRequest>(_pending_connect->react_data);
        assert(req != nullptr);
        _pending_connect->error_code = error_code;
        _socket.setConnectStatus(_pending_connect.get(), error_code);
        enqueueCompletedIO(_pending_connect, true);
        _pending_connect.reset();

        // In case of connection error, nothing can be sent, cancel all I/O.
        if (!SysSuccess(error_code)) {
            cancelReadWriteStream(true);
            return;
        }
    }

    // Call the superclass for other write operations.
    ReactiveStream::handleWriteReady(reactor, id, error_code);
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size)
{
    // At this point, we only have an IOSB, not a std::shared_ptr<IOSB>.
    // Our custom request is in the react_data of the IOSB.
    std::shared_ptr<ConnectRequest> conn;

    if ((conn = std::dynamic_pointer_cast<ConnectRequest>(iosb.react_data)) != nullptr) {
        // Got a completed connect request.
        // There is only one pending connect request at a time. Is it this one?
        if (_pending_connect.get() != &iosb) {
            report().error(u"unreferenced completed asynchronous TCP connect request");
        }
        else {
            // Update the request result.
            if (SysSuccess(_pending_connect->error_code) && !_socket.setConnectStatus(&iosb, _pending_connect->error_code)) {
                _pending_connect->error_code = SYS_ERROR;
            }
            // Move the receive request into the completed queue.
            enqueueCompletedIO(_pending_connect, true);
            _pending_connect.reset();
        }
    }
    else {
        // Let superclass handles other types of asynchronous I/O completions.
        ReactiveStream::handleAsynchronousIO(reactor, id, iosb, io_size);
    }

    // Process any completed I/O.
    processQueuedOperations();
}


//----------------------------------------------------------------------------
// Start closing the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startClose(ReactiveTCPConnectionHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    reactor().trace(u"ReactiveTCPConnection: start close %X, socket %d", uintptr_t(this), _socket.getSocket());

    if (!_socket.isOpen()) {
        report().log(Socket::SilentLevel(silent), u"socket is not open");
        return false;
    }

    // If a close is already in progress, simply update the parameters.
    if (_pending_close != nullptr) {
        auto req = std::dynamic_pointer_cast<CloseRequest>(_pending_close->react_data);
        assert(req != nullptr);
        req->handler = handler;
        req->silent = silent;
        _pending_close->app_data = user_data;
        return true;
    }

    // Build the close request.
    auto req = std::make_shared<CloseRequest>();
    req->handler = handler;
    req->silent = silent;
    _pending_close = std::make_shared<IOSB>();
    _pending_close->app_data = user_data;
    _pending_close->react_data = req;

    // Cancel any pending I/O in the reactor.
    cancelReadWriteStream(silent);

    // Disconnect the socket.
    _socket.disconnect(silent);

    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        // With non-blocking I/O, send/receive immediately succeed or fail. However, a connect() operation is
        // asynchronous and one can be in progress, for possibly a long time if the server is irresponsive.
        // Unlike asynchronous I/O on Windows, there is no way to cancel a connect() in progress. The only way
        // is to close the socket first, and then get the failure shortly afterward.
        _socket.close(silent);
    }

    // Notify the completion event to call the user's notification in the context of the reactor.
    // If no I/O is in progress, this will complete the close.
    return signalQueuedOperations();
}
