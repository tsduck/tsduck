//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Using non-blocking devices with StreamInterface in a Reactor environment.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveDevice.h"
#include "tsReactiveStreamHandlerInterface.h"
#include "tsReactiveInputControl.h"
#include "tsStreamInterface.h"

namespace ts {
    //!
    //! Using non-blocking devices with StreamInterface in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveStream is a wrapper around a stream device class to handle reactive I/O.
    //! Typically, the device class is a subclass of NonBlockingDevice which implements StreamInterface.
    //!
    //! The stream device is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call writeStream() or readStream() on this device and
    //! delegate these operations to startWriteStream() and startReadStream() in class ReactiveStream.
    //!
    class TSCOREDLL ReactiveStream: public ReactiveDevice
    {
        TS_NOBUILD_NOCOPY(ReactiveStream);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] device Associated non-blocking device. The device object must remain valid as long as this object is valid.
        //! @param [in,out] stream Associated stream interface. Typically, the @a device class is a subclass of NonBlockingDevice which
        //! implements StreamInterface. Therefore, @a device and @a stream are two views of the same object. That device object must
        //! remain valid as long as this instance of ReactiveStream exists.
        //!
        ReactiveStream(Reactor& reactor, NonBlockingDevice& device, StreamInterface& stream);

        //!
        //! Get a reference to the associated stream.
        //! Typically, device() and stream() return two views of the same object.
        //! @return A reference to the associated stream.
        //!
        StreamInterface& stream() { return _stream; }

        //!
        //! Default buffer size for receive operations.
        //! @see startReadStream()
        //!
        static constexpr size_t DEFAULT_RECEIVE_BUFFER_SIZE = 4096;

        //!
        //! Start the operation of reading data from the stream.
        //! Reading operation is permanent and @a handler is called whenever incoming data are available.
        //! @param [in] handler Handler class to call each time data are received. The method handleReadStream() will be called
        //! for each new chunk of data. Cannot be null. If a previous receive handler was registered, it is replaced.
        //! @param [in] buffer_size Size of input buffers to receive data.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        virtual bool startReadStream(ReactiveStreamHandlerInterface* handler, size_t buffer_size = DEFAULT_RECEIVE_BUFFER_SIZE, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start the operation of writing data to the stream.
        //! @param [in] handler Handler class to call when the send operation completes. The method handleWriteStream()
        //! will be called. If nullptr, no handler is called.
        //! @param [in] data Address of the data to write. The corresponding memory area must remain valid until the
        //! completion or cancelation of the send operation.
        //! @param [in] size Size in bytes of the data to send.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        virtual bool startWriteStream(ReactiveStreamHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Cancel any pending read or write operation on this stream.
        //! If a repeated read operation is in progress, the repetition is canceled as well.
        //! @param [in] silent If true, do not report errors through the logger.
        //!
        virtual void cancelReadWriteStream(bool silent = false);

    protected:
        //!
        //! Check if some I/O operations are in progress.
        //! @return True if there is any pending operation or permanent read operation in progress. False otherwise.
        //!
        virtual bool hasPendingIO();

        //!
        //! With non-blocking I/O, check if we need to be notified of write-ready conditions.
        //! If overriden by a subclass, the method must also call its superclass counterpart.
        //! @return True if non-blocking write-ready notifications are required. False otherwise.
        //!
        virtual bool needsWriteReady();

        //!
        //! Start closing the write direction of the stream.
        //! The request is enqueued and will be processed by calling processCloseWriteStream() after all previous send
        //! requests are completed. This feature can be used by subclasses for which "closing the write direction of
        //! the stream" means something (e.g. true for TCP socket, meaningless for files).
        //! @param [in] handler Handler class to call when the close-writer operation completes. The method handleWriteStream()
        //! will be called with its parameter @a error_code containing SYS_EOF. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        virtual bool startCloseWriteStream(ReactiveStreamHandlerInterface* handler, bool silent, const ObjectPtr& user_data);

