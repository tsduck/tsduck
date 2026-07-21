//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  UDP socket for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveDevice.h"
#include "tsReactiveUDPHandlerInterface.h"
#include "tsUDPSocket.h"
#include "tsIPProtocols.h"

namespace ts {
    //!
    //! UDP socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveUDPSocket is a wrapper around UDPSocket to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call send(), receive(), or close() on this socket and delegate
    //! these operations to startSend(), startReceive(), and startClose() in class ReactiveUDPSocket.
    //!
    class TSCOREDLL ReactiveUDPSocket: public ReactiveDevice, private SocketHandlerInterface
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
        //! Get a reference to the associated socket.
        //! @return A reference to the associated socket.
        //!
        UDPSocket& socket() { return _socket; }

        //!
        //! Check if the reactive socket is open.
        //! This is different from Socket::isOpen() during the closing phase, after startClose() has been called but before the underlying socket is fully closed.
        //! @return True if the reactive socket is open, false if the underlying socket is closed or if startClose() has been called.
        //!
        bool isOpen() const { return _socket.isOpen() && _pending_close == nullptr; }

        //!
        //! Start the operation of sending a message to a destination address and port.
        //! @param [in] handler Handler class to call when the send operation completes. The method handleUDPSend()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancelation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] destination Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to IPAddress::AnyAddress4 or IPSocketAddress::AnyPort.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start the operation of sending a message to the default destination address and port.
        //! @param [in] handler Handler class to call when the send operation completes. The method handleUDPSend()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancelation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data = ObjectPtr())
        {
            return startSend(handler, data, size, _socket.getDefaultDestination(), user_data);
        }

        //!
        //! Start the operation of receiving messages from the socket.
        //! @param [in] handler Handler class to call each time a message is received. The method handleUDPReceive() will be called
        //! for each new datagram. Cannot be null. If a previous receive handler was registered, it is replaced.
        //! @param [in] max_message_size Maximum incoming message size. Used as size of internal reception
        //! buffer. The default is the maximum IP packet size.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveUDPHandlerInterface* handler, size_t max_message_size = IP_MAX_PACKET_SIZE, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Cancel any pending send or receive operation on this socket.
        //! If a repeated reception operation is in progress, the repetition is canceled as well.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        void cancelSendReceive(bool silent = false);

        //!
        //! Start closing the socket.
        //! Pending asynchronous operations are canceled. The actual cancelation will take place later.
        //! In the meantime, the user's data buffers for these pending operations are busy and shall not
        //! be destroyed / deallocated by the application. The close operation terminates when the handler
        //! handleUDPClosed() is invoked. At this point, no more operation is pending and the application
        //! may get rid of data buffers.
        //! @param [in] handler Handler class to call when the close operation completes. The method handleUDPClosed()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        bool startClose(ReactiveUDPHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr());

    private:
        using HandlerType = ReactiveUDPHandlerInterface;

        // Description of a send request.
        class TSCOREDLL SendRequest: public Object
        {
            TS_NOCOPY(SendRequest);
        public:
            SendRequest() = default;
            virtual ~SendRequest() override;
        public:
            HandlerType*    handler = nullptr;
            const void*     data = nullptr;
            size_t          size = 0;
            IPSocketAddress destination {};
        };

        // Description of a receive request.
        class TSCOREDLL ReceiveRequest: public Object
        {
            TS_NOCOPY(ReceiveRequest);
        public:
            ReceiveRequest(size_t size = 0) : data(std::make_shared<ByteBlock>(size)) {}
            virtual ~ReceiveRequest() override;
        public:
            ByteBlockPtr             data {};
            IPSocketAddress          sender {};
            IPSocketAddress          destination {};
            cn::microseconds         timestamp {};
            UDPSocket::TimeStampType timestamp_type = UDPSocket::TimeStampType::NONE;
            size_t                   recv_size = 0;
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

        // ReactiveUDPSocket private fields.
        UDPSocket&            _socket;                    // The actual socket.
        IOQueue               _pending_send {};           // Queue of pending send operations, waiting for write-ready.
        size_t                _max_receive_size = 0;      // Zero means no active message reception.
        HandlerType*          _receive_handler = nullptr; // Same handler for all receive requests.
        ObjectPtr             _receive_app_data {};       // User data for all received messages.
        std::shared_ptr<IOSB> _pending_receive {};        // Only one pending receive operation at a time (asynchronous I/O only).
        std::shared_ptr<IOSB> _pending_close {};          // Close request, waiting for asynchronous I/O to complete.

        // Start a receive operation.
        bool tryReceive(IOSB*);

        // Call the receive handler.
        void callReceiveHandler(const std::shared_ptr<ReceiveRequest>&, int error_code);

        // Inherited methods.
        virtual void processQueuedOperations() override;
        virtual void handleReadReady(Reactor&, EventId, int) override;
        virtual void handleWriteReady(Reactor&, EventId, int) override;
        virtual void handleAsynchronousIO(Reactor&, EventId, IOSB&, size_t) override;
    };
}
