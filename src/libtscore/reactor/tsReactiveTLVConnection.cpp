//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLVConnection.h"
#include "tstlvProtocol.h"
#include "tstlvMessageFactory.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTLVConnection::ReactiveTLVConnection(tlv::Logger& logger, const tlv::Protocol& protocol, ReactiveTCPConnection& socket, bool auto_error_response, size_t max_invalid_msg) :
    _logger(logger),
    _protocol(protocol),
    _socket(socket),
    _auto_error_response(auto_error_response),
    _max_invalid_msg(max_invalid_msg)
{
}

ts::ReactiveTLVConnection::~ReactiveTLVConnection()
{
}

ts::ReactiveTLVConnection::SendUserData::~SendUserData()
{
}


//----------------------------------------------------------------------------
// Start the operation of sending a message over the TCP connection.
//----------------------------------------------------------------------------

bool ts::ReactiveTLVConnection::startSendMessage(const tlv::Message& msg)
{
    _logger.log(msg, u"sending message to " + _socket.socket().peerName());

    // Allocate a buffer into which we serialize the message. We need a ByteBlockPtr to serialize the message.
    // We need to encapsulate it into a subclass of Object to use it as user-data for asynchronous I/O.
    SendUserDataPtr buf = std::make_shared<SendUserData>();
    buf->buffer = std::make_shared<ByteBlock>();
    tlv::Serializer serial(buf->buffer);
    msg.serialize(serial);

    // Start the I/O. The buffer pointer is used as user-data to make sure that the shared pointer is
    // saved as long as the I/O is in progress. Thus, we guarantee that 1) the buffer remains valid
    // during the I/O and 2) it is automatically freed as the end of the I/O.
    return _socket.startSend(nullptr, buf->buffer->data(), buf->buffer->size(), buf);
}


//----------------------------------------------------------------------------
// Start the operation of receiving messages from the socket.
//----------------------------------------------------------------------------

bool ts::ReactiveTLVConnection::startReceive(ReactiveTLVConnectionHandlerInterface* handler, size_t buffer_size)
{
    // The handler cannot be null because there is no other way to get received data.
    if (handler == nullptr) {
        _logger.report().error(u"internal error: null handler in ReactiveTLVConnection::startReceive");
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

void ts::ReactiveTLVConnection::handleTCPReceive(ReactiveTCPConnection& sock, const ByteBlock& data, ReactiveInputControl& control, int error_code, const ObjectPtr& user_data)
{
    // Ignore all inputs if no handler is defined.
    if (_receive_handler == nullptr) {
        return;
    }

    // Report errors to application.
    if (!SysSuccess(error_code)) {
        _receive_handler->handleReceivedMessage(*this, nullptr, error_code);
        return;
    }

    // We need at least the size of a header.
    const size_t header_size = _protocol.headerSize();
    if (data.size() < header_size) {
        control.used_size = 0;
        control.min_next_size = header_size;
        return;
    }

    // Get total message size, header and payload. Make sure we have enough to decode the message.
    const size_t msg_size = header_size + GetUInt16(data.data() + _protocol.lengthOffset());
    if (data.size() < msg_size) {
        control.used_size = 0;
        control.min_next_size = msg_size;
        return;
    }

    // Indicate where we stop consuming the buffer.
    control.used_size = msg_size;

    // Analyze the message.
    tlv::MessagePtr msg;
    tlv::MessageFactory mf(data.data(), msg_size, _protocol);
    if (mf.errorStatus() == tlv::OK) {
        mf.factory(msg);
        if (msg != nullptr) {
            // Successful decoding of a message.
            // Reset the number of successive invalid messages.
            _invalid_msg_count = 0;

            // Log the message, usually for debug or trace purpose.
            _logger.log(*msg, u"received message from " + _socket.socket().peerName());

            // Call the application.
            _receive_handler->handleReceivedMessage(*this, msg, error_code);
            return;
        }
    }

    // At this point, the message has not been successfully decoded.
    _invalid_msg_count++;

    // Send back an error message if necessary
    if (_auto_error_response) {
        tlv::MessagePtr resp;
        mf.buildErrorResponse(resp);
        startSendMessage(*resp);
    }

    // If invalid message max has been reached, break the connection.
    if (_max_invalid_msg > 0 && _invalid_msg_count >= _max_invalid_msg) {
        // In practice, we report and end-of-session to allow the application to trigger and handle an asynchronous disconnection.
        _receive_handler->handleReceivedMessage(*this, nullptr, SYS_EOF);
    }
}
