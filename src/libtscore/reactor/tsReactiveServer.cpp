//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveServer.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveServer::ReactiveServer(ReactiveTCPServer& server) :
    _server(server),
    _reactor(server.reactor())
{
}

ts::ReactiveServer::~ReactiveServer()
{
}


//----------------------------------------------------------------------------
// Start the reactive server in the reactor.
//----------------------------------------------------------------------------

bool ts::ReactiveServer::start(ReactiveServerFactoryInterface* session_factory, ReactiveServerHandlerInterface* handler, const ObjectPtr& user_data)
{
    // Check context and parameters.
    if (_state != STOPPED) {
        _reactor.report().error(u"reactive server already started");
        return false;
    }
    if (session_factory == nullptr) {
        _reactor.report().error(u"internal error: no session factory specified in reactive server");
        return false;
    }

    // Create the user event for session deletion, if not yet created.
    if (!_delete_id.isValid()) {
        _delete_id = _reactor.newEvent(this);
        if (!_delete_id.isValid()) {
            return false;
        }
    }

    // Remember factory and handler for the rest of the session.
    _session_factory = session_factory;
    _handler = handler;
    _user_data = user_data;
    _close_completed = false;
    _client_count = 0;

    // Start accepting the first client.
    return createNewSession();
}


//----------------------------------------------------------------------------
// Mark the server to exit after having accepted a given number of clients.
//----------------------------------------------------------------------------

void ts::ReactiveServer::setExitAfterClientCount(size_t count)
{
    // Remember the max count.
    _max_client_count = count;

    // If we already reached that number, exit now.
    if (_client_count >= _max_client_count) {
        exit();
    }
}


//----------------------------------------------------------------------------
// Mark the server to exit when the last active client terminates.
//----------------------------------------------------------------------------

void ts::ReactiveServer::exit(bool silent)
{
    if (_state == ACCEPTING) {
        _state = EXITING;
        _server.startClose(this, silent);
    }
}


//----------------------------------------------------------------------------
// Abort all connected clients and exits the server.
//----------------------------------------------------------------------------

void ts::ReactiveServer::abort(bool silent)
{
    // Abort all established connections.
    // Use a copy of the set because the original will be modified by the callbacks.
    const auto copy(_clients);
    for (auto session : copy) {
        session->getConnection().socket().close(silent);
    }
    exit(silent);
}


//----------------------------------------------------------------------------
// Create a session object and start accepting on it.
//----------------------------------------------------------------------------

bool ts::ReactiveServer::createNewSession()
{
    // Allocate the new session object.
    assert(_session_factory != nullptr);
    assert(_accepting == nullptr);
    auto new_session = _session_factory->newClientSession();

    // Check the validity of the session object.
    if (new_session == nullptr) {
        _reactor.report().error(u"internal error: session factory returned null pointer");
        return false;
    }
    if (&new_session->getConnection().reactor() != &_reactor) {
        _reactor.report().error(u"internal error: session factory uses an different reactor");
        scheduleDeletion(new_session, true);
        return false;
    }

    // Record the association of socket/session.
    Socket* sock = &new_session->getConnection().socket();
    _sockets[sock] = new_session;

    // Subscribe to events on the reactive socket. We subscribe to events on the reactive socket, not on the internal
    // socket, to make sure that handleSocketCloseComplete() is called at the end of the asynchronous close.
    new_session->getConnection().addSubscription(this);

    // Start accepting a new connection.
    _accepting = new_session;
    if (_server.startAccept(this, _accepting->getConnection())) {
        _state = ACCEPTING;
        return true;
    }
    else {
        _accepting = nullptr;
        scheduleDeletion(new_session, true);
        exit(true);
        return false;
    }
}


//----------------------------------------------------------------------------
// Called when a client is accepted.
//----------------------------------------------------------------------------

