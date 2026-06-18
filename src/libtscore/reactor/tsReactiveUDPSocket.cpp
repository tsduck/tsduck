//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveUDPSocket.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveUDPSocket::ReactiveUDPSocket(Reactor& reactor, UDPSocket& socket, Object* owner) :
    ReactiveSocketBase(reactor, socket, owner),
    _socket(socket)
{
}

ts::ReactiveUDPSocket::~ReactiveUDPSocket()
{
    // It is an error to destroy a reactive socket without waiting for asynchronous I/O.
    // Force a blocking wait on all pending asynchronous I/O.
    if constexpr (Reactor::UseAsynchronousIO()) {
        for (auto& iosb : _pending_send) {
            reactor().trace(u"warning: blocking wait for UDP pending asynchronous send");
            cancelAndWaitAsynchronousIO(*iosb, true);
        }
        if (_pending_receive != nullptr) {
            reactor().trace(u"warning: blocking wait for UDP pending asynchronous receive");
            cancelAndWaitAsynchronousIO(*_pending_receive, true);
        }
    }
}


//----------------------------------------------------------------------------
// Virtual destructors for request classes.
//----------------------------------------------------------------------------

ts::ReactiveUDPSocket::SendRequest::~SendRequest() {}
ts::ReactiveUDPSocket::ReceiveRequest::~ReceiveRequest() {}
ts::ReactiveUDPSocket::CloseRequest::~CloseRequest() {}


//----------------------------------------------------------------------------
// Process completed I/O operations (here, only close requests).
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::processQueuedOperations()
{
    // Complete a close request if no more I/O is pending.
    if (_pending_close != nullptr && _pending_send.empty() && _pending_receive == nullptr) {
        reactor().trace(u"ReactiveUDPSocket: completed close %X, socket %d", uintptr_t(this), _socket.getSocket());

        auto req = std::dynamic_pointer_cast<CloseRequest>(_pending_close->react_data);
        assert(req != nullptr);

        // Get a copy of the handler, req is going to be destroyed.
        auto handler = req->handler;
        bool silent = req->silent;
        auto user_data = _pending_close->app_data;

        // Close the underlying socket.
        _socket.close(silent);

        // Clear all states. Don't notify of anything else.
        _pending_send.clear();
        _pending_receive.reset();
        _pending_close.reset();
        _max_receive_size = 0;
        deactivateAll(silent);

        // Notify application.
        if (handler != nullptr) {
            handler->handleUDPClosed(*this, user_data);
        }
    }
}


//----------------------------------------------------------------------------
// Start the operation of sending a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination, const ObjectPtr& user_data)
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
    req->destination = destination;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    // Pending send request processing:
    // - With non-blocking I/O, we enqueue the request and it will be processed later in handleWriteReady().
    //   We shall not try to start it. If write suddenly becomes possible, we could successfully send a
    //   message while others are in the pending send queue. In case, messages would be sent in the wrong order.
    // - With asynchronous I/O, we need to start the send operation now. All send operations in the pending
    //   send queue are in progress or completed. Don't test if the operation completed, this will be done
    //   in handleAsynchronousIO().

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Enqueue the send request. Will be sent when write is possible.
        _pending_send.push_back(iosb);
        // Make sure to be notified when a send operation is possible.
        return activateWriteReady();
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Start the asynchronous send.
        if (!_socket.send(data, size, destination, iosb.get())) {
            // Failed to start the operation.
            return false;
        }
        // Add the send in the unordered set of send in progress.
        _pending_send.push_back(iosb);
    }
    return true;
}


