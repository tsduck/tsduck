//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTelnetConnection.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTelnetConnection::ReactiveTelnetConnection(ReactiveTCPConnection& socket, Object* owner) :
    ReactiveBase(socket.reactor(), socket.socket(), owner),
    _socket(socket)
{
    // Get notified of connection and disconnection.
    _socket.socket().addSubscription(this);
}

ts::ReactiveTelnetConnection::~ReactiveTelnetConnection()
{
}

ts::ReactiveTelnetConnection::SendUserData::~SendUserData()
{
}


//----------------------------------------------------------------------------
// Clear the flushable buffer on connect and disconnect.
//----------------------------------------------------------------------------

void ts::ReactiveTelnetConnection::handleSocketConnected(TCPConnection& sock)
{
    _unflushed_data.reset();
}

void ts::ReactiveTelnetConnection::handleSocketDisconnected(TCPConnection& sock, bool silent)
{
    _unflushed_data.reset();
}


//----------------------------------------------------------------------------
// Get and send a formatted buffer.
//----------------------------------------------------------------------------

ts::ReactiveTelnetConnection::SendUserDataPtr ts::ReactiveTelnetConnection::getBuffer(bool flush)
{
    SendUserDataPtr buf = _unflushed_data != nullptr ? _unflushed_data : std::make_shared<SendUserData>();
    if (flush) {
        _unflushed_data.reset();
    }
    else if (_unflushed_data == nullptr) {
        _unflushed_data = buf;
    }
    return buf;
}

bool ts::ReactiveTelnetConnection::startSendData(SendUserDataPtr& buf, bool eol, bool flush)
{
    if (eol) {
        buf->buffer.append("\r\n");
    }
    return !flush || _socket.startSend(nullptr, buf->buffer.data(), buf->buffer.size(), buf);
}

//----------------------------------------------------------------------------
// Start the operation of sending text over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveTelnetConnection::startSendLine(const std::string& line, bool flush)
{
    auto buf = getBuffer(flush);
    buf->buffer.append(line);
    return startSendData(buf, true, flush);
}

bool ts::ReactiveTelnetConnection::startSendLine(const UString& line, bool flush)
{
    auto buf = getBuffer(flush);
    line.appendUTF8(buf->buffer);
    return startSendData(buf, true, flush);
}

bool ts::ReactiveTelnetConnection::startSendText(const std::string& line, bool flush)
{
    auto buf = getBuffer(flush);
    buf->buffer.append(line);
    return startSendData(buf, false, flush);
}

bool ts::ReactiveTelnetConnection::startSendText(const UString& line, bool flush)
{
    auto buf = getBuffer(flush);
    line.appendUTF8(buf->buffer);
    return startSendData(buf, false, flush);
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTelnetConnection::startReceive(ReactiveTelnetConnectionHandlerInterface* handler, size_t buffer_size)
{
    // The handler cannot be null because there is no other way to get received data.
    if (handler == nullptr) {
        report().error(u"internal error: null handler in ReactiveTelnetConnection::startReceive");
        return false;
    }
    else {
        _receive_handler = handler;
        return _socket.startReceive(this, buffer_size);
    }
}


//----------------------------------------------------------------------------
// Invoked when binary data is received from the TCP connection.
//----------------------------------------------------------------------------

void ts::ReactiveTelnetConnection::handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveTCPInputControl& control, int error_code, const ObjectPtr& user_data)
{
    if (_receive_handler != nullptr) {
        if (error_code != SYS_SUCCESS) {
            // Report an error to application.
            _receive_handler->handleTelnetLine(*this, UString(), error_code);
        }
        else {
            // Loop on searching for line-feed. Stop if a handler stopped the socket.
            size_t lf = NPOS;
            size_t start = 0;
            while (start < data.size() && (lf = data.find('\n', start)) != NPOS && sock.isOpen()) {
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
                _receive_handler->handleTelnetLine(*this, line, error_code);
                // Look for next line-feed.
                start = lf + 1;
            }
            // Indicate where we stopped consuming the buffer.
            control.used_size = start;
            // Indicate that we need a line-feed;
            control.min_next_size = NPOS;
            control.next_delimiter = '\n';
        }
    }
}
