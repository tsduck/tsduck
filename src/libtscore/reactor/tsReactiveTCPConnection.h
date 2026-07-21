//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TCP connected socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveStream.h"
#include "tsSubscriptionBase.h"
#include "tsReactiveTCPConnectionHandlerInterface.h"
#include "tsTCPConnection.h"

namespace ts {
    //!
    //! TCP connected socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveTCPConnection is a wrapper around TCPConnection to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call connect(), send(), receive(), closeWriter(), or close() on this socket and
    //! delegate these operations to startConnect(), startSend(), startReceive(), startCloseWriter() and startClose() in
    //! class ReactiveTCPConnection.
    //!
    //! The class implements the same subscription mechanism as the class Socket. The events are the same as the internal
    //! Socket element, except the handleSocketCloseComplete() event which occurs at the end of the asynchronous completion
    //! of the reactive socket.
    //!
    class TSCOREDLL ReactiveTCPConnection: public ReactiveStream, public SubscriptionBase, private SocketHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveTCPConnection);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveTCPConnection must be initialized before the @a socket is opened.
        //!
        ReactiveTCPConnection(Reactor& reactor, TCPConnection& socket);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveTCPConnection() override;

        //!
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        TCPConnection& socket() { return _socket; }

        //!
        //! Check if the reactive socket is open.
        //! This is different from Socket::isOpen() during the closing phase, after startClose() has been called but before the underlying socket is fully closed.
        //! @return True if the reactive socket is open, false if the underlying socket is closed or if startClose() has been called.
        //!
        bool isOpen() const { return _socket.isOpen() && _pending_close == nullptr; }

        //!
        //! Start the operation of connecting to a TCP server.
        //! @param [in] handler Handler class to call when the connect operation completes. The method handleTCPConnected()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] addr IP address and port of the server to connect to.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the connection was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        virtual bool startConnect(ReactiveTCPConnectionHandlerInterface* handler, const IPSocketAddress& addr, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Define the handler to call when accepted as a client session by a TCP server.
        //! Typically called from the constructor of an enclosing object which is used as client session context.
        //! @param [in] handler Handler class to call when this object is connected to a remote client by a server.
        //! The method handleTCPAccepted() will be called. If nullptr, no handler is called.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //!
        virtual void whenAccepted(ReactiveTCPConnectionHandlerInterface* handler, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start closing the send direction of the socket.
        //!
        //! The peer will receive an end-of-file condition. All pending send operations are guaranteed to
        //! complete before that end-of-file is sent.
        //!
        //! @param [in] handler Handler class to call when the close-writer operation completes. The method handleWriteStream()
        //! will be called with its parameter @a error_code containing SYS_EOF. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        virtual bool startCloseWriter(ReactiveStreamHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start closing the socket.
        //!
        //! Pending asynchronous operations are canceled. The actual cancelation will take place later.
        //! In the meantime, the user's data buffers for these pending operations are busy and shall not
        //! be destroyed / deallocated by the application. The close operation terminates when the handler
        //! handleTCPClosed() is invoked. At this point, no more operation is pending and the application
        //! may get rid of data buffers.
        //!
        //! Note that the application, usually inside a handler, can call disconnect() on the TCPConnection object.
        //! Only closeWriter() and close() shall not be called and replaced by startCloseWriter() and startClose()
        //! on the ReactiveTCPConnection object.
        //!
        //! @param [in] handler Handler class to call when the close operation completes. The method handleTCPClosed()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        virtual bool startClose(ReactiveTCPConnectionHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr());

    protected:
        //! Shorter name for handler type.
        using HandlerType = ReactiveTCPConnectionHandlerInterface;

        // Inherited methods (implementation of protected interface).
        virtual int processCloseWriteStream(bool silent) override;
        virtual bool tryCompletedIO(const std::shared_ptr<IOSB>& iosb) override;
        virtual void processQueuedOperations() override;
        virtual void handleWriteReady(Reactor&, EventId, int) override;
        virtual void handleAsynchronousIO(Reactor&, EventId, IOSB&, size_t) override;

    private:
        // Description of a connect request.
        class TSCOREDLL ConnectRequest: public Object
        {
            TS_NOCOPY(ConnectRequest);
        public:
            ConnectRequest() = default;
            virtual ~ConnectRequest() override;
        public:
            HandlerType* handler = nullptr;
            IPSocketAddress server {};
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
            bool silent = false;
        };

        // ReactiveTCPConnection private fields.
        TCPConnection&        _socket;
        std::shared_ptr<IOSB> _pending_connect {};   // Only one pending connect operation at a time (non-blocking and asynchronous I/O).
        std::shared_ptr<IOSB> _pending_close {};     // Close request, waiting for asynchronous I/O to complete.
        ReactiveTCPConnectionHandlerInterface* _accept_handler = nullptr; // Called when accepted as a client session in a server.
        ObjectPtr             _accept_user_data {};  // Used in _accept_handler.

        // Declare that the socket has just become connected as a TCP client session in a server. Must be called in reactor context.
        friend class ReactiveTCPServer;
        void declareConnected(ReactiveTCPServer& server, int error_code);

        // Capture all events from the underlying Socket, except handleSocketCloseComplete().
        virtual void handleSocketOpenStart(Socket& sock) override;
        virtual void handleSocketOpenComplete(Socket& sock, bool success) override;
        virtual void handleSocketConnected(TCPConnection& sock) override;
        virtual void handleSocketDisconnected(TCPConnection& sock, bool silent) override;
        virtual void handleSocketCloseStart(Socket& sock, bool silent) override;
    };
}