        //!
        //! Implement the close of the write direction of the stream.
        //! Must be implemented by subclasses which use startCloseWriteStream(). The default implementation does nothing.
        //! This method is always called in the context of a Reactor handler, never as an indirect call from the application.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return A system-specific error code. SYS_SUCCESS in case of success.
        //!
        virtual int processCloseWriteStream(bool silent);

        //!
        //! Enqueue an IOSB in the completed I/O queue.
        //! This method can be called by subclasses which need to process specific I/O completion, other than read or write on the stream.
        //! @param [in] iosb Shared pointer to an IOSB. Specific data can be saved in iosb->react_data.
        //! @param [in] in_reactor Set to true if we are in a reactor context, meaning directly called from a reactor handler, not from the application.
        //! @return True on success, false on error.
        //!
        bool enqueueCompletedIO(const std::shared_ptr<IOSB>& iosb, bool in_reactor);

        //!
        //! Try to interpret an IOSB as a valid completed I/O request and process the completion.
        //! If overriden by a subclass, the method must also call its superclass counterpart.
        //! @param [in] iosb Shared pointer to an IOSB. Specific data can be saved in iosb->react_data.
        //! @return True if the @a iosb contained a recognized I/O completion request. False if the request is unknown.
        //!
        virtual bool tryCompletedIO(const std::shared_ptr<IOSB>& iosb);

        //!
        //! Invoke the receive handler as many times as possible on a data buffer.
        //! @param [in,out] data Data buffer containing the received data. On output, data which are processed by the handler are removed.
        //! @param [in,out] control Input control. On input, this is the previously returned value from the handler. Modified by the handler.
        //! @param [in] handler Application handler.
        //! @param [in] error_code Receive error code. If not success, the handler is called exactly once.
        //! @param [in] user_data User data for the handler.
        //!
        void processReceiveBuffer(ByteBlock& data, ReactiveInputControl& control, ReactiveStreamHandlerInterface* handler, int error_code, const ObjectPtr& user_data);

        // Inherited from ReactiveBase.
        virtual void processQueuedOperations() override;

        // Implementation of ReactorHandlerInterface, inherited from ReactiveBase.
        virtual void handleReadReady(Reactor&, EventId, int) override;
        virtual void handleWriteReady(Reactor&, EventId, int) override;
        virtual void handleAsynchronousIO(Reactor&, EventId, IOSB&, size_t) override;

    private:
        // Description of a write request.
        class TSCOREDLL SendRequest: public Object
        {
            TS_NOCOPY(SendRequest);
        public:
            SendRequest() = default;
            virtual ~SendRequest() override;
        public:
            ReactiveStreamHandlerInterface* handler = nullptr;
            const void* data = nullptr;
            size_t      size = 0;
            bool        blocking = false;  // Perform a blocking I/O in the completion queue.
            bool        eof = false;       // Send an end-of-file condition, ie. close the write direction.
            bool        silent = false;    // Used with startCloseWriteStream().
        };

        // Description of a read request.
        class TSCOREDLL ReceiveRequest: public Object
        {
            TS_NOCOPY(ReceiveRequest);
        public:
            ReceiveRequest() = default;
            virtual ~ReceiveRequest() override;
        public:
            ReactiveStreamHandlerInterface* handler = nullptr;
            ReactiveInputControl control {};
            ByteBlock data {};
            bool      blocking = false;  // Perform a blocking I/O in the completion queue.
            bool      new_data = false;  // Some new data were received since last time we examined the buffer.
            size_t    next_read = 0;     // Previously read in data but not yet consumed by application.
            size_t    buffer_size = DEFAULT_RECEIVE_BUFFER_SIZE;
        };

        // ReactiveStream private fields.
        StreamInterface&      _stream;
        IOQueue               _pending_send {};          // Queue of pending send operations, waiting for write-ready or send completion.
        IOQueue               _completed_io {};          // Queue of completed I/O operations, to be notified to application.
        std::shared_ptr<IOSB> _pending_receive {};       // Only one pending receive operation at a time (asynchronous I/O only).

        // Process receive buffer. Must be called in the context of a Reactor handler, when no asynchronous I/O is in progress.
        void processReceiveBuffer();
    };
}