void ts::ReactiveServer::handleTCPClientAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& sock, const IPSocketAddress& addr, int error_code, const ObjectPtr& user_data)
{
    assert(&server == &_server);
    assert(_accepting != nullptr);
    assert(&_accepting->getConnection() == &sock);

    // Count accepted clients (success or fail).
    _client_count++;

    if (error_code == SYS_SUCCESS) {
        // Register client as active.
        _clients.insert(_accepting);
        _accepting = nullptr;

        // Start accepting a new client. Exit server on error.
        if (_client_count < _max_client_count && !createNewSession()) {
            exit(true);
        }
    }
    else {
        // Close the failed accepting socket. Let the close handler continue the processing.
        _accepting->getConnection().socket().close(true);
    }

    // Exit server when the max number of clients is reached.
    // Currently connected clients continue to run.
    if (_client_count >= _max_client_count) {
        exit();
    }
}


//----------------------------------------------------------------------------
// Socket handler: called when a session socket is closed.
//----------------------------------------------------------------------------

void ts::ReactiveServer::handleSocketCloseComplete(Socket& sock, bool silent, bool success)
{
    // Check that the socket is one of ours.
    const auto it = _sockets.find(&sock);
    if (it == _sockets.end()) {
        _reactor.report().log(ReporterBase::SilentLevel(silent), u"spurious socket close notification @%X", uintptr_t(&sock));
    }
    else {
        scheduleDeletion(it->second, silent);
    }
}


//----------------------------------------------------------------------------
// Schedule the deletion of a session for some later reactor handler.
//----------------------------------------------------------------------------

bool ts::ReactiveServer::scheduleDeletion(ReactiveServerSessionInterface* session, bool silent)
{
    // Remove the session from the active ones.
    if (session == _accepting) {
        _accepting = nullptr;
    }
    else if (_clients.erase(session) == 0) {
        _reactor.report().log(ReporterBase::SilentLevel(silent), u"spurious session deletion @%X", uintptr_t(session));
    }

    // Actual deletions will occur later, outside all calling handlers.
    _pending_delete.insert(session);
    return _reactor.signalEvent(_delete_id);
}


//----------------------------------------------------------------------------
// Event handler: called when a session must be deleted.
//----------------------------------------------------------------------------

void ts::ReactiveServer::handleUserEvent(Reactor& reactor, EventId id)
{
    // Filter out other events (there should be none).
    if (id == _delete_id) {
        // Save and clear the structures containing sessions to protect against callbacks in destructors.
        decltype(_pending_delete) sessions;
        sessions.swap(_pending_delete);

        // Delete sessions one by one.
        for (auto sess : sessions) {
            Socket* sock = &sess->getConnection().socket();
            _reactor.report().debug(u"deleting session @%X, socket @%X", uintptr_t(sess), uintptr_t(sock));
            sock->cancelSubscription(this);
            if (_sockets.erase(sock) == 0) {
                _reactor.report().error(u"internal error: deleting session @%X, socket @%X was not registered", uintptr_t(sess), uintptr_t(sock));
            }
            delete sess;
        }

        // If there are no more clients, this may be a server exit.
        exitWhenReady();
    }
}


//----------------------------------------------------------------------------
// Called when the server completes its close.
//----------------------------------------------------------------------------

void ts::ReactiveServer::handleTCPServerClosed(ReactiveTCPServer&, const ObjectPtr& user_data)
{
    _close_completed = true;
    exitWhenReady();
}


//----------------------------------------------------------------------------
// Exit server if all conditions match.
//----------------------------------------------------------------------------

void ts::ReactiveServer::exitWhenReady()
{
    _reactor.report().debug(u"reactive server: test exit, state: %d, closed: %s, accepting: %s, connected: %d, pending delete: %d",
                            _state, _close_completed, _accepting != nullptr, _clients.size(), _pending_delete.size());

    if (_state == EXITING && _close_completed && _accepting == nullptr && _clients.empty() && _pending_delete.empty()) {
        _state = STOPPED;
        if (_handler != nullptr) {
            _handler->handleServerExited(*this, _user_data);
        }
        if (_exit_loop) {
            _reactor.exitEventLoop(true);
        }
    }
}
