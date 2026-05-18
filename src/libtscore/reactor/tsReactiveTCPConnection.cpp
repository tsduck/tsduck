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
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTCPConnection::ReactiveTCPConnection(Reactor& reactor, TCPConnection& socket, Object* owner) :
    ReactiveBase(reactor, socket, owner),
    _socket(socket)
{
}

ts::ReactiveTCPConnection::~ReactiveTCPConnection()
{
}


//----------------------------------------------------------------------------
// Virtual destructors for request classes.
//----------------------------------------------------------------------------

ts::ReactiveTCPConnection::ConnectRequest::~ConnectRequest() {}
ts::ReactiveTCPConnection::SendRequest::~SendRequest() {}
ts::ReactiveTCPConnection::ReceiveRequest::~ReceiveRequest() {}
ts::ReactiveTCPConnection::CloseRequest::~CloseRequest() {}


//----------------------------------------------------------------------------
// Process completed I/O operations.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::processCompletedIO()
{
    reactor().trace(u"ReactiveTCPConnection: processing %d completed I/O", _completed_io.size());

    // Process completed I/O.
    while (!_completed_io.empty()) {

        // Remove the front completed IOSB.
        auto iosb = _completed_io.front();
        _completed_io.pop_front();

        // The application data contains the request.
        std::shared_ptr<ConnectRequest> conn;
        std::shared_ptr<SendRequest> send;

        if ((conn = std::dynamic_pointer_cast<ConnectRequest>(iosb->react_data)) != nullptr) {
            // Completed connect request.
            if (conn->handler != nullptr) {
                conn->handler->handleTCPConnected(*this, iosb->error_code, iosb->app_data);
            }
        }
        else if ((send = std::dynamic_pointer_cast<SendRequest>(iosb->react_data)) != nullptr) {
            // Completed send request or send eof request.
            if (send->eof) {
                // The error code after a successful operation is SYS_EOF.
                iosb->error_code = _socket.closeWriter(send->silent) ? SYS_EOF : LastSysErrorCode();
            }
            if (send->handler != nullptr) {
                send->handler->handleTCPSend(*this, send->total_size, iosb->error_code, iosb->app_data);
            }
        }
        else {
            report().error(u"internal error in ReactiveTCPConnection: unexpected type of completed request");
        }
    }

    // If a close request is pending and no more I/O is pending, close.
    if (_pending_send.empty() && _pending_connect == nullptr && _pending_receive == nullptr && _pending_close != nullptr) {
        reactor().trace(u"ReactiveTCPConnection: completed close %X, socket %d", uintptr_t(this), _socket.getSocket());

        // Get a copy of the handler, req is going to be destroyed.
        auto req = std::dynamic_pointer_cast<CloseRequest>(_pending_close->react_data);
        assert(req != nullptr);
        HandlerType* handler = req->handler;
        bool silent = req->silent;
        auto app_data = _pending_close->app_data;

        // Close the underlying socket. This must be done in asynchronous I/O only. With
        // non-blocking I/O, this was done in startSend() because of potential connect in progress.
        if constexpr (Reactor::UseAsynchronousIO()) {
            _socket.close(silent);
        }

        // Clear all states. Don't notify of anything else.
        assert(_pending_send.empty());
        _completed_io.clear();
        _pending_receive.reset();
        _pending_close.reset();
        deactivateAll(silent);

        // Notify application.
        if (handler != nullptr) {
            handler->handleTCPClosed(*this, app_data);
        }
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

    if constexpr (Reactor::UseNonBlockingIO()) {
        // With non-blocking I/O, the connection may have immediately succeeded.
        if (iosb->pending) {
            // Connection in progress. Register the request.
            _pending_connect = iosb;
            // Make sure to be notified when a send operation is possible.
            return activateWriteReady();
        }
        else {
            // Connection already completed. Enqueue for processing.
            _completed_io.push_back(iosb);
            return signalCompletedIO();
        }
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Connection in progress or completed, there will be an asynchronous I/O completion. Register the request.
        _pending_connect = iosb;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start the operation of sending data over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startSend(ReactiveTCPConnectionHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Allocate a send request structure.
    auto req = std::make_shared<SendRequest>();
    req->handler = handler;
    req->data = data;
    req->size = size;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Enqueue the send request. Will be sent when write is possible.
        _pending_send.push_back(iosb);
        // Make sure to be notified when a send operation is possible.
        return activateWriteReady();
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Start the asynchronous send.
        if (!_socket.send(data, size, iosb.get())) {
            // Failed to start the operation.
            return false;
        }
        // The send operation is in progress and its completion will be notified later.
        _pending_send.push_back(iosb);
    }
    return true;
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.b
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startCloseWriter(ReactiveTCPConnectionHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Allocate a closeWriter request structure as a send request.
    auto req = std::make_shared<SendRequest>();
    req->handler = handler;
    req->eof = true;
    req->silent = silent;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    // Enqueue the closeWriter request.
    if (_pending_send.empty()) {
        // No pending send request, directly enqueue in completed I/O.
        _completed_io.push_back(iosb);
        return signalCompletedIO();
    }
    else {
        // Will be executed after all pending requests.
        _pending_send.push_back(iosb);
        return true;
    }
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
        _sent_bytes = 0;
        _pending_connect->error_code = error_code;
        _socket.setConnectStatus(_pending_connect.get(), error_code);
        _completed_io.push_back(_pending_connect);
        _pending_connect.reset();

        // In case of connection error, nothing can be sent.
        if (error_code != SYS_SUCCESS) {
            _pending_send.clear();
            return;
        }
    }

    // It is now possible to write on this socket, try to send all pending messages.
    while (_socket.isOpen() && !_pending_send.empty()) {

        // Remove the front send request.
        auto iosb = _pending_send.front();
        _pending_send.pop_front();

        // The send request is in the application data of the IOSB.
        auto req = std::dynamic_pointer_cast<SendRequest>(iosb->react_data);
        assert(req != nullptr);

        // EOF requests are directly enqueued in completed I/O.
        if (req->eof) {
            _completed_io.push_back(iosb);
            continue;
        }

        // Try to send the message.
        const bool success = _socket.send(req->data, req->size, iosb.get());
        if (iosb->pending) {
            // Would block again.
            assert(iosb->sent_size < req->size);
            if (iosb->sent_size > 0) {
                // The beginning of the data buffer was sent, split the send request in two.
                auto req0 = std::make_shared<SendRequest>();
                req0->handler = req->handler;
                req0->data = req->data;
                req0->size = iosb->sent_size;
                _sent_bytes += iosb->sent_size;
                req0->total_size = _sent_bytes;
                auto iosb0 = std::make_shared<IOSB>();
                iosb0->app_data = iosb->app_data;
                iosb0->react_data = req0;
                // The first part is moved to the completed I/O queue.
                _completed_io.push_back(iosb0);
                // Adjust the remaining data to be sent later.
                req->data = reinterpret_cast<const uint8_t*>(req->data) + iosb->sent_size;
                req->size -= iosb->sent_size;
            }
            // Put request back where it was in the queue and retry later.
            _pending_send.push_front(iosb);
            break;
        }
        else {
            // Immediate success or error.
            if (success) {
                _sent_bytes += req->size;
                req->total_size = _sent_bytes;
                iosb->error_code = SYS_SUCCESS;
            }
            else {
                iosb->error_code = LastSysErrorCode();
            }
            // Move the request to the completed queue.
            _completed_io.push_back(iosb);
        }
    }

    // Now process all completed sent messages.
    processCompletedIO();

    // When no more pending send operation, stop being notified of send-ready.
    if (_pending_connect == nullptr && _pending_send.empty()) {
        deactivateWriteReady(false);
    }
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPConnection::startReceive(ReactiveTCPConnectionHandlerInterface* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    // Unlike send operation, the handler cannot be null because there is no other way to get received data.
    if (handler == nullptr) {
        report().error(u"internal error: null handler in ReactiveTCPConnection::startReceive");
        return false;
    }

    // Don't use empty or too small reception buffers.
    buffer_size = std::max<size_t>(256, buffer_size);

    // Check if there is an existing read cycle in progress.
    std::shared_ptr<ReceiveRequest> req;
    if (_pending_receive != nullptr) {
        req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
    }
    if (req != nullptr) {
        // Simply update the request. Do not modify/resize the data buffer, an asynchronous I/O may be in progress.
        req->handler = handler;
        req->buffer_size = buffer_size;
        _pending_receive->app_data = user_data;
        return true;
    }

    // Asynchronous I/O model: Be notified of I/O completion.
    // Non-blocking I/O model: Get notified when a reception operation is possible, if not already done.
    if (!activateAsynchronousIO() || !activateReadReady()) {
        return false;
    }

    // Create the receive request
    req = std::make_shared<ReceiveRequest>();
    req->handler = handler;
    req->buffer_size = buffer_size;
    req->data.resize(buffer_size);
    _pending_receive = std::make_shared<IOSB>();
    _pending_receive->app_data = user_data;
    _pending_receive->react_data = req;

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Start the first receive operation. Even if it immediately completes, an asynchronous I/O completion will be posted.
        size_t retsize = 0;
        if (!_socket.receive(req->data.data(), req->data.size(), retsize, nullptr, _pending_receive.get())) {
            _pending_receive.reset();
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Called when a receive operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    // Loop on possible reception. Check that the socket is still open (can have been closed in a handler).
    while (_socket.isOpen() && _pending_receive != nullptr) {

        auto req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
        assert(req != nullptr);
        assert(req->next_read <= req->data.size());

        // Enlarge buffer if no room left.
        if (req->next_read == req->data.size()) {
            req->data.resize(req->data.size() + req->buffer_size);
        }

        // Try to receive once.
        size_t retsize = 0;
        if (!_socket.receive(req->data.data() + req->next_read, req->data.size() - req->next_read, retsize, nullptr, _pending_receive.get())) {
            // Receive error.
            req->new_data = true;
            _pending_receive->error_code = LastSysErrorCode();
            // Report the error to the application.
            processReceiveBuffer(_pending_receive, req);
            // Stop receiving.
            _pending_receive.reset();
            deactivateReadReady(true);
        }
        else if (_pending_receive->pending) {
            // No receive is possible now, retry later on read-notification.
            break;
        }
        else {
            // Receive completed successfully.
            req->new_data = true;
            req->next_read += retsize;
            // Successfully receiving zero means end of connection.
            _pending_receive->error_code = retsize == 0 ? SYS_EOF : SYS_SUCCESS;
            // Let the application process the buffer.
            processReceiveBuffer(_pending_receive, req);
        }
    }

    // Process any completed I/O.
    processCompletedIO();
}


//----------------------------------------------------------------------------
// Process receive buffer in the context of a Reactor handler.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::processReceiveBuffer(const std::shared_ptr<IOSB>& iosb, const std::shared_ptr<ReceiveRequest>& req)
{
    // Mark the buffer as processed.
    req->new_data = false;

    // Loop on calls to handler, when the handler uses only a part of the buffer.
    for (;;) {

        // Check if we have the required condition to call the handler.
        // Always call the handler once in case of error.
        bool call_handler = iosb->error_code != SYS_SUCCESS;
        if (!call_handler) {
            // Check user-requested conditions.
            if (req->control.min_next_size == NPOS) {
                // Need a specific delimiter byte in the received data.
                call_handler = req->data.find(req->control.next_delimiter, 0, req->next_read) != NPOS;
            }
            else {
                // Need a minimum amount of received data.
                call_handler = req->next_read > 0 && req->next_read >= req->control.min_next_size;
            }
        }
        if (!call_handler) {
            break;
        }

        // Default values for the input control, may be updated later by the handler.
        req->control.used_size = req->next_read;
        req->control.min_next_size = 0;
        req->control.next_delimiter = 0;

        // Resize the data buffer to the exact received size for the handler.
        // Keep the previous size, will restore it later, hoping that there will be no reallocation / memory move.
        const size_t previous_size = req->data.size();
        req->data.resize(req->next_read);

        // Call the handler with the content of the buffer.
        assert(req->handler != nullptr);
        req->handler->handleTCPReceive(*this, req->data, req->control, iosb->error_code, iosb->app_data);

        // In case of receive error, call the handler only once and cancel receive.
        if (iosb->error_code != SYS_SUCCESS) {
            deactivateReadReady(true); // non-blocking IO only
            _pending_receive->reset();
            break;
        }

        // Adjust unread data in the buffer.
        if (req->control.used_size < req->next_read) {
            // The buffer was not entirely read by the application, compact it.
            req->data.erase(0, req->control.used_size);
            req->next_read -= req->control.used_size;
        }
        else {
            // The buffer was entirely read, no need to move memory.
            req->next_read = 0;
        }

        // Restore previous buffer size, hoping that there will be no reallocation / memory move.
        req->data.resize(previous_size);
    }
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size)
{
    // At this point, we only have an IOSB, not a std::shared_ptr.
    // Our custom request is in the react_data of the IOSB.
    std::shared_ptr<ConnectRequest> conn;
    std::shared_ptr<ReceiveRequest> recv;
    std::shared_ptr<SendRequest> send;

    if ((conn = std::dynamic_pointer_cast<ConnectRequest>(iosb.react_data)) != nullptr) {
        // Got a completed connect request.
        // There is only one pending connect request at a time. Is it this one?
        if (_pending_connect.get() != &iosb) {
            report().error(u"unreferenced completed asynchronous TCP connect request");
        }
        else {
            // Update the request result.
            if (_pending_connect->error_code == SYS_SUCCESS && !_socket.setConnectStatus(&iosb, _pending_connect->error_code)) {
                _pending_connect->error_code = SYS_ERROR;
            }
            // Move the receive request into the completed queue.
            _completed_io.push_back(_pending_connect);
            _pending_connect.reset();
        }
    }
    else if ((send = std::dynamic_pointer_cast<SendRequest>(iosb.react_data)) != nullptr) {
        // Got a completed send request. Update the request.
        _sent_bytes += io_size;
        send->total_size = _sent_bytes;
        // Search for the request in the queue of pending send requests.
        std::shared_ptr<IOSB> req = removeFromQueue(_pending_send, &iosb);
        if (req == nullptr) {
            report().error(u"unreferenced completed asynchronous TCP send request");
        }
        else {
            // Enqueue completed send request.
            _completed_io.push_back(req);
            // If next send request is an eof request, also pass it to completed I/O queue.
            if (!_pending_send.empty()) {
                std::shared_ptr<SendRequest> next_send = std::dynamic_pointer_cast<SendRequest>(_pending_send.front()->react_data);
                if (next_send != nullptr && next_send->eof) {
                    _completed_io.push_back(_pending_send.front());
                    _pending_send.pop_front();
                }
            }
        }
    }
    else if ((recv = std::dynamic_pointer_cast<ReceiveRequest>(iosb.react_data)) != nullptr) {
        // Got a completed receive request.
        // There is only one pending receive request at a time. Is it this one?
        if (_pending_receive.get() != &iosb) {
            report().error(u"unreferenced completed asynchronous TCP receive request");
        }
        else if (_pending_receive->error_code == SYS_CANCELED) {
            // Ignore canceled receive requests. A receive operation is always active. When canceled, this is the
            // symptom of socket being closed. A send request, on the other hand, is an explicit message from the
            // application which expects a notification at the end of it, potentially canceled.
            _pending_receive.reset();
        }
        else {
            // Successfully receiving zero means end of connection.
            if (_pending_receive->error_code == SYS_SUCCESS && io_size == 0) {
                _pending_receive->error_code = SYS_EOF;
            }
            // Update the request result.
            recv->new_data = true;
            recv->next_read = std::min(recv->next_read + io_size, recv->data.size());
            // Directly call processReceiveBuffer() because we are in reactor handler context.
            processReceiveBuffer(_pending_receive, recv);
            // Start the next receive operation if necesary.
            if (_pending_receive == nullptr || _pending_receive->error_code != SYS_SUCCESS || !_socket.isConnected()) {
                // There was a receive error or the socket is closed, maybe from a handler, stop receiving.
                _pending_receive.reset();
            }
            else {
                // Enlarge buffer if no room left.
                if (recv->next_read >= recv->data.size()) {
                    recv->data.resize(recv->data.size() + recv->buffer_size);
                }
                // Start the next receive.
                size_t retsize = 0;
                if (!_socket.receive(recv->data.data() + recv->next_read, recv->data.size() - recv->next_read, retsize, nullptr, _pending_receive.get())) {
                    // Failed to start a new receive, stop receiving.
                    _pending_receive.reset();
                }
            }
        }
    }
    else {
        report().error(u"unexpected completed asynchronous TCP request, not connect, not send, not receive");
    }

    // Process any completed I/O.
    processCompletedIO();
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveTCPConnection::cancelSendReceive(bool silent)
{
    // Stop being notified of send/receive-ready.
    deactivateReadReady(silent);
    deactivateWriteReady(silent);

    // Cancel asynchronous I/O currently in progress.
    cancelAsynchronousIO(silent);

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Discard pending send requests, they are not started yet.
        _pending_send.clear();
    }
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
    cancelSendReceive(silent);

    // Disconnect the socket.
    _socket.disconnect(silent);

    if constexpr (Reactor::UseNonBlockingIO()) {
        // With non-blocking I/O, send/receive immediately succeed or fail. However, a connect() operation is
        // asynchronous and one can be in progress, for possibly a long time if the server is irresponsive.
        // Unlike asynchronous I/O on Windows, there is no way to cancel a connect() in progress. The only way
        // is to close the socket first, and then get the failure shortly afterward.
        _socket.close(silent);
    }

    // Notify the completion event to call the user's notification in the context of the reactor.
    // If no I/O is in progress, this will complete the close.
    return signalCompletedIO();
}
