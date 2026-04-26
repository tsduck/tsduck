//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveUDPSocket.h"
#include "tsSysUtils.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveUDPSocket::ReactiveUDPSocket(Reactor& reactor, UDPSocket& socket) :
    _reactor(reactor),
    _socket(socket)
{
    _socket.setNonBlocking(true);
    _socket.setReport(&_reactor.report());

    // Make sure that the associated Socket notifies us of its open() and close().
    _socket.addSubscription(this);
}

ts::ReactiveUDPSocket::~ReactiveUDPSocket()
{
    // Cancel pending events.
    if (_send_event.isValid()) {
        _reactor.deleteWriteNotify(_send_event, true);
        _send_event.invalidate();
    }
    if (_receive_event.isValid()) {
        _reactor.deleteReadNotify(_receive_event, true);
        _receive_event.invalidate();
    }
    if (_completion_event.isValid()) {
        _reactor.deleteEvent(_completion_event, true);
        _completion_event.invalidate();
    }
    // If opened, the socket will be closed in the superclass' destructor.
}


//----------------------------------------------------------------------------
// Handlers which are invoked when the socket is opened or closed.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleSocketOpen(Socket& sock)
{
    _reactor.trace(u"ReactiveUDPSocket::handleSocketOpen");
}

void ts::ReactiveUDPSocket::handleSocketClose(Socket& sock)
{
    _reactor.trace(u"ReactiveUDPSocket::handleSocketClose");

    // Cancel any pending I/O in the reactor.
    // Mute error messages because the socket has just been closed.
    cancelSend(true);
    cancelReceive(true);
}


//----------------------------------------------------------------------------
// Called when a user-defined event is specified.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleUserEvent(Reactor& reactor, EventId id)
{
    // Process I/O completions.
    if (id == _completion_event) {
        processCompletedWrite();
    }
}


//----------------------------------------------------------------------------
// Start the operation of sending a message to a destination address and port.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startSend(ReactiveUDPSendHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination)
{
    // Allocate a request structure.
    std::shared_ptr<SendRequest> req(std::make_shared<SendRequest>());
    req->handler = handler;
    req->data = data;
    req->size = size;
    req->destination = destination;

    // Try to start the send operation now.
    Socket::IOSB iosb;
    if (_socket.send(data, size, destination, &iosb)) {
        // Immediate success.
        // Notify the completion event to call the user's notification in the context of the reactor.
        // The completion event is created the first time it is used.
        if (!_completion_event.isValid()) {
            _completion_event = _reactor.newEvent(this);
            if (!_completion_event.isValid()) {
                return false;
            }
        }
        if (!_reactor.signalEvent(_completion_event)) {
            return false;
        }
        // Enqueue a completed request. Will be processed in the _completion_event handler.
        _completed_send.push_back(req);
        return true;
    }
    else if (iosb.pending) {
        // Be notified when a send operation is possible. Repeat until all pending send operations are completed.
        if (!_send_event.isValid()) {
            _send_event = _reactor.newWriteNotify(this, _socket.getSocket());
            if (!_send_event.isValid()) {
                return false;
            }
        }
        // Enqueue a pending send, to be attempted later.
        _pending_send.push_back(req);
        return true;
    }
    else {
        // Actual error when trying to send.
        return false;
    }
}


//----------------------------------------------------------------------------
// Cancel any pending send operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::cancelSend(bool silent)
{
    // Stop being notified of send-ready.
    if (_send_event.isValid()) {
        _reactor.deleteWriteNotify(_send_event, silent);
        _send_event.invalidate();
    }

    // Cancel and deallocate all pending requests, completed or not.
    // Completed requests must be deallocated as well because the application must not expect being notified after cancelSend().
    _pending_send.clear();
    _completed_send.clear();
}