//----------------------------------------------------------------------------
// Called in the Reactor context when a send operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleWriteReady(Reactor& reactor, EventId id, int error_code)
{
    // Keep count of sent messages for debug messages.
    const size_t pending_count = _pending_send.size();
    size_t sent_count = 0;

    // It is now possible to write on this socket, try to send all pending messages.
    while (!_pending_send.empty() && _socket.isOpen()) {

        // Remove the front send request.
        auto iosb = _pending_send.front();
        _pending_send.pop_front();

        // The send request is in the application data of the IOSB.
        auto req = std::dynamic_pointer_cast<SendRequest>(iosb->react_data);
        assert(req != nullptr);

        // Try to send the message.
        const bool success = _socket.send(req->data, req->size, req->destination, iosb.get());
        iosb->error_code = success ? SYS_SUCCESS : LastSysErrorCode();

        // If would block again, put request back where it was in the queue and retry later.
        if (iosb->pending) {
            _pending_send.push_front(iosb);
            break;
        }

        // Immediate success or error. Notify the application.
        sent_count += success;
        if (req->handler != nullptr) {
            req->handler->handleUDPSend(*this, req->data, req->size, req->destination, iosb->error_code, iosb->app_data);
        }
    }
    reactor.trace(u"UDP handle write: %d pending, %d sent", pending_count, sent_count);

    // When no more pending send operation, stop being notified of send-ready.
    if (_pending_send.empty()) {
        deactivateWriteReady(false);
    }
}


//----------------------------------------------------------------------------
// Start a receive operation, return an error code, SYS_SUCCESS on success.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::tryReceive(IOSB* iosb)
{
    assert(iosb != nullptr);
    auto req = std::dynamic_pointer_cast<ReceiveRequest>(iosb->react_data);
    assert(req != nullptr);

    const bool success = _socket.receive(req->data->data(), req->data->size(), req->recv_size,
                                         req->sender, req->destination, nullptr,
                                         &req->timestamp, &req->timestamp_type, iosb);

    iosb->error_code = success ? SYS_SUCCESS : LastSysErrorCode();
    return success;
}


//----------------------------------------------------------------------------
// Call the receive handler.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::callReceiveHandler(const std::shared_ptr<ReceiveRequest>& req, int error_code)
{
    assert(req != nullptr);

    // Update the received packet size.
    if (req->data == nullptr) {
        req->data = std::make_shared<ByteBlock>();
    }
    else {
        req->data->resize(std::min(req->recv_size, req->data->size()));
    }

    // Call the handler, except when the reception was explicitly canceled by the user.
    if (_receive_handler != nullptr && error_code != SYS_CANCELED) {
        _receive_handler->handleUDPReceive(*this, req->data, req->sender, req->destination, req->timestamp, req->timestamp_type, error_code, _receive_app_data);
        // Reset the shared pointer, the handler may have kept it for further usage and we don't want to reuse it.
        req->data.reset();
    }
}


