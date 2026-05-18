//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTCPServer.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTCPServer::ReactiveTCPServer(Reactor& reactor, TCPServer& socket, Object* owner) :
    ReactiveBase(reactor, socket, owner),
    _socket(socket)
{
}

ts::ReactiveTCPServer::~ReactiveTCPServer()
{
}


//----------------------------------------------------------------------------
// Virtual destructors for request classes.
//----------------------------------------------------------------------------

ts::ReactiveTCPServer::AcceptRequest::~AcceptRequest() {}
ts::ReactiveTCPServer::CloseRequest::~CloseRequest() {}


//----------------------------------------------------------------------------
// Process completed I/O operations.
//----------------------------------------------------------------------------

void ts::ReactiveTCPServer::processCompletedIO()
{
    reactor().trace(u"ReactiveTCPServer: processing %d completed I/O", _completed_io.size());

    // Process completed I/O.
    while (!_completed_io.empty()) {

        // Remove the front completed IOSB. Must be an accept request.
        auto iosb = _completed_io.front();
        _completed_io.pop_front();
        auto req = std::dynamic_pointer_cast<AcceptRequest>(iosb->react_data);
        assert(req != nullptr);

        // Notify the application.
        if (req->handler != nullptr) {
            // Notify that the server accepted a connection (or reported an error).
            req->handler->handleTCPClientAccepted(*this, *req->client, req->client_addr, iosb->error_code, iosb->app_data);
            // If there was no error but the connection is not open, the server's handler break the connection (e.g. client is rejected).
            if (iosb->error_code == SYS_SUCCESS && !req->client->isOpen()) {
                iosb->error_code = SYS_REJECTED;
            }
            req->client->declareConnected(*this, iosb->error_code);
        }
    }

    // If a close request is pending and no more I/O is pending, close.
    if (_pending_accept.empty() && _pending_close != nullptr) {
        reactor().trace(u"ReactiveTCPServer: completed close %X, socket %d", uintptr_t(this), _socket.getSocket());

        // Get a copy of the handler, req is going to be destroyed.
        auto req = std::dynamic_pointer_cast<CloseRequest>(_pending_close->react_data);
        assert(req != nullptr);
        auto handler = req->handler;
        bool silent = req->silent;
        auto app_data = _pending_close->app_data;

        // Close the underlying socket.
        _socket.close(silent);

        // Clear all states. Don't notify of anything else.
        assert(_pending_accept.empty());
        _completed_io.clear();
        _pending_close.reset();
        deactivateAll(silent);

        // Notify application.
        if (handler != nullptr) {
            handler->handleTCPServerClosed(*this, app_data);
        }
    }
}


//----------------------------------------------------------------------------
// Start the operation of accepting a TCP client.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPServer::startAccept(ReactiveTCPServerHandlerInterface* handler, ReactiveTCPConnection& client, const ObjectPtr& user_data)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    // Non-blocking I/O model: Get notified when a read operation is possible, if not already done.
    if (!activateAsynchronousIO() || !activateReadReady()) {
        return false;
    }

    // Allocate an accept request structure.
    auto req = std::make_shared<AcceptRequest>();
    req->handler = handler;
    req->client = &client;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    // Try to start the accept operation now.
    if (!_socket.accept(req->client->socket(), req->client_addr, iosb.get())) {
        // Error when trying to accept.
        return false;
    }
    else if (iosb->pending) {
        // Connection in progress. Register the request.
        _pending_accept.push_back(iosb);
        return true;
    }
    else {
        // Immediate success. Enqueue a completed request. Will be processed in a reactor handler.
        _completed_io.push_back(iosb);
        // Notify the completion event to call the user's notification in the context of the reactor.
        return signalCompletedIO();
    }
}


//----------------------------------------------------------------------------
// Called when a receive operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPServer::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    // Try to accept all pending requests.
    while (_socket.isOpen() && !_pending_accept.empty()) {

        // Remove the front request.
        auto iosb = _pending_accept.front();
        _pending_accept.pop_front();

        // The send request is in the application data of the IOSB.
        auto req = std::dynamic_pointer_cast<AcceptRequest>(iosb->react_data);
        assert(req != nullptr);

        // Try to accept a connection.
        const bool success = _socket.accept(req->client->socket(), req->client_addr, iosb.get());
        if (iosb->pending) {
            // Would block again. Put request back where it was in the queue and retry later.
            _pending_accept.push_front(iosb);
            break;
        }
        else {
            // Immediate success or error. Move the request to the completed queue.
            iosb->error_code = success ? SYS_SUCCESS : LastSysErrorCode();
            _completed_io.push_back(iosb);
        }
    }

    // Now process all completed accepts.
    processCompletedIO();

    // When no more pending operation, stop being notified of read-ready.
    if (_pending_accept.empty()) {
        deactivateReadReady(false);
    }
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveTCPServer::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size)
{
    // At this point, we only have an IOSB, not a std::shared_ptr.
    // Our custom request is in the react_data of the IOSB.
    // Here, there is only one possible type request: accept completion.
    auto acpt = std::dynamic_pointer_cast<AcceptRequest>(iosb.react_data);
    if (acpt == nullptr) {
        report().error(u"unexpected completed asynchronous TCP server request, not accept");
        return;
    }

    // Search for the request in the queue of pending accept requests.
    std::shared_ptr<IOSB> req = removeFromQueue(_pending_accept, &iosb);
    if (req == nullptr) {
        report().error(u"unreferenced completed asynchronous TCP send request");
        return;
    }

    // Get the parameters of the accept.
    if (iosb.error_code == SYS_SUCCESS && !_socket.setAcceptStatus(acpt->client->socket(), acpt->client_addr, &iosb)) {
        iosb.error_code = SYS_ERROR;
    }

    // Process the request.
    _completed_io.push_back(req);
    processCompletedIO();
}


//----------------------------------------------------------------------------
// Cancel any pending accept operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveTCPServer::cancelAccept(bool silent)
{
    // Stop being notified of read-ready (non-blocking I/O).
    deactivateReadReady(silent);

    // Cancel asynchronous I/O currently in progress (asynchronous I/O).
    cancelAsynchronousIO(silent);

    if constexpr (Reactor::UseNonBlockingIO()) {
        // Mark all pending accept requests as canceled.
        cancelQueue<AcceptRequest>(_pending_accept, _completed_io);
        // Handle all completions in reactor context.
        signalCompletedIO();
    }
}


//----------------------------------------------------------------------------
// Start closing the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTCPServer::startClose(ReactiveTCPServerHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    reactor().trace(u"ReactiveTCPServer: start close %X, socket %d", uintptr_t(this), _socket.getSocket());

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
    cancelAccept(silent);

    // If there is no more pending accept request, process the close request in reactor context.
    if (_pending_accept.empty()) {
        signalCompletedIO();
    }
    return true;
}
