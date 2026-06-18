//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP server socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveSocketBase.h"
#include "tsReactiveTCPServerHandlerInterface.h"
#include "tsReactiveTCPConnection.h"
#include "tsTCPServer.h"

namespace ts {
    //!
    //! TCP server socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTCPServer is a wrapper around TCPServer to handle reactive I/O.
    //!
    //! The actual server socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call accept(), or close() on this socket and delegate
    //! these operations to startAccept() and startClose() in class ReactiveTCPServer.
    //!
    class TSCOREDLL ReactiveTCPServer: public ReactiveSocketBase
    {
        TS_NOBUILD_NOCOPY(ReactiveTCPServer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated server socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTCPServer must be initialized before the @a socket is opened.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveTCPServer(Reactor& reactor, TCPServer& socket, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveTCPServer() override;

        //!
        //! Get a reference to the associated server socket.
        //! @return A reference to the associated socket.
        //!
        TCPServer& socket() { return _socket; }

        //!
        //! Check if the reactive socket is open.
        //! This is different from Socket::isOpen() during the closing phase, after startClose() has been called but before the underlying socket is fully closed.
        //! @return True if the reactive socket is open, false if the underlying socket is closed or if startClose() has been called.
        //!
        bool isOpen() const { return _socket.isOpen() && _pending_close == nullptr; }

        //!
        //! Start the operation of accepting a TCP client.
        //! @param [in] handler Handler class to call when the accept operation completes. The method handleTCPClientAccepted()
        //! will be called when the accept() operation completes. If nullptr, no handler is called.
        //! @param [out] client This object receives the new connection. The ReactiveTCPConnection must remain
        //! valid as long as the accept operation is in progress and the handler is not called.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the connection was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        virtual bool startAccept(ReactiveTCPServerHandlerInterface* handler, ReactiveTCPConnection& client, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Cancel any pending accept operation on this socket.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        virtual void cancelAccept(bool silent = false);

        //!
        //! Start closing the socket.
        //! Pending asynchronous operations are canceled. The actual cancelation will take place later.
        //! In the meantime, the user's data buffers for these pending operations are busy and shall not
        //! be destroyed / deallocated by the application. The close operation terminates when the handler
        //! handleTCPClosed() is invoked. At this point, no more operation is pending and the application
        //! may get rid of data buffers.
        //! @param [in] handler Handler class to call when the close operation completes. The method handleTCPServerClosed~()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        virtual bool startClose(ReactiveTCPServerHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr());

    protected:
        //! Internal shorter name for handler interface.
        using HandlerType = ReactiveTCPServerHandlerInterface;

    private:
        // Description of an accept request.
        class TSCOREDLL AcceptRequest: public Object
        {
            TS_NOCOPY(AcceptRequest);
        public:
            AcceptRequest() = default;
            virtual ~AcceptRequest() override;
        public:
            HandlerType*           handler = nullptr;
            ReactiveTCPConnection* client = nullptr;
            IPSocketAddress        client_addr {};
        };

        // Description of a close request.
        class TSCOREDLL CloseRequest: public Object
        {
            TS_NOCOPY(CloseRequest);
        public:
            CloseRequest() = default;
            virtual ~CloseRequest() override;
        public:
            HandlerType* handler = nullptr;
            bool         silent = false;
        };

        // ReactiveTCPConnection private fields.
        TCPServer&            _socket;
        IOQueue               _pending_accept {};  // Queue of pending accept operations, waiting for read-ready or accept completion.
        IOQueue               _completed_io {};    // Queue of completed accept operations, to be notified to application.
        std::shared_ptr<IOSB> _pending_close {};   // Close request, waiting for asynchronous I/O to complete.

        // Inherited methods.
        virtual void processQueuedOperations() override;
        virtual void handleReadReady(Reactor&, EventId, int) override;
        virtual void handleAsynchronousIO(Reactor&, EventId, IOSB&, size_t) override;
    };
}
