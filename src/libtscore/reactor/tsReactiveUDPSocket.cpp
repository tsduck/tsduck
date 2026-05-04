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

ts::ReactiveUDPSocket::ReactiveUDPSocket(Reactor& reactor, UDPSocket& socket) :
    _reactor(reactor),
    _socket(socket)
{
    // The socket must be set in non-blocking mode, a reactor can never block.
    _socket.setNonBlocking(true);

    // Redirect all socket errors to the reactor.
    _socket.setReport(&_reactor);
}

ts::ReactiveUDPSocket::~ReactiveUDPSocket()
{
    if (_socket.isOpen()) {
        _reactor.trace(u"warning: ReactiveUDPSocket is destroyed while the underlying socket is still open");
    }

    // It is an error to destroy a reactive socket without waiting for asynchronous I/O.
    // Force a blocking wait on all pending asynchronous I/O.
    if constexpr (Reactor::UseAsynchronousIO()) {
        for (auto& iosb : _pending_send) {
            _reactor.trace(u"warning: blocking wait for UDP pending asynchronous send");
            _reactor.cancelAndWaitAsynchronousIO(_async_io_id, *iosb, true);
        }
        if (_pending_receive != nullptr) {
            _reactor.trace(u"warning: blocking wait for UDP pending asynchronous receive");
            _reactor.cancelAndWaitAsynchronousIO(_async_io_id, *_pending_receive, true);
        }
    }

    // Delete all registered ids in the reactor.
    deleteAllIds(true);
}


//----------------------------------------------------------------------------
// Virtual desctructors for request classes.
//----------------------------------------------------------------------------

ts::ReactiveUDPSocket::SendRequest::~SendRequest() {}
ts::ReactiveUDPSocket::ReceiveRequest::~ReceiveRequest() {}
ts::ReactiveUDPSocket::CloseRequest::~CloseRequest() {}


//----------------------------------------------------------------------------
// Delete and invalidate all registrations.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::deleteAllIds(bool silent)
{
    if (_completion_event.isValid()) {
        _reactor.deleteEvent(_completion_event, silent);
        _completion_event.invalidate();
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            _reactor.deleteAsynchronousIO(_async_io_id, silent);
            _async_io_id.invalidate();
        }
    }

    if constexpr (Reactor::UseNonBlockingIO()) {
        deleteReadReady(silent);
        deleteWriteReady(silent);
    }
}


//----------------------------------------------------------------------------
// Delete any read/write-ready (non-blocking I/O).
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::deleteReadReady(bool silent)
{
    if (_receive_ready_id.isValid()) {
        _reactor.deleteReadNotify(_receive_ready_id, silent);
        _receive_ready_id.invalidate();
    }
}

void ts::ReactiveUDPSocket::deleteWriteReady(bool silent)
{
    if (_send_ready_id.isValid()) {
        _reactor.deleteWriteNotify(_send_ready_id, silent);
        _send_ready_id.invalidate();
    }
}


//----------------------------------------------------------------------------
// Signal the completed event, so that processCompletedIO() is called in the Reactor context.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::signalCompletedIO()
{
    // The completion event is created the first time it is used.
    if (!_completion_event.isValid()) {
        _completion_event = _reactor.newEvent(this);
        if (!_completion_event.isValid()) {
            return false;
        }
    }
    return _reactor.signalEvent(_completion_event);
}


//----------------------------------------------------------------------------
// Called when an internal user-defined event is specified.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleUserEvent(Reactor& reactor, EventId id)
{
    // Process I/O completions.
    if (id == _completion_event) {
        processCompletedIO();
    }
}


//----------------------------------------------------------------------------
// Process completed I/O operations.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::processCompletedIO()
{
    _reactor.trace(u"ReactiveUDPSocket: processing %d completed I/O", _completed_io.size());

    while (!_completed_io.empty()) {

        // Remove the front completed IOSB.
        auto iosb(_completed_io.front());
        _completed_io.pop_front();

        // The application data contains the request.
        SendRequest* send = nullptr;
        ReceiveRequest* recv = nullptr;
        CloseRequest* clos = nullptr;

        if ((send = dynamic_cast<SendRequest*>(iosb->app_data.get())) != nullptr) {
            // Completed send request.
            if (send->handler != nullptr) {
                send->handler->handleUDPSend(*this, send->data, send->size, send->destination, send->error_code);
            }
        }
        else if ((recv = dynamic_cast<ReceiveRequest*>(iosb->app_data.get())) != nullptr) {
            // Completed receive request.
            if (_receive_handler != nullptr) {
                _receive_handler->handleUDPReceive(*this, recv->data, recv->sender, recv->destination, recv->timestamp, recv->timestamp_type, recv->error_code);
            }
            // In asynchronous I/O model, restart a new asynchronous receive operation.
            if constexpr (Reactor::UseAsynchronousIO()) {
                if (_max_receive_size > 0) {
                    startPendingReceive();
                }
            }
        }
        else if ((clos = dynamic_cast<CloseRequest*>(iosb->app_data.get())) != nullptr) {
            // Close request now ready.
            processCompletedClose(clos);
        }
        else {
            _reactor.report().error(u"internal error in ReactiveUDPSocket: unexpected type of completed request");
        }
    }

    // With asynchronous I/O, if a close request is pending and no more I/O is pending, close.
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_pending_send.empty() && _pending_receive == nullptr && _pending_close != nullptr) {
            processCompletedClose(dynamic_cast<CloseRequest*>(_pending_close->app_data.get()));
        }
    }
}