//----------------------------------------------------------------------------
// Start the operation of receiving a message from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startReceive(ReactiveUDPHandlerInterface* handler, size_t max_message_size, const ObjectPtr& user_data)
{
    // Unlike send operation, the handler cannot be null because there is no other way to get a datagram.
    if (handler == nullptr) {
        report().error(u"internal error: null handler in ReactiveUDPSocket::startReceive");
        return false;
    }

    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Don't use empty or too small reception buffers.
    const size_t previous_receive_size = _max_receive_size;
    _max_receive_size = std::max<size_t>(16, max_message_size);
    _receive_handler = handler;
    _receive_app_data = user_data;

    // If there was already a reception in progress, don't start anything new.
    if (previous_receive_size > 0 || _pending_receive != nullptr) {
        return true;
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Start the first receive operation.
        auto req = std::make_shared<ReceiveRequest>(_max_receive_size);
        _pending_receive = std::make_shared<IOSB>();
        _pending_receive->react_data = req;
        return tryReceive(_pending_receive.get());
    }

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Non-blocking I/O model: Get notified when a reception operation is possible, if not already done.
        return activateReadReady();
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    size_t received = 0;

    // Loop on all received datagrams, until it would block.
    // When we should no longer read, _max_receive_size is set to zero.
    while (_max_receive_size > 0 && _socket.isOpen()) {

        // Build a receive request.
        auto req = std::make_shared<ReceiveRequest>(_max_receive_size);
        Socket::IOSB iosb;
        iosb.react_data = req;

        // Try to receive one datagram.
        if (!tryReceive(&iosb)) {
            // Receive error, call the handler and cancel the read notification.
            callReceiveHandler(req, iosb.error_code);
            _max_receive_size = 0;
            deactivateReadReady(true);
            break;
        }
        else if (iosb.pending) {
            // Would block, end of pending messages, retry later, when the Reactor invokes this method again.
            break;
        }
        else {
            // Actual datagram received, call the handler.
            received++;
            callReceiveHandler(req, iosb.error_code);
        }
    }
    reactor.trace(u"UDP handle read: %d received", received);
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size)
{
    // At this point, we only have an IOSB address, not a std::shared_ptr.
    std::shared_ptr<ReceiveRequest> recv;
    std::shared_ptr<SendRequest> send;

    if ((send = std::dynamic_pointer_cast<SendRequest>(iosb.react_data)) != nullptr) {
        // Got a completed send request.
        send->size = io_size;
        if (send->handler != nullptr) {
            send->handler->handleUDPSend(*this, send->data, send->size, send->destination, iosb.error_code, iosb.app_data);
        }

        // Remove the request from the set of I/O in progress.
        std::shared_ptr<IOSB> req = removeFromQueue(_pending_send, &iosb);
        if (req == nullptr) {
            report().error(u"unreferenced completed asynchronous UDP send request");
        }
    }
    else if ((recv = std::dynamic_pointer_cast<ReceiveRequest>(iosb.react_data)) != nullptr) {
        // Got a completed receive request.
        recv->recv_size = io_size;

        // Extract the reception parameters from the IOSB.
        if (SysSuccess(iosb.error_code)) {
            _socket.getReceiveStatus(&iosb, recv->sender, recv->destination, &recv->timestamp, &recv->timestamp_type);
        }

        // Call the user handler.
        callReceiveHandler(recv, iosb.error_code);

        // There is only one pending receive request at a time. Is it this one?
        if (_pending_receive == nullptr) {
            // Continuous receive was stopped, do nothing.
        }
        else if (_pending_receive.get() != &iosb) {
            report().error(u"unreferenced completed asynchronous UDP receive request");
        }
        else if (SysSuccess(iosb.error_code) && _max_receive_size > 0) {
            // Successful reception, restart a new one, with a new buffer.
            recv = std::make_shared<ReceiveRequest>(_max_receive_size);
            iosb.react_data = recv;
            if (!tryReceive(&iosb)) {
                // Error starting the receive operation. Report that to application.
                callReceiveHandler(recv, iosb.error_code);
                // Disable further receive.
                _pending_receive.reset();
                _max_receive_size = 0;
            }
        }
        else {
            // Error during previous receive, disable further receive.
            _pending_receive.reset();
            _max_receive_size = 0;
        }
    }
    else {
        report().error(u"unexpected completed asynchronous UDP request, not send, not receive");
    }

    // Process any completed I/O.
    processQueuedOperations();
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::cancelSendReceive(bool silent)
{
    // Zero means no active message reception.
    _max_receive_size = 0;

    // Cancel currently in progress I/O.
    cancelAsynchronousIO(silent);

    // Stop being notified of send/receive-ready.
    deactivateReadReady(silent);
    deactivateWriteReady(silent);

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Discard pending send requests, they are not started yet.
        _pending_send.clear();
    }
}


//----------------------------------------------------------------------------
// Start closing the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startClose(ReactiveUDPHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    reactor().trace(u"ReactiveUDPSocket: start close %X, socket %d", uintptr_t(this), _socket.getSocket());

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

    // Try to close in the reactor context. If I/O are in progress, this will be done when they complete.
    return signalQueuedOperations();
}
