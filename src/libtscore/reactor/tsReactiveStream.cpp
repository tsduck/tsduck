//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveStream.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveStream::ReactiveStream(Reactor& reactor, NonBlockingDevice& device, StreamInterface& stream) :
    ReactiveDevice(reactor, device),
    _stream(stream)
{
}

ts::ReactiveStream::SendRequest::~SendRequest()
{
}

ts::ReactiveStream::ReceiveRequest::~ReceiveRequest()
{
}


//----------------------------------------------------------------------------
// Check if some I/O operations are in progress.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::hasPendingIO()
{
    return !_completed_io.empty() || !_pending_send.empty() || _pending_receive != nullptr;
}


//----------------------------------------------------------------------------
// With non-blocking I/O, check if we need to be notified of write-ready.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::needsWriteReady()
{
    // We need write-ready notifications when some send operations are pending.
    return ReactorSupport::UseNonBlockingIO() && !_pending_send.empty();
}


//----------------------------------------------------------------------------
// Implement the close of the write direction of the stream.
//----------------------------------------------------------------------------

int ts::ReactiveStream::processCloseWriteStream(bool silent)
{
    // The default implementation does nothing.
    return SYS_SUCCESS;
}


//----------------------------------------------------------------------------
// Enqueue an IOSB in the completed I/O queue.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::enqueueCompletedIO(const std::shared_ptr<IOSB>& iosb, bool in_reactor)
{
    _completed_io.push_back(iosb);
    return in_reactor || signalQueuedOperations();
}


