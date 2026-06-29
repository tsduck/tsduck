//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic TCP server in a Reactor environment.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveTCPServer.h"
#include "tsReactiveServerFactoryInterface.h"
#include "tsReactiveServerHandlerInterface.h"

namespace ts {
    //!
    //! Generic TCP server in a Reactor environment.
    //!
    //! This class works on an application-provided ReactiveTCPServer, meaning that any specialized subclass
    //! such as ReactiveTLSServer can be used. The (subclass of) ReactiveTCPServer object and its associated
    //! (subclass of) TCPServer  must be initialized in the application, up to and including listen(). The
    //! calls to accept() are performed by the ReactiveServer.
    //!
    //! This server automatically creates sessions when incoming clients connect and automatically deletes
    //! the session object when they disconnect. The "session" objects are created by an instance of
    //! ReactiveServerFactoryInterface. The session object class must implement ReactiveServerSessionInterface.
    //!
    //! Note that one session object is always allocated in advance to catch the next incoming client.
    //!
    //! A session object is deleted after the underlying socket is closed. More precisely, the deletion occurs
    //! in an independent reactor handler, shortly after the socket is closed. This means that if the closing
    //! occurs deep in a cascade of calls involving methods or handlers in the session object, that session object
    //! remains valid all along the return chain of those calls. When the deletion occurs, no method of the session
    //! object is present in the stack frames. However, the application shall take care that no further handler will
    //! be called later on that session object because it will no longer exist.
    //!
    class TSCOREDLL ReactiveServer: private ReactorHandlerInterface, private ReactiveTCPServerHandlerInterface, private SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveServer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] server Actual reactive server. This can also be a subclass such as ReactiveTLSServer.
        //! The associated Reactor will be used for all event dispatching. A reference to @a server is kept inside
        //! "this" object. The @a server object must remain valid as long as "this" object exists.
        //!
        explicit ReactiveServer(ReactiveTCPServer& server);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveServer() override;

        //!
        //! Start the reactive server in the reactor.
        //! @param [in] session_factory A factory object which is called for each new incoming client session.
        //! @param [in] handler Optional handler to receive notification of internal events.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        bool start(ReactiveServerFactoryInterface* session_factory, ReactiveServerHandlerInterface* handler = nullptr, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Specify if the event loop of the reactor shall be exited when the server exits.
        //! This could be implemented in a handleServerExited() handler in the application but,
        //! since this is a common usage, provide a more straightforward way.
        //! @param [in] on If true, the event loop of the reactor is exited when the server exits.
        //!
        void setExitEventLoop(bool on) { _exit_loop = on; }

        //!
        //! Mark the server to exit after having accepted a given number of clients.
        //! @param [in] count Number of clients after which the server shall exit.
        //!
        void setExitAfterClientCount(size_t count);

        //!
        //! Mark the server to exit when the last active client terminates.
        //! No more new client will be accepted.
        //! If no client is currenly connected, the server exits immediately.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void exit(bool silent = false);

        //!
        //! Abort all connected clients and exits the server.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void abort(bool silent = false);

    private:
        enum ServerState {STOPPED, ACCEPTING, EXITING};

        ReactiveTCPServer&                        _server;                    // Actual server.
        Reactor&                                  _reactor;                   // Associated reactor.
        ReactiveServerFactoryInterface*           _session_factory = nullptr; // Factory to create new sessions.
        ReactiveServerHandlerInterface*           _handler = nullptr;         // Application handler for internal server events.
        ObjectPtr                                 _user_data {};              // Handler user data.
        ReactiveServerSessionInterface*           _accepting = nullptr;       // Unconnected client in current accept().
        std::set<ReactiveServerSessionInterface*> _clients {};                // Connected active clients.
        std::set<ReactiveServerSessionInterface*> _pending_delete {};         // Closed clients to delete.
        std::map<Socket*, ReactiveServerSessionInterface*> _sockets {};       // Associate low-level sockets with session objects (for close handler).
        EventId                                   _delete_id {};              // Event id to trigger to deletion of closed clients.
        size_t                                    _client_count = 0;          // Number of accepted clients.
        size_t                                    _max_client_count = std::numeric_limits<size_t>::max(); // Exit after that number of clients.
        ServerState                               _state = STOPPED;           // Current server state.
        bool                                      _exit_loop = false;         // Exit event loop with server.
        bool                                      _close_completed = false;   // Server close is completed.

        // Create a session object and start accepting on it.
        bool createNewSession();

        // Schedule the deletion of a session for some later reactor handler.
        bool scheduleDeletion(ReactiveServerSessionInterface* session, bool silent);

        // Exit server if all conditions match.
        void exitWhenReady();

        // Event handler: called when a session must be deleted.
        virtual void handleUserEvent(Reactor&, EventId) override;

        // Socket handler: called when a session socket is closed.
        virtual void handleSocketCloseComplete(Socket& sock, bool silent, bool success) override;

        // ReactiveTCPServer handlers: called when a client is accepted or the server completes its close.
        virtual void handleTCPClientAccepted(ReactiveTCPServer&, ReactiveTCPConnection& sock, const IPSocketAddress& addr, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPServerClosed(ReactiveTCPServer&, const ObjectPtr& user_data) override;
    };
}