//----------------------------------------------------------------------------
// Called when a send operation is possible or completed.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleWriteEvent(Reactor& reactor, EventId id, size_t size, int error_code)
{
    if (id == _send_event) {

        const size_t pending = _pending_send.size();
        size_t sent = 0;

        // It is now possible to write on this socket, try to send all pending messages.
        while (!_pending_send.empty()) {
            // First, remove the front send request.
            auto req(_pending_send.front());
            _pending_send.pop_front();

            // Then, try to send the message.
            Socket::IOSB iosb;
            const bool success = _socket.send(req->data, req->size, req->destination, &iosb);
            if (success || !iosb.pending) {
                // Immediate success or non-blocking error. Move the request to the completed queue.
                if (success) {
                    sent++;
                }
                else {
                    req->error_code = LastSysErrorCode();
                }
                _completed_send.push_back(req);
            }
            else {
                // Would block, put request back where it was in the queue and retry later.
                _pending_send.push_front(req);
                break;
            }
        }
        _reactor.trace(u"UDP handle write: %d pending, %d sent", pending, sent);

        // Now process all completed sent messages.
        processCompletedWrite();

        // When no more pending send operation, stop being notified of send-ready.
        if (_pending_send.empty()) {
            _reactor.deleteWriteNotify(_send_event);
            _send_event.invalidate();
        }
    }
}


//----------------------------------------------------------------------------
// Process completed send completions.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::processCompletedWrite()
{
    while (!_completed_send.empty()) {
        // First, remove the front completed send request.
        auto req(_completed_send.front());
        _completed_send.pop_front();

        // Then, call the application's handler.
        if (req->handler != nullptr) {
            req->handler->handleUDPSend(*this, req->data, req->size, req->destination, req->error_code);
        }
    }
}


//----------------------------------------------------------------------------
// Start the operation of receiving a message from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveUDPSocket::startReceive(ReactiveUDPReceiveHandlerInterface* handler)
{
    // Unlike send operation, the handler cannot be null because there is no other way to get a datagram.
    if (handler == nullptr) {
        _reactor.report().error(u"internal error: null handler in UDPSocket::TimeStampType");
        return false;
    }

    // Register receive-ready event if not already set.
    if (!_receive_event.isValid()) {
        _receive_handler = handler;
        _receive_event = _reactor.newReadNotify(this, _socket.getSocket());
    }

    return _receive_event.isValid();
}


//----------------------------------------------------------------------------
// Cancel any pending receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::cancelReceive(bool silent)
{
    // Stop being notified of receive-ready.
    if (_receive_event.isValid()) {
        _reactor.deleteReadNotify(_receive_event, silent);
        _receive_event.invalidate();
        _receive_handler = nullptr;
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation is possible or completed.
//----------------------------------------------------------------------------

void ts::ReactiveUDPSocket::handleReadEvent(Reactor& reactor, EventId id, size_t size, int error_code)
{
    if (id == _receive_event) {

        assert(_receive_handler != nullptr);
        size_t received = 0;

        // Loop on all received datagrams, until it would block.
        for (;;) {

            // Try to receive one datagram.
            ByteBlockPtr data(std::make_shared<ByteBlock>(IP_MAX_PACKET_SIZE));
            size_t retsize = 0;
            IPSocketAddress sender;
            IPSocketAddress destination;
            cn::microseconds timestamp;
            UDPSocket::TimeStampType timestamp_type = UDPSocket::TimeStampType::NONE;
            Socket::IOSB iosb;
            bool success = _socket.receive(data->data(), data->size(), retsize, sender, destination, nullptr, &timestamp, &timestamp_type, &iosb);

            if (success) {
                // Actual datagram received, call the handler.
                received++;
                data->resize(std::min(retsize, data->size()));
                _receive_handler->handleUDPReceive(*this, data, sender, destination, timestamp, timestamp_type, 0);
            }
            else if (iosb.pending) {
                // End of pending messages, retry later, when the Reactor invokes this method again.
                break;
            }
            else {
                // Receive error, call the handler and cancel the read notification.
                error_code = LastSysErrorCode();
                data->clear();
                _receive_handler->handleUDPReceive(*this, data, IPSocketAddress(), IPSocketAddress(), cn::microseconds::zero(), UDPSocket::TimeStampType::NONE, error_code);
                cancelReceive();
                break;
            }
        }
        _reactor.trace(u"UDP handle read: %d received", received);
    }
}