//----------------------------------------------------------------------------
// Try to interpret an IOSB as a valid completed I/O request and process.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::tryCompletedIO(const std::shared_ptr<IOSB>& iosb)
{
    // At the level of this class, we only handle send operations.
    const std::shared_ptr<SendRequest> req = std::dynamic_pointer_cast<SendRequest>(iosb->react_data);
    if (req == nullptr) {
        // Don't know this request.
        return false;
    }
    else {
        // Completed send request or send eof request.
        if (req->eof) {
            // Call subclass to perform actual close of the write side.
            iosb->error_code = processCloseWriteStream(req->silent);
            // The error code after a successful close-write operation is SYS_EOF.
            if (SysSuccess(iosb->error_code)) {
                iosb->error_code = SYS_EOF;
            }
        }
        else if (req->blocking) {
            // Perform a blocking write (device does not support insertion in reactor).
            const uint8_t* addr = reinterpret_cast<const uint8_t*>(req->data);
            size_t remain = req->size;
            while (remain > 0) {
                size_t ret_size = 0;
                if (_stream.writeStream(addr, remain, ret_size, iosb.get())) {
                    assert(ret_size <= remain);
                    assert(!iosb->pending);
                    addr += ret_size;
                    remain -= ret_size;
                    iosb->error_code = SYS_SUCCESS;
                }
                else {
                    iosb->error_code = LastSysErrorCode();
                    break;
                }
            }
        }
        if (req->handler != nullptr) {
            req->handler->handleWriteStream(*this, iosb->error_code, iosb->app_data);
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Process completed I/O operations.
//----------------------------------------------------------------------------

void ts::ReactiveStream::processQueuedOperations()
{
    reactor().trace(u"ReactiveStream: processing %d completed I/O", _completed_io.size());

    // Process completed I/O.
    while (!_completed_io.empty()) {

        // Remove the front completed IOSB.
        std::shared_ptr<IOSB> iosb = _completed_io.front();
        _completed_io.pop_front();

        // Let the cascade of subclasses try to handle the request.
        if(!tryCompletedIO(iosb)) {
            // Completed request wasn't recognized by the subclass.
            report().error(u"internal error in ReactiveStream: unexpected type of completed request");
        }
    }

    // If a blocking read is required, read now (device does not support insertion in reactor).
    if (_pending_receive != nullptr) {
        auto req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
        if (req != nullptr && req->blocking) {
            handleReadReady(reactor(), EventId(), SYS_SUCCESS);
        }
    }
}


//----------------------------------------------------------------------------
// Start the operation of sending data over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::startWriteStream(ReactiveStreamHandlerInterface* handler, const void* data, size_t size, const ObjectPtr& user_data)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Allocate a send request structure.
    auto req = std::make_shared<SendRequest>();
    req->handler = handler;
    req->data = data;
    req->size = size;
    req->blocking = !device().isSupportedByReactor();
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    if (req->blocking) {
        // Cannot be notified of write-ready conditions, assumed to be "always ready".
        // Will perform a blocking write (supposed to not block too long) in pot-processing queue.
        enqueueCompletedIO(iosb, false);
    }
    else if constexpr (ReactorSupport::UseNonBlockingIO()) {
        // Enqueue the send request. Will be sent when write is possible.
        _pending_send.push_back(iosb);
        // Make sure to be notified when a send operation is possible.
        return activateWriteReady();
    }
    else if constexpr (ReactorSupport::UseAsynchronousIO()) {
        // Start the asynchronous send.
        if (!_stream.writeStream(data, size, iosb.get())) {
            // Failed to start the operation.
            return false;
        }
        // The send operation is in progress and its completion will be notified later.
        _pending_send.push_back(iosb);
    }
    return true;
}


//----------------------------------------------------------------------------
// Start closing the send direction of the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::startCloseWriteStream(ReactiveStreamHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    // Asynchronous I/O model: Be notified of I/O completion.
    if (!activateAsynchronousIO()) {
        return false;
    }

    // Allocate a close-write request structure as a send request.
    auto req = std::make_shared<SendRequest>();
    req->handler = handler;
    req->eof = true;
    req->silent = silent;
    auto iosb = std::make_shared<IOSB>();
    iosb->app_data = user_data;
    iosb->react_data = req;

    // Enqueue the close-write request.
    if (_pending_send.empty()) {
        // No pending send request, directly enqueue in completed I/O.
        return enqueueCompletedIO(iosb, false);
    }
    else {
        // Will be executed after all pending requests.
        _pending_send.push_back(iosb);
        return true;
    }
}


//----------------------------------------------------------------------------
// Called in the Reactor context when a send operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveStream::handleWriteReady(Reactor& reactor, EventId id, int error_code)
{
    // While it is possible to write on this stream, try to send all pending messages.
    while (_stream.isWriteStream() && !_pending_send.empty()) {

        // Remove the front send request.
        auto iosb = _pending_send.front();
        _pending_send.pop_front();

        // The send request is in the application data of the IOSB.
        auto req = std::dynamic_pointer_cast<SendRequest>(iosb->react_data);
        assert(req != nullptr);

        // EOF requests are directly enqueued in completed I/O.
        if (req->eof) {
            _completed_io.push_back(iosb);
            continue;
        }

        // Try to writeStream the message.
        const bool success = _stream.writeStream(req->data, req->size, iosb.get());
        if (iosb->pending) {
            // Would block again.
            assert(iosb->sent_size < req->size);
            if (iosb->sent_size > 0) {
                // The beginning of the data buffer was sent, split the send request in two.
                auto req0 = std::make_shared<SendRequest>();
                req0->handler = req->handler;
                req0->data = req->data;
                req0->size = iosb->sent_size;
                auto iosb0 = std::make_shared<IOSB>();
                iosb0->app_data = iosb->app_data;
                iosb0->react_data = req0;
                // The first part is moved to the completed I/O queue.
                _completed_io.push_back(iosb0);
                // Adjust the remaining data to be sent later.
                req->data = reinterpret_cast<const uint8_t*>(req->data) + iosb->sent_size;
                req->size -= iosb->sent_size;
            }
            // Put request back where it was in the queue and retry later.
            _pending_send.push_front(iosb);
            break;
        }
        else {
            // Immediate success or error.
            iosb->error_code = success ? SYS_SUCCESS : LastSysErrorCode();
            // Move the request to the completed queue.
            enqueueCompletedIO(iosb, true);
        }
    }

    // Now process all completed sent messages.
    processQueuedOperations();

    // When no more pending send operation, stop being notified of send-ready.
    if (!needsWriteReady()) {
        deactivateWriteReady(false);
    }
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveStream::startReadStream(ReactiveStreamHandlerInterface* handler, size_t buffer_size, const ObjectPtr& user_data)
{
    reactor().trace(u"ReactiveStream: start receive");

    // Unlike send operation, the handler cannot be null because there is no other way to get received data.
    if (handler == nullptr) {
        report().error(u"internal error: null handler in ReactiveStream::startReadStream");
        return false;
    }

    // Don't use empty or too small reception buffers.
    buffer_size = std::max<size_t>(256, buffer_size);

    // Check if there is an existing read cycle in progress.
    std::shared_ptr<ReceiveRequest> req;
    if (_pending_receive != nullptr) {
        req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
    }
    if (req != nullptr) {
        // Simply update the request. Do not modify/resize the data buffer, an asynchronous I/O may be in progress.
        req->handler = handler;
        req->buffer_size = buffer_size;
        _pending_receive->app_data = user_data;
        return true;
    }

    // Asynchronous I/O model: Be notified of I/O completion.
    // Non-blocking I/O model: Get notified when a reception operation is possible, if not already done.
    if (!activateAsynchronousIO() || !activateReadReady()) {
        return false;
    }

    // Create the receive request
    req = std::make_shared<ReceiveRequest>();
    req->handler = handler;
    req->buffer_size = buffer_size;
    req->data.resize(buffer_size);
    req->blocking = !device().isSupportedByReactor();
    _pending_receive = std::make_shared<IOSB>();
    _pending_receive->app_data = user_data;
    _pending_receive->react_data = req;

    if (req->blocking) {
        // Cannot be notified of read-ready conditions, assumed to be "always ready".
        // Will perform a blocking read (supposed to not block too long) in pot-processing queue.
        signalQueuedOperations();
    }
    else if constexpr (ReactorSupport::UseAsynchronousIO()) {
        // Start the first receive operation. Even if it immediately completes, an asynchronous I/O completion will be posted.
        size_t retsize = 0;
        if (!_stream.readStream(req->data.data(), req->data.size(), retsize, nullptr, _pending_receive.get())) {
            _pending_receive.reset();
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Called when a readStream operation is possible.
// Called only in the non-blocking I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveStream::handleReadReady(Reactor& reactor, EventId id, int error_code)
{
    // Loop on possible reception. Check that the socket is still open (can have been closed in a handler).
    while (_stream.isReadStream() && _pending_receive != nullptr) {

        auto req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
        assert(req != nullptr);
        assert(req->next_read <= req->data.size());

        // Enlarge buffer if no room left.
        if (req->next_read == req->data.size()) {
            req->data.resize(req->data.size() + req->buffer_size);
        }

        // Try to receive once.
        size_t retsize = 0;
        if (!_stream.readStream(req->data.data() + req->next_read, req->data.size() - req->next_read, retsize, nullptr, _pending_receive.get())) {
            // Receive error.
            req->new_data = true;
            _pending_receive->error_code = LastSysErrorCode();
            // Report the error to the application. This will also stop reception.
            processReceiveBuffer();
        }
        else if (_pending_receive->pending) {
            // No receive is possible now, retry later on read-notification.
            assert(!req->blocking);
            break;
        }
        else {
            // Receive completed successfully.
            req->new_data = true;
            req->next_read += retsize;
            // Successfully receiving zero means end of connection.
            _pending_receive->error_code = retsize == 0 ? SYS_EOF : SYS_SUCCESS;
            // Let the application process the buffer.
            processReceiveBuffer();
        }
    }

    // Process any completed I/O.
    processQueuedOperations();
}


//----------------------------------------------------------------------------
// Process receive buffer in the context of a Reactor handler.
//----------------------------------------------------------------------------

void ts::ReactiveStream::processReceiveBuffer()
{
    assert(_pending_receive != nullptr);
    auto req = std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data);
    assert(req != nullptr);

    // Mark the buffer as processed.
    req->new_data = false;

    // Resize the data buffer to the exact received size for the handler.
    // Keep the previous size, will restore it later, hoping that there will be no reallocation / memory move.
    const size_t previous_size = req->data.size();
    req->data.resize(req->next_read);

    // Loop on calls to handler, when the handler uses only a part of the buffer.
    processReceiveBuffer(req->data, req->control, req->handler, _pending_receive->error_code, _pending_receive->app_data);

    // If receive buffer processing did not cancel reception.
    if (_pending_receive != nullptr) {
        // In case of receive error, cancel receive.
        if (!SysSuccess(_pending_receive->error_code)) {
            deactivateReadReady(true);  // non-blocking IO only
            _pending_receive.reset();
        }
        else {
            // Restore previous buffer size, hoping that there will be no reallocation / memory move.
            assert(req == std::dynamic_pointer_cast<ReceiveRequest>(_pending_receive->react_data));
            assert(previous_size >= req->data.size());
            req->next_read = req->data.size();
            req->data.resize(previous_size);
        }
    }
}


//----------------------------------------------------------------------------
// Invoke the receive handler as many times as possible on a data buffer.
//----------------------------------------------------------------------------

void ts::ReactiveStream::processReceiveBuffer(ByteBlock& data, ReactiveInputControl& control, ReactiveStreamHandlerInterface* handler, int error_code, const ObjectPtr& user_data)
{
    // Loop on calls to handler, when the handler uses only a part of the buffer.
    // Check if we have the required condition to call the handler.
    // Always call the handler once in case of error.
    while (!SysSuccess(error_code) ||
           (!data.empty() &&
            // Need a minimum amount of received data.
            (!control.min_next_size.has_value() || data.size() >= control.min_next_size.value()) &&
            // Need a specific delimiter byte in the received data.
            (!control.next_delimiter.has_value() || data.find(control.next_delimiter.value()) != NPOS)))
    {
        // Default values for the input control, may be updated later by the handler.
        control.reset();

        // Call the handler with the content of the buffer.
        if (handler != nullptr) {
            handler->handleReadStream(*this, data, control, error_code, user_data);
        }

        // In case of receive error, call the handler only once.
        if (!SysSuccess(error_code)) {
            break;
        }

        // Adjust unread data in the buffer.
        if (control.used_size.value_or(NPOS) < data.size()) {
            // The buffer was not entirely read by the application, compact it.
            data.erase(0, control.used_size.value());
        }
        else {
            // The buffer was entirely read, no need to move memory.
            data.clear();
        }
    }
}


//----------------------------------------------------------------------------
// Called when an asynchronous I/O completes.
// Called only in the asynchronous I/O model.
//----------------------------------------------------------------------------

void ts::ReactiveStream::handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size)
{
    // At this point, we only have an IOSB, not a std::shared_ptr<IOSB>.
    // Our custom request is in the react_data of the IOSB.
    std::shared_ptr<ReceiveRequest> recv;
    std::shared_ptr<SendRequest> send;

    if ((send = std::dynamic_pointer_cast<SendRequest>(iosb.react_data)) != nullptr) {
        // Got a completed send request. Search for the request in the queue of pending send requests.
        std::shared_ptr<IOSB> req = removeFromQueue(_pending_send, &iosb);
        if (req == nullptr) {
            report().error(u"unreferenced completed asynchronous TCP send request");
        }
        else {
            // Adjust the stream object with the result.
            _stream.asyncCompletedStream(&iosb);
            // Enqueue completed send request.
            _completed_io.push_back(req);
            // If next send request is an eof request, also pass it to completed I/O queue.
            if (!_pending_send.empty()) {
                std::shared_ptr<SendRequest> next_send = std::dynamic_pointer_cast<SendRequest>(_pending_send.front()->react_data);
                if (next_send != nullptr && next_send->eof) {
                    _completed_io.push_back(_pending_send.front());
                    _pending_send.pop_front();
                }
            }
        }
    }
    else if ((recv = std::dynamic_pointer_cast<ReceiveRequest>(iosb.react_data)) != nullptr) {
        // Got a completed receive request.
        // There is only one pending receive request at a time. Is it this one?
        if (_pending_receive.get() != &iosb) {
            report().error(u"unreferenced completed asynchronous TCP receive request");
        }
        else if (_pending_receive->error_code == SYS_CANCELED) {
            // Ignore canceled receive requests. A receive operation is always active. When canceled, this is the
            // symptom of socket being closed. A send request, on the other hand, is an explicit message from the
            // application which expects a notification at the end of it, potentially canceled.
            _pending_receive.reset();
        }
        else {
            // Adjust the stream object with the result.
            _stream.asyncCompletedStream(&iosb);
            // Successfully receiving zero means end of connection.
            if (SysSuccess(_pending_receive->error_code) && io_size == 0) {
                _pending_receive->error_code = SYS_EOF;
            }
            // Update the request result.
            recv->new_data = true;
            recv->next_read = std::min(recv->next_read + io_size, recv->data.size());
            // Directly call processReceiveBuffer() because we are in reactor handler context.
            processReceiveBuffer();
            // Start the next receive operation if necesary.
            if (_pending_receive == nullptr || !SysSuccess(_pending_receive->error_code) || !_stream.isReadStream()) {
                // There was a receive error or the stream is closed, maybe from a handler, stop receiving.
                _pending_receive.reset();
            }
            else {
                // Enlarge buffer if no room left.
                if (recv->next_read >= recv->data.size()) {
                    recv->data.resize(recv->data.size() + recv->buffer_size);
                }
                // Start the next receive.
                size_t retsize = 0;
                if (!_stream.readStream(recv->data.data() + recv->next_read, recv->data.size() - recv->next_read, retsize, nullptr, _pending_receive.get())) {
                    // Failed to start a new receive, stop receiving.
                    _pending_receive.reset();
                }
            }
        }
    }
    else {
        report().error(u"unexpected completed asynchronous TCP request, error code: %d, I/O size: %d", iosb.error_code, io_size);
    }

    // Process any completed I/O.
    processQueuedOperations();
}


//----------------------------------------------------------------------------
// Cancel any pending send or receive operation on this socket.
//----------------------------------------------------------------------------

void ts::ReactiveStream::cancelReadWriteStream(bool silent)
{
    // Stop being notified of send/receive-ready.
    deactivateReadReady(silent);
    deactivateWriteReady(silent);

    // Cancel asynchronous I/O currently in progress.
    cancelAsynchronousIO(silent);

    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        // Discard pending send and receive requests, they are not started yet.
        _pending_send.clear();
        _pending_receive.reset();
    }
}
