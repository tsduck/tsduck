//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  TCP connection using TLV messages.
//
//----------------------------------------------------------------------------

#include "tstlvMessageFactory.h"
#include "tsGuardMutex.h"

//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

template <class MUTEX>
ts::tlv::Connection<MUTEX>::Connection(const Protocol* protocol, bool auto_error_response, size_t max_invalid_msg) :
    ts::TCPConnection(),
    _protocol(protocol),
    _auto_error_response(auto_error_response),
    _max_invalid_msg(max_invalid_msg),
    _invalid_msg_count(0),
    _send_mutex(),
    _receive_mutex()
{
}


//----------------------------------------------------------------------------
// Invoked when connection is established.
//----------------------------------------------------------------------------

// With MSVC, we get a bogus warning:
// warning C4505: 'ts::tlv::Connection<ts::Mutex>::handleConnected': unreferenced local function has been removed
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4505)

template <class MUTEX>
void ts::tlv::Connection<MUTEX>::handleConnected(Report& report)
{
    SuperClass::handleConnected(report);
    _invalid_msg_count = 0;
}

TS_POP_WARNING()


//----------------------------------------------------------------------------
// Serialize and send a TLV message.
//----------------------------------------------------------------------------

template <class MUTEX>
bool ts::tlv::Connection<MUTEX>::send(const Message& msg, Report& report)
{
    tlv::Logger logger(Severity::Debug, &report);
    return send(msg, logger);
}

template <class MUTEX>
bool ts::tlv::Connection<MUTEX>::send(const Message& msg, Logger& logger)
{
    logger.log(msg, u"sending message to " + peerName());

    ByteBlockPtr bbp(new ByteBlock);
    Serializer serial(bbp);
    msg.serialize(serial);

    GuardMutex lock(_send_mutex);
    return SuperClass::send(bbp->data(), bbp->size(), logger.report());
}


//----------------------------------------------------------------------------
// Receive a TLV message (wait for the message, deserialize it and validate it)
//----------------------------------------------------------------------------

template <class MUTEX>
bool ts::tlv::Connection<MUTEX>::receive(MessagePtr& msg, const AbortInterface* abort, Report& report)
{
    tlv::Logger logger(Severity::Debug, &report);
    return receive(msg, abort, logger);
}

template <class MUTEX>
bool ts::tlv::Connection<MUTEX>::receive(MessagePtr& msg, const AbortInterface* abort, Logger& logger)
{
    const bool has_version(_protocol->hasVersion());
    const size_t header_size(has_version ? 5 : 4);
    const size_t length_offset(has_version ? 3 : 2);

    // Loop until a valid message is received
    for (;;) {
        ByteBlock bb(header_size);

        // Receive complete message
        {
            GuardMutex lock(_receive_mutex);

            // Read message header
            if (!SuperClass::receive(bb.data(), header_size, abort, logger.report())) {
                return false;
            }

            // Get message length and read message payload
            const size_t length = GetUInt16(bb.data() + length_offset);
            bb.resize(header_size + length);
            if (!SuperClass::receive(bb.data() + header_size, length, abort, logger.report())) {
                return false;
            }
        }

        // Analyze the message
        MessageFactory mf(bb.data(), bb.size(), _protocol);
        if (mf.errorStatus() == tlv::OK) {
            _invalid_msg_count = 0;
            mf.factory(msg);
            if (!msg.isNull()) {
                logger.log(*msg, u"received message from " + peerName());
            }
            return true;
        }

        // Received an invalid message
        _invalid_msg_count++;

        // Send back an error message if necessary
        if (_auto_error_response) {
            MessagePtr resp;
            mf.buildErrorResponse(resp);
            if (!send(*resp, logger.report())) {
                return false;
            }
        }

        // If invalid message max has been reached, break the connection
        if (_max_invalid_msg > 0 && _invalid_msg_count >= _max_invalid_msg) {
            logger.report().error(u"too many invalid messages from %s, disconnecting", {peerName()});
            disconnect(logger.report());
            return false;
        }
    }
}
