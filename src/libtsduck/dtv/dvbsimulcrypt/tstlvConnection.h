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
//!
//!  @file
//!  TCP connection using TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPConnection.h"
#include "tstlvProtocol.h"
#include "tsMutex.h"
#include "tstlvMessage.h"
#include "tstlvLogger.h"

namespace ts {
    namespace tlv {
        //!
        //! TCP connection using TLV messages.
        //! @ingroup net
        //!
        //! @tparam MUTEX Mutex type for synchronization.
        //! Serialization & deserialization need synchronized access.
        //! By default, use thread-safe implementation.
        //! Instantiate with MUTEX = NullMutex for mono-thread appli.
        //!
        template <class MUTEX = Mutex>
        class Connection: public ts::TCPConnection
        {
            TS_NOBUILD_NOCOPY(Connection);
        public:
            //!
            //! Reference to superclass.
            //!
            typedef ts::TCPConnection SuperClass;

            //!
            //! Constructor.
            //! @param [in] protocol The incoming messages are interpreted
            //! according to this protocol.
            //! @param [in] auto_error_response When an invalid message is
            //! received, the corresponding error message is automatically
            //! sent back to the sender when @a auto_error_response is true.
            //! @param [in] max_invalid_msg When non-zero, the connection is
            //! automatically disconnected when the number of consecutive
            //! invalid messages has reached this value.
            //!
            explicit Connection(const Protocol* protocol, bool auto_error_response = true, size_t max_invalid_msg = 0);

            //!
            //! Serialize and send a TLV message.
            //! @param [in] msg The message to send.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool send(const Message& msg, Report& report);

            //!
            //! Serialize and send a TLV message.
            //! @param [in] msg The message to send.
            //! @param [in,out] logger Where to report errors and messages.
            //! @return True on success, false on error.
            //!
            bool send(const Message& msg, Logger& logger);

            //!
            //! Receive a TLV message.
            //! Wait for the message, deserialize it and validate it.
            //! Process invalid messages and loop until a valid message is received.
            //! @param [out] msg A safe pointer to the received message.
            //! @param [in] abort If non-zero, invoked when I/O is interrupted
            //! (in case of user-interrupt, return, otherwise retry).
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool receive(MessagePtr& msg, const AbortInterface* abort, Report& report);

            //!
            //! Receive a TLV message.
            //! Wait for the message, deserialize it and validate it.
            //! Process invalid messages and loop until a valid message is received.
            //! @param [out] msg A safe pointer to the received message.
            //! @param [in] abort If non-zero, invoked when I/O is interrupted
            //! (in case of user-interrupt, return, otherwise retry).
            //! @param [in,out] logger Where to report errors and messages.
            //! @return True on success, false on error.
            //!
            bool receive(MessagePtr& msg, const AbortInterface* abort, Logger& logger);

            //!
            //! Get invalid incoming messages processing.
            //! @return True if, when an invalid message is received, the corresponding
            //! error message is automatically sent back to the sender.
            //!
            bool getAutoErrorResponse() const {return _auto_error_response;}

            //!
            //! Set invalid incoming messages processing.
            //! @param [in] on When an invalid message is received, the corresponding
            //! error message is automatically sent back to the sender when @a on is true.
            //!
            void setAutoErrorResponse(bool on)
            {
                _auto_error_response = on;
            }

            //!
            //! Get invalid message threshold.
            //! @return When non-zero, the connection is automatically disconnected
            //! when the number of consecutive invalid messages has reached this value.
            //!
            size_t getMaxInvalidMessages() const {return _max_invalid_msg;}

            //!
            //! Set invalid message threshold.
            //! @param [in] n When non-zero, the connection is automatically disconnected
            //! when the number of consecutive invalid messages has reached this value.
            //!
            void setMaxInvalidMessages(size_t n) {_max_invalid_msg = n;}

        protected:
            // Inherited from TCPConnection
            virtual void handleConnected(Report&) override;

        private:
            const Protocol* _protocol;
            bool            _auto_error_response;
            size_t          _max_invalid_msg;
            size_t          _invalid_msg_count;
            MUTEX           _send_mutex;
            MUTEX           _receive_mutex;
        };
    }
}

#include "tstlvConnectionTemplate.h"