//----------------------------------------------------------------------------
// Start the operation of sending a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (!_async_io_id.isValid()) {
            _async_io_id = _reactor.newAsynchronousIO(this, _socket.getSocket());
        }
        if (!_async_io_id.isValid()) {
            return false;
        }
    }

    // Allocate a send request structure.
    SendRequest* req = new SendRequest;
    req->handler = handler;
    req->data = data;
    req->size = size;
    req->destination = destination;
    std::shared_ptr<IOSB> iosb(std::make_shared<IOSB>());
    iosb->app_data.reset(req);

    // Try to start the send operation now.
    if (_socket.send(data, size, destination, iosb.get())) {
        // Immediate success. Enqueue a completed request. Will be processed in a reactor handler.
        _completed_io.push_back(iosb);
        // Notify the completion event to call the user's notification in the context of the reactor.
        return signalCompletedIO();
    }
    else if (iosb->pending) {
        // Enqueue a pending send.
        _pending_send.push_back(iosb);
        // With asynchronous I/O, the send operation is already in progress and its completion will be notified later.
        // With non-blocking I/O, make sure to be notified when a send operation is possible.
        if constexpr (Reactor::UseNonBlockingIO()) {
            if (!_send_ready_id.isValid()) {
                _send_ready_id = _reactor.newWriteNotify(this, _socket.getSocket());
                if (!_send_ready_id.isValid()) {
                    return false;
                }
            }
        }
        return true;
    }
    else {
        // Actual error when trying to send.
        return false;
    }
}


//----------------------------------------------------------------------------
// Called in the Reactor context when a send operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleWriteReady(Reactor& reactor, EventId id, int error_code)
{
    if (id == _send_ready_id) {

        // Keep count of sent messages for debug messages.
        const size_t pending_count = _pending_send.size();
        size_t sent_count = 0;

        // It is now possible to write on this socket, try to send all pending messages.
        while (!_pending_send.empty()) {

            // Check that the socket is still open (can have been closed in a handler).
            if (!_socket.isOpen()) {
                break;
            }

            // Remove the front send request.
            auto iosb(_pending_send.front());
            _pending_send.pop_front();

            // The send request is in the application data of the IOSB.
            SendRequest* req = dynamic_cast<SendRequest*>(iosb->app_data.get());
            if (req == nullptr) {
                _reactor.report().error(u"internal error in ReactiveUDPSocket: unexpected type of pending send request");
                continue;
            }

            // Try to send the message.
            const bool success = _socket.send(req->data, req->size, req->destination, iosb.get());
            if (iosb->pending) {
                // Would block again, put request back where it was in the queue and retry later.
                _pending_send.push_front(iosb);
                break;
            }
            else {
                // Immediate success or error. Move the request to the completed queue.
                if (success) {
                    sent_count++;
                    req->error_code = SYS_SUCCESS;
                }
                else {
                    req->error_code = LastSysErrorCode();
                }
                _completed_io.push_back(iosb);
            }
        }
        _reactor.trace(u"UDP handle write: %d pending, %d sent", pending_count, sent_count);

        // Now process all completed sent messages.
        processCompletedIO();

        // When no more pending send operation, stop being notified of send-ready.
        if (_pending_send.empty()) {
            _reactor.deleteWriteNotify(_send_ready_id);
            _send_ready_id.invalidate();
        }
    }
}


