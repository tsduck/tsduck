//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  UDP Socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveUDPHandlerInterface.h"
#include "tsUDPSocket.h"
#include "tsReactor.h"

namespace ts {
    //!
    //! UDP Socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveUDPSocket is a wrapper around UDPSocket to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application must not directly call send() or receive on this socket and delegate the
    //! send/receive operations to the class ReactiveUDPSocket.
    //!
    class TSCOREDLL ReactiveUDPSocket:
        private SocketHandlerInterface,
        private ReactorEventHandlerInterface,
        private ReactorReadHandlerInterface,
        private ReactorWriteHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveUDPSocket);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] socket Associated socket. The socket object must remain valid as long as this object is valid.
        //! The ReactiveUDPSocket must be initialized before the @a socket is opened.
        //!
        ReactiveUDPSocket(Reactor& reactor, UDPSocket& socket);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveUDPSocket() override;

        //!
        //! Get a reference to the associated reactor.
        //! @return A reference to the associated reactor.
        //!
        Reactor& reactor() { return _reactor; }

        //!
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        UDPSocket& socket() { return _socket; }

        //!
        //! Start the operation of sending a message to a destination address and port.
        //!
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancellation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] destination Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to IPAddress::AnyAddress4 or IPSocketAddress::AnyPort.
        //! @param [in] handler Handler class to call when the send operation completes.
        //! If nullptr, no handler is caller.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPSendHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination);

        //!
        //! Start the operation of sending a message to the default destination address and port.
        //!
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancellation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] handler Handler class to call when the send operation completes.
        //! If nullptr, no handler is caller.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPSendHandlerInterface* handler, const void* data, size_t size)
        {
            return startSend(handler, data, size, _socket.getDefaultDestination());
        }

        //!
        //! Cancel any pending send operation on this socket.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void cancelSend(bool silent = false);

        //!
        //! Start the operation of receiving messages from the socket.
        //!
        //! @param [in] handler Handler class to call each time a message is received. Cannot be null.
        //! If a previous handler was registered, it is replaced.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveUDPReceiveHandlerInterface* handler);

        //!
        //! Cancel any pending receive operation on this socket.
        //! If a repeated reception operation is in progress, the repetition is cancelled as well.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void cancelReceive(bool silent = false);

    private:
        // Description of a pending send request.
        class TSCOREDLL SendRequest
        {
            TS_NOCOPY(SendRequest);
        public:
            SendRequest() {}
            ReactiveUDPSendHandlerInterface* handler = nullptr;
            const void*                      data = nullptr;
            size_t                           size = 0;
            IPSocketAddress                  destination {};
            int                              error_code = 0;
        };
        using SendQueue = std::list<std::shared_ptr<SendRequest>>;

        // ReactiveUDPSocket private fields.
        Reactor&   _reactor;
        UDPSocket& _socket;
        EventId    _completion_event {};
        EventId    _send_event {};
        EventId    _receive_event {};
        SendQueue  _pending_send {};
        SendQueue  _completed_send {};
        ReactiveUDPReceiveHandlerInterface* _receive_handler = nullptr;

        // Process completed send operations.
        void processCompletedWrite();

        // Implementation of Socket and Reactor handlers.
        virtual void handleSocketOpen(Socket& sock) override;
        virtual void handleSocketClose(Socket& sock) override;
        virtual void handleUserEvent(Reactor& reactor, EventId id) override;
        virtual void handleReadEvent(Reactor& reactor, EventId id, size_t size, int error_code) override;
        virtual void handleWriteEvent(Reactor& reactor, EventId id, size_t size, int error_code) override;
    };
}
