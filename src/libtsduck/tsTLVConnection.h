//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#pragma once
#include "tsTCPConnection.h"
#include "tsTLVProtocol.h"
#include "tsMutex.h"
#include "tsTLVMessage.h"

namespace ts {
    namespace tlv {

        // Serialization & deserialization need synchronized access.
        // By default, use thread-safe implementation.
        // Instantiate with MUTEX = NullMutex for mono-thread appli.

        template <class MUTEX = Mutex>
        class Connection: public ts::TCPConnection
        {
        public:
            typedef ts::TCPConnection SuperClass;

            // Constructor.
            // The incoming messages are interpreted according to the specified
            // protocol. When an invalid message is received, the corresponding
            // error message is automatically sent back to the sender when
            // auto_error_response is true. If max_invalid_msg is non-zero,
            // the connection is automatically disconnected when the number of
            // consecutive invalid messages has reached this value.
            Connection (const Protocol*, bool auto_error_response = true, size_t max_invalid_msg = 0);

            // Serialize and send a TLV message.
            bool send (const Message&, ReportInterface&);

            // Receive a TLV message (wait for the message, deserialize it
            // and validate it). Process invalid messages and loop until
            // a valid message is received.
            bool receive (MessagePtr&, const AbortInterface*, ReportInterface&);

            // Invalid incoming messages processing
            bool getAutoErrorResponse() const {return _auto_error_response;}
            void setAutoErrorResponse(bool on) {_auto_error_response = on;}
            size_t getMaxInvalidMessages() const {return _max_invalid_msg;}
            void setMaxInvalidMessages(size_t n) {_max_invalid_msg = n;}

        protected:
            // Inherited from TCPConnection
            virtual void handleConnected(ReportInterface&);

        private:
            const Protocol* _protocol;
            bool            _auto_error_response;
            size_t          _max_invalid_msg;
            size_t          _invalid_msg_count;
            MUTEX           _send_mutex;
            MUTEX           _receive_mutex;

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
        };
    }
}

#include "tsTLVConnectionTemplate.h"