//----------------------------------------------------------------------------
// Allocate the and start the pending receive request.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startPendingReceive()
{
    ReceiveRequest* req = new ReceiveRequest;
    req->data = std::make_shared<ByteBlock>(_max_receive_size);
    _pending_receive = std::make_shared<IOSB>();
    _pending_receive->app_data.reset(req);

    size_t retsize = 0;
    bool success = _socket.receive(req->data->data(), req->data->size(), retsize, req->sender, req->destination, nullptr, &req->timestamp, &req->timestamp_type, _pending_receive.get());
    if (success) {
        // The receive operation completed immediately.
        req->data->resize(std::min(retsize, req->data->size()));
        // Move it to completed I/O queue.
        _completed_io.push_back(_pending_receive);
        _pending_receive.reset();
    }
    return success;
}


//----------------------------------------------------------------------------
// Start the operation of receiving a message from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startReceive(ReactiveUDPHandlerInterface* handler, size_t max_message_size)
{
    // Unlike send operation, the handler cannot be null because there is no other way to get a datagram.
    if (handler == nullptr) {
        _reactor.report().error(u"internal error: null handler in UDPSocket::TimeStampType");
        return false;
    }

    _receive_handler = handler;

    // Don't use empty or too small reception buffers.
    const size_t previous_receive_size = _max_receive_size;
    _max_receive_size = std::max<size_t>(16, max_message_size);

    // If there was already a reception in progress, don't start anything new.
    if (previous_receive_size > 0) {
        return true;
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        // Asynchronous I/O model: Get notified of I/O completion, if not already done.
        if (!_async_io_id.isValid()) {
            _async_io_id = _reactor.newAsynchronousIO(this, _socket.getSocket());
            if (!_async_io_id.isValid()) {
                return false;
            }
        }
        // Start the first receive operation.
        if (startPendingReceive()) {
            // Receive operation immediately completed.
            // Notify the completion event to call the user's notification in the context of the reactor.
            // The next receive operation will be started after processing this one.
            return signalCompletedIO();
        }
        else {
            // The receive operation is either in progress or failed to start.
            return _pending_receive->pending;
        }
    }

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Non-blocking I/O model: Get notified when a reception operation is possible, if not already done.
        if (!_receive_ready_id.isValid()) {
            _receive_ready_id = _reactor.newReadNotify(this, _socket.getSocket());
        }
        return _receive_ready_id.isValid();
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    if (id == _receive_ready_id) {

        assert(_receive_handler != nullptr);
        size_t received = 0;

        // Loop on all received datagrams, until it would block.
        // When we should no longer read, _max_receive_size is set to zero.
        while (_max_receive_size > 0) {

            // Check that the socket is still open (can have been closed in a handler).
            if (!_socket.isOpen()) {
                break;
            }

            // Try to receive one datagram.
            ReceiveRequest req;
            req.data = std::make_shared<ByteBlock>(_max_receive_size);
            size_t retsize = 0;
            Socket::IOSB iosb;
            bool success = _socket.receive(req.data->data(), req.data->size(), retsize, req.sender, req.destination, nullptr, &req.timestamp, &req.timestamp_type, &iosb);

            if (iosb.pending) {
                // End of pending messages, retry later, when the Reactor invokes this method again.
                break;
            }
            else if (_receive_handler == nullptr) {
                // Nothing to report.
            }
            else if (success) {
                // Actual datagram received, call the handler.
                received++;
                req.data->resize(std::min(retsize, req.data->size()));
                _receive_handler->handleUDPReceive(*this, req.data, req.sender, req.destination, req.timestamp, req.timestamp_type, SYS_SUCCESS);
            }
            else {
                // Receive error, call the handler and cancel the read notification.
                error_code = LastSysErrorCode();
                req.data->clear();
                _receive_handler->handleUDPReceive(*this, req.data, IPSocketAddress(), IPSocketAddress(), cn::microseconds::zero(), UDPSocket::TimeStampType::NONE, error_code);
                _max_receive_size = 0;
                deleteReadReady(true);
                break;
            }
        }
        _reactor.trace(u"UDP handle read: %d received", received);
    }
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size, int error_code)
{
    if (id == _async_io_id) {
        // At this point, we only have an IOSB, not a std::shared_ptr.
        ReceiveRequest* recv = nullptr;
        SendRequest* send = dynamic_cast<SendRequest*>(iosb.app_data.get());
        if (send != nullptr) {
            // Got a completed send request. Update the request.
            send->size = io_size;
            send->error_code = error_code;
            // Search for the request in the queue of pending send requests.
            // Use reverse iterator since the completed I/O is likely on the front.
            std::shared_ptr<IOSB> req;
            for (auto it = _pending_send.rbegin(); it != _pending_send.rend(); ++it) {
                if (it->get() == &iosb) {
                    req = *it;
                    // C++ trick to erase using a reverse iterator.
                    _pending_send.erase(std::next(it).base());
                    break;
                }
            }
            if (req == nullptr) {
                _reactor.report().error(u"unreferenced completed asynchronous UDP send request");
            }
            else {
                _completed_io.push_back(req);
            }
        }
        else if ((recv = dynamic_cast<ReceiveRequest*>(iosb.app_data.get())) != nullptr) {
            // Got a completed receive request.
            // There is only one pending receive request at a time. Is it this one?
            if (_pending_receive.get() != &iosb) {
                _reactor.report().error(u"unreferenced completed asynchronous UDP receive request");
            }
            else {
                // Ignore canceled receive requests. A receive operation is always active. When canceled, this is the
                // symptom of socket being closed. A send request, on the other hand, is an explicit message from the
                // application which expects a notification at the end of it, potentially canceled.
                if (error_code != SYS_CANCELED) {
                    // Update the request result.
                    recv->error_code = error_code;
                    if (recv->data != nullptr) {
                        // Update the received packet size.
                        recv->data->resize(std::min(io_size, recv->data->size()));
                    }
                    if (error_code == SYS_SUCCESS) {
                        _socket.getReceiveStatus(&iosb, recv->sender, recv->destination, &recv->timestamp, &recv->timestamp_type);
                    }
                    // Move the receive request into the completed queue.
                    _completed_io.push_back(_pending_receive);
                }
                // Passed to the application or canceled, we need to clear the current receive request.
                _pending_receive.reset();
            }
        }
        else {
            _reactor.report().error(u"unexpected completed asynchronous UDP request, not send, not receive");
        }

        // Process any completed I/O.
        processCompletedIO();
    }
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::cancelSendReceive(bool silent)
{
    // Zero means no active message reception.
    _max_receive_size = 0;

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Stop being notified of send/receive-ready.
        deleteReadReady(silent);
        deleteWriteReady(silent);

        // Discard pending send requests, they are not started yet.
        _pending_send.clear();
    }

    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            // Cancel currently in progress I/O and wait for their completion.
            _reactor.cancelAsynchronousIO(_async_io_id, silent);
        }
    }
}


