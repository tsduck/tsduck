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
#include "tsIPProtocols.h"
#include "tsReactor.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! UDP Socket for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveUDPSocket is a wrapper around UDPSocket to handle reactive I/O.
    //!
    //! The actual socket is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call send(), receive(), or close() on this socket and delegate
    //! these operations to startSend(), startReceive(), and startClose in class ReactiveUDPSocket.
    //!
    class TSCOREDLL ReactiveUDPSocket : private ReactorHandlerInterface
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
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancelation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] destination Socket address of the destination.
        //! Both address and port are mandatory in the socket address, they cannot
        //! be set to IPAddress::AnyAddress4 or IPSocketAddress::AnyPort.
        //! @param [in] handler Handler class to call when the send operation completes.
        //! If nullptr, no handler is caller.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size, const IPSocketAddress& destination);

        //!
        //! Start the operation of sending a message to the default destination address and port.
        //! @param [in] data Address of the message to send. The corresponding memory area must remain
        //! valid until the completion or cancelation of the send operation.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in] handler Handler class to call when the send operation completes.
        //! If nullptr, no handler is caller.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startSend(ReactiveUDPHandlerInterface* handler, const void* data, size_t size)
        {
            return startSend(handler, data, size, _socket.getDefaultDestination());
        }

        //!
        //! Start the operation of receiving messages from the socket.
        //! @param [in] handler Handler class to call each time a message is received. Cannot be null.
        //! If a previous handler was registered, it is replaced.
        //! @param [in] max_message_size Maximum incoming message size. Used as size of internal reception
        //! buffer. The default is the maximum IP packet size.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveUDPHandlerInterface* handler, size_t max_message_size = IP_MAX_PACKET_SIZE);

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
        //! @param [in] handler Handler class to call when the close operation completes.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        bool startClose(ReactiveUDPHandlerInterface* handler, bool silent = false);

    private:
        // Send and receive requests are structures which are stored in the app_data of the IOSB.
        // Queues of I/O requests are queues of shared_ptr to IOSB.
        using IOSB = NonBlockingDevice::IOSB;
        using IOQueue = std::list<std::shared_ptr<IOSB>>;

        // Description of a send request.
        class TSCOREDLL SendRequest: public NonBlockingDevice::AncillaryData
        {
            TS_NOCOPY(SendRequest);
        public:
            SendRequest() = default;
            virtual ~SendRequest() override;

            ReactiveUDPHandlerInterface* handler = nullptr;
            const void*                  data = nullptr;
            size_t                       size = 0;
            IPSocketAddress              destination {};
            int                          error_code = SYS_SUCCESS;
        };

        // Description of a receive request.
        class TSCOREDLL ReceiveRequest: public NonBlockingDevice::AncillaryData
        {
            TS_NOCOPY(ReceiveRequest);
        public:
            ReceiveRequest() = default;
            virtual ~ReceiveRequest() override;

            ByteBlockPtr             data {};
            IPSocketAddress          sender {};
            IPSocketAddress          destination {};
            cn::microseconds         timestamp {};
            UDPSocket::TimeStampType timestamp_type = UDPSocket::TimeStampType::NONE;
            int                      error_code = SYS_SUCCESS;
        };

        // Description of a close request.
        class TSCOREDLL CloseRequest: public NonBlockingDevice::AncillaryData
        {
            TS_NOCOPY(CloseRequest);
        public:
            CloseRequest() = default;
            virtual ~CloseRequest() override;

            ReactiveUDPHandlerInterface* handler = nullptr;
            bool                         silent = false;
        };

        // ReactiveUDPSocket private fields.
        Reactor&   _reactor;
        UDPSocket& _socket;                // The actual socket.
        EventId    _completion_event {};   // Internal user-event to notify processing of completed I/O.
        EventId    _send_ready_id {};      // Reactor id for write-ready (non-blocking I/O).
        EventId    _receive_ready_id {};   // Reactor id for read-ready (non-blocking I/O).
        EventId    _async_io_id {};        // Reactor id for I/O completion (asynchronous I/O).
        IOQueue    _pending_send {};       // Queue of pending send operations, waiting for write-ready or send completion.
        IOQueue    _completed_io {};       // Queue of completed I/O operations, to be notified to application.
        size_t     _max_receive_size = 0;  // Zero means no active message reception.
        std::shared_ptr<IOSB>        _pending_receive {};        // Only one pending receive operation at a time (asynchronous I/O).
        std::shared_ptr<IOSB>        _pending_close {};          // Close request, waiting for asynchronous I/O to complete.
        ReactiveUDPHandlerInterface* _receive_handler = nullptr; // Same handler for all receive requests.

        // Allocate the and start the pending receive request.
        // If it completes immediately, move it to the completed queue and return true.
        bool startPendingReceive();

        // Process a completed close.
        void processCompletedClose(CloseRequest*);

        // Process completed I/O operations. Must be called in the context of a Reactor handler.
        void processCompletedIO();

        // Signal the completed event, so that processCompletedIO() is called in the Reactor context.
        bool signalCompletedIO();

        // Cancel any read/write-ready (non-blocking I/O).
        void deleteReadReady(bool silent);
        void deleteWriteReady(bool silent);

        // Delete and invalidate all registrations.
        void deleteAllIds(bool silent);

        // Implementation of Reactor handlers.
        virtual void handleUserEvent(Reactor&, EventId) override;
        virtual void handleReadReady(Reactor&, EventId, int) override;
        virtual void handleWriteReady(Reactor&, EventId, int) override;
        virtual void handleAsynchronousIO(Reactor&, EventId, IOSB&, size_t, int) override;
    };
}
