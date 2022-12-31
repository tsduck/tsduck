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
//!  ECM generator client.
//!
//!  Use ECMG <=> SCS protocol to request EMC's.
//!  An ECMGClient object acts as an SCS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsECMGClientArgs.h"
#include "tsECMGClientHandlerInterface.h"
#include "tstlvConnection.h"
#include "tsMessageQueue.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"

namespace ts {
    //!
    //! A DVB-ECMG client which acts as a DVB-SCS.
    //!
    //! Restriction: The target ECMG shall support only current or current/next control
    //! words in ECM, meaning CW_per_msg = 1 or 2 and lead_CW = 0 or 1.
    //!
    //! @see DVB standard ETSI TS 103.197 V1.4.1 for ECMG <=> SCS protocol.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ECMGClient: private Thread
    {
        TS_NOCOPY(ECMGClient);
    public:
        //!
        //! Constructor.
        //! @param [in] extra_handler_stack_size If asynchronous ECM notification is used,
        //! an internal thread is created. This parameter gives the minimum amount of stack
        //! size for the execution of the handler. Zero for defaults.
        //!
        ECMGClient(size_t extra_handler_stack_size = 0);

        //!
        //! Destructor.
        //!
        virtual ~ECMGClient() override;

        //!
        //! Connect to a remote ECMG.
        //! Perform all initial channel and stream negotiation.
        //!
        //! @param [in] args Set of ECMG parameters.
        //! @param [out] channel_status Initial response to channel_setup
        //! @param [out] stream_status Initial response to stream_setup
        //! @param [in] abort An interface to check if the application is interrupted.
        //! @param [in] logger Where to report errors and messages.
        //! @return True on success, false on error.
        //!
        bool connect(const ECMGClientArgs& args,
                     ecmgscs::ChannelStatus& channel_status,
                     ecmgscs::StreamStatus& stream_status,
                     const AbortInterface* abort,
                     const tlv::Logger& logger);

        //!
        //! Synchronously generate an ECM.
        //!
        //! @param [in] cp_number Current crypto-period number.
        //! @param [in] current_cw Control word for current crypto-period.
        //! @param [in] next_cw Control word for next crypto-period.
        //! If empty, the ECMG must work with CW_per_msg = 1.
        //! @param [in] ac Access criteria, can be empty.
        //! @param [in] cp_duration Crypto-period in 100 ms units, unspecified if zero.
        //! @param [out] response Returned ECM.
        //! @return True on success, false on error.
        //!
        bool generateECM(uint16_t cp_number,
                         const ByteBlock& current_cw,
                         const ByteBlock& next_cw,
                         const ByteBlock& ac,
                         uint16_t cp_duration,
                         ecmgscs::ECMResponse& response);

        //!
        //! Asynchronously generate an ECM.
        //! Submit the ECM request and return immediately.
        //! The notification of the ECM generation or error is performed through the specified handler.
        //!
        //! @param [in] cp_number Current crypto-period number.
        //! @param [in] current_cw Control word for current crypto-period.
        //! @param [in] next_cw Control word for next crypto-period.
        //! If empty, the ECMG must work with CW_per_msg = 1.
        //! @param [in] ac Access criteria, can be empty.
        //! @param [in] cp_duration Crypto-period in 100 ms units, unspecified if zero.
        //! @param [in] handler Object which will be notified of the returned ECM.
        //! @return True on success, false on error.
        //!
        bool submitECM(uint16_t cp_number,
                       const ByteBlock& current_cw,
                       const ByteBlock& next_cw,
                       const ByteBlock& ac,
                       uint16_t cp_duration,
                       ECMGClientHandlerInterface* handler);

        //!
        //! Disconnect from remote ECMG.
        //! Close stream and channel.
        //! @return True on success, false on error.
        //!
        bool disconnect();

        //!
        //! Check if the ECMG is connected.
        //! @return True if the ECMG is connected.
        //!
        bool isConnected() const {return _state == CONNECTED;}

    private:
        // State of the client connection
        enum State {
            INITIAL,         // initial state, receiver thread not started
            DISCONNECTED,    // no TCP connection
            CONNECTING,      // opening channel and stream
            CONNECTED,       // stream established
            DISCONNECTING,   // closing stream and channel
            DESTRUCTING,     // object destruction in progress
        };

        // Stack size for execution of the receiver thread
        static const size_t RECEIVER_STACK_SIZE = 128 * 1024;

        // Maximum number of messages in response queue
        static const size_t RESPONSE_QUEUE_SIZE = 10;

        // Timeout for responses from ECMG (except ECM generation)
        static const MilliSecond RESPONSE_TIMEOUT = 5000;

        // List of asynchronous ECM requests: key=cp_number, value=handler
        typedef std::map <uint16_t, ECMGClientHandlerInterface*> AsyncRequests;

        // Private members
        State                   _state;
        const AbortInterface*   _abort;
        tlv::Logger             _logger;
        tlv::Connection <Mutex> _connection;     // connection with ECMG server
        ecmgscs::ChannelStatus  _channel_status; // initial response to channel_setup
        ecmgscs::StreamStatus   _stream_status;  // initial response to stream_setup
        Mutex                   _mutex;          // exclusive access to protected fields
        Condition               _work_to_do;     // notify receiver thread to do some work
        AsyncRequests           _async_requests;
        MessageQueue <tlv::Message, NullMutex> _response_queue;

        // Build a CW_provision message.
        void buildCWProvision(ecmgscs::CWProvision& msg,
                              uint16_t cp_number,
                              const ByteBlock& current_cw,
                              const ByteBlock& next_cw,
                              const ByteBlock& ac,
                              uint16_t cp_duration);

        // Receiver thread main code
        virtual void main() override;

        // Report specified error message if not empty, abort connection and return false
        bool abortConnection(const UString& = UString());
    };
}
