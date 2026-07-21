//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTextStream.h"
#include "tsSocket.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTextStream::ReactiveTextStream(ReactiveStream& stream, const std::string& eol) :
    _stream(stream),
    _eol(eol)
{
    // Get notified of connection and disconnection if the device is a socket.
    Socket* sock = dynamic_cast<Socket*>(&stream.stream());
    if (sock != nullptr) {
        sock->addSubscription(this);
    }
}

ts::ReactiveTextStream::~ReactiveTextStream()
{
}

ts::ReactiveTextStream::SendUserData::~SendUserData()
{
}


//----------------------------------------------------------------------------
// Clear the flushable buffer on connect and disconnect.
//----------------------------------------------------------------------------

void ts::ReactiveTextStream::handleSocketConnected(TCPConnection& sock)
{
    _unflushed_data.reset();
}

void ts::ReactiveTextStream::handleSocketDisconnected(TCPConnection& sock, bool silent)
{
    _unflushed_data.reset();
}


//----------------------------------------------------------------------------
// Get and send a formatted buffer.
//----------------------------------------------------------------------------

ts::ReactiveTextStream::SendUserDataPtr ts::ReactiveTextStream::getBuffer(bool flush)
{
    // Output buffer is _unflushed_data if not empty, a new empty buffer otherwise.
    SendUserDataPtr buf = _unflushed_data != nullptr ? _unflushed_data : std::make_shared<SendUserData>();

    if (flush) {
        // This I/O is final and the output buffer will be used for asynchronous I/O.
        // Reset _unflushed_data pointer to make sure we use a new buffer next time.
        _unflushed_data.reset();
    }
    else if (_unflushed_data == nullptr) {
        // This I/O is not final. Make sure it will be used next time to append into it.
        _unflushed_data = buf;
    }

    return buf;
}

bool ts::ReactiveTextStream::startSendData(SendUserDataPtr& buf, bool eol, bool flush)
{
    // Add CR-LF (Telnet protocol end-of-line marker) when necessary.
    if (eol) {
        buf->buffer.append(_eol);
    }

    // Start the I/O when necessay. The buffer pointer is used as user-data to make sure that the shared pointer
    // is saved as long as the I/O is in progress. Thus, we guarantee that 1) the buffer remains valid during
    // the I/O and 2) it is automatically freed as the end of the I/O.
    return !flush || _stream.startWriteStream(nullptr, buf->buffer.data(), buf->buffer.size(), buf);
}


//----------------------------------------------------------------------------
// Start the operation of sending text over the stream device.
//----------------------------------------------------------------------------

bool ts::ReactiveTextStream::startWriteLine(const std::string& line, bool flush)
{
    auto buf = getBuffer(flush);
    buf->buffer.append(line);
    return startSendData(buf, true, flush);
}

bool ts::ReactiveTextStream::startWriteLine(const UString& line, bool flush)
{
    auto buf = getBuffer(flush);
    line.appendUTF8(buf->buffer);
    return startSendData(buf, true, flush);
}

bool ts::ReactiveTextStream::startWriteText(const std::string& line, bool flush)
{
    auto buf = getBuffer(flush);
    buf->buffer.append(line);
    return startSendData(buf, false, flush);
}

bool ts::ReactiveTextStream::startWriteText(const UString& line, bool flush)
{
    auto buf = getBuffer(flush);
    line.appendUTF8(buf->buffer);
    return startSendData(buf, false, flush);
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the stream device.
//----------------------------------------------------------------------------

bool ts::ReactiveTextStream::startReadText(ReactiveTextStreamHandlerInterface* handler, size_t buffer_size)
{
    // The handler cannot be null because there is no other way to get received data.
    if (handler == nullptr) {
        _stream.report().error(u"internal error: null handler in ReactiveTextStream::startReceive");
        return false;
    }
    else {
        _receive_handler = handler;
        return _stream.startReadStream(this, buffer_size);
    }
}


//----------------------------------------------------------------------------
// Invoked when binary data is received from the stream device.
//----------------------------------------------------------------------------

void ts::ReactiveTextStream::handleReadStream(ReactiveStream& stream, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data)
{
    if (_receive_handler != nullptr) {
        if (!SysSuccess(error_code)) {
            // Report an error to application.
            _receive_handler->handleTextLine(*this, UString(), error_code);
        }
        else {
            // Loop on searching for line-feed. Stop if a handler stopped the socket.
            size_t lf = NPOS;
            size_t start = 0;
            while (start < data.size() && (lf = data.find('\n', start)) != NPOS && _stream.stream().isReadStream()) {
                assert(lf >= start);
                assert(lf < data.size());
                // Found a complete line.
                ts::UString line;
                line.assignFromUTF8(reinterpret_cast<const char*>(data.data() + start), lf - start);
                // Remove potential carriage-return before line-feed.
                while (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                // Call the application.
                _receive_handler->handleTextLine(*this, line, error_code);
                // Look for next line-feed.
                start = lf + 1;
            }
            // Indicate where we stopped consuming the buffer.
            control.used_size = start;
            // Indicate that we need a line-feed;
            control.next_delimiter = '\n';
        }
    }
}