//----------------------------------------------------------------------------
// Start closing the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startClose(ReactiveUDPHandlerInterface* handler, bool silent)
{
    _reactor.trace(u"ReactiveUDPSocket: start close %X, socket %d", uintptr_t(this), _socket.getSocket());

    if (!_socket.isOpen()) {
        _reactor.report().log(Socket::SilentLevel(silent), u"socket is not open");
        return false;
    }

    // If a close is already in progress, simply update the parameters.
    if (_pending_close != nullptr) {
        CloseRequest* req = dynamic_cast<CloseRequest*>(_pending_close->app_data.get());
        assert(req != nullptr);
        req->handler = handler;
        req->silent = silent;
        return true;
    }

    // Build the close request.
    CloseRequest* req = new CloseRequest;
    req->handler = handler;
    req->silent = silent;
    _pending_close = std::make_shared<IOSB>();
    _pending_close->app_data.reset(req);

    // Cancel any pending I/O in the reactor.
    cancelSendReceive(silent);

    // With non-blocking I/O, nothing is in progress, we can directly enqueue a close notification.
    if constexpr (Reactor::UseNonBlockingIO()) {
        _completed_io.push_back(_pending_close);
        _pending_close.reset();
        // Notify the completion event to call the user's notification in the context of the reactor.
        return signalCompletedIO();
    }

    // With asynchronous I/O, we must take care of I/O in progress.
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (!_pending_send.empty() || _pending_receive != nullptr) {
            // Cancel currently in progress I/O and wait for their completion.
            _reactor.cancelAsynchronousIO(_async_io_id, silent);
        }
        else {
            // No I/O in progress, directly enqueue a close notification.
            _completed_io.push_back(_pending_close);
            _pending_close.reset();
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Process a completed close.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::processCompletedClose(CloseRequest* req)
{
    _reactor.trace(u"ReactiveUDPSocket: completed close %X, socket %d", uintptr_t(this), _socket.getSocket());

    // Get a copy of the handler, req is going to be destroyed.
    auto handler = req->handler;
    bool silent = req->silent;

    // Close the underlying socket.
    _socket.close(silent);

    // Clear all states. Don't notify of anything else.
    assert(_pending_send.empty());
    _completed_io.clear();
    _pending_receive.reset();
    _pending_close.reset();
    _max_receive_size = 0;
    deleteAllIds(silent);

    // Notify application.
    if (handler != nullptr) {
        handler->handleUDPClosed(*this);
    }
}
