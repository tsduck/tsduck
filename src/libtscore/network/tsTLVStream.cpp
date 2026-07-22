//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLVStream.h"
#include "tsTCPConnection.h"
#include "tstlvMessageFactory.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLVStream::TLVStream(tlv::Logger& logger, const tlv::Protocol& protocol, StreamInterface& stream, bool auto_error_response, size_t max_invalid_msg) :
    _logger(logger),
    _protocol(protocol),
    _stream(stream),
    _socket(dynamic_cast<TCPConnection*>(&_stream)),
    _auto_error_response(auto_error_response),
    _max_invalid_msg(max_invalid_msg)
{
    // Get notified of connection and disconnection if the device is a socket.
    if (_socket != nullptr) {
        _socket->addSubscription(this);
    }
}


//----------------------------------------------------------------------------
// Socket handler interface. Invoked when connection is established.
//----------------------------------------------------------------------------

void ts::TLVStream::handleSocketConnected(TCPConnection& sock)
{
    _invalid_msg_count = 0;
}


//----------------------------------------------------------------------------
// Serialize and send a TLV message.
//----------------------------------------------------------------------------

bool ts::TLVStream::sendMessage(const tlv::Message& msg)
{
    if (_socket != nullptr) {
        _logger.log(msg, u"sending message to " + _socket->peerName());
    }
    else {
        _logger.log(msg, u"sending message");
    }

    ByteBlockPtr bbp = std::make_shared<ByteBlock>();
    tlv::Serializer serial(bbp);
    msg.serialize(serial);

    std::lock_guard<std::mutex> lock(_send_mutex);
    return _stream.writeStream(bbp->data(), bbp->size());
}


//----------------------------------------------------------------------------
// Receive a TLV message (wait for the message, deserialize and validate).
//----------------------------------------------------------------------------

bool ts::TLVStream::receiveMessage(tlv::MessagePtr& msg, const AbortInterface* abort)
{
    // Loop until a valid message is received
    for (;;) {
        const size_t header_size = _protocol.headerSize();
        ByteBlock bb(header_size);

        // Receive complete message
        {
            std::lock_guard<std::mutex> lock(_receive_mutex);

            // Read message header
            if (!_stream.readStream(bb.data(), header_size, abort)) {
                return false;
            }

            // Get message length and read message payload
            const size_t length = GetUInt16(bb.data() + _protocol.lengthOffset());
            bb.resize(header_size + length);
            if (!_stream.readStream(bb.data() + header_size, length, abort)) {
                return false;
            }
        }

        // Analyze the message
        tlv::MessageFactory mf(bb.data(), bb.size(), _protocol);
        if (mf.errorStatus() == tlv::OK) {
            _invalid_msg_count = 0;
            mf.factory(msg);
            if (msg != nullptr) {
                // Log the message, usually for debug or trace purpose.
                if (_socket != nullptr) {
                    _logger.log(*msg, u"received message from " + _socket->peerName());
                }
                else {
                    _logger.log(*msg, u"received message");
                }
            }
            return true;
        }

        // Received an invalid message
        _invalid_msg_count++;

        // Send back an error message if necessary
        if (_auto_error_response) {
            tlv::MessagePtr resp;
            mf.buildErrorResponse(resp);
            if (!sendMessage(*resp)) {
                return false;
            }
        }

        // If invalid message max has been reached, break the connection or report an error.
        if (_max_invalid_msg > 0 && _invalid_msg_count >= _max_invalid_msg) {
            if (_socket != nullptr) {
                _logger.report().error(u"too many invalid messages from %s, disconnecting", _socket->peerName());
                _socket->disconnect();
            }
            else {
                _logger.report().error(u"too many invalid messages, aborting");
            }
            return false;
        }
    }
}
