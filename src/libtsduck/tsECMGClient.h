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
//!
//!  @file
//!  ECM generator client.
//!
//!  Use ECMG <=> SCS protocol to request EMC's.
//!  An ECMGClient object acts as an SCS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsECMGClientHandlerInterface.h"
#include "tsTLVConnection.h"
#include "tsMessageQueue.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"

namespace ts {

    // See DVB standard ETSI TS;103.197 V1.4.1 for ECMG <=> SCS protocol.
    //
    // Restriction: ECMG shall support only current/next control words in ECM,
    // meaning CW_per_msg = 2 and lead_CW = 1.

    class TSDUCKDLL ECMGClient: private Thread
    {
    public:
        // Constructor & destructor.
        // If asynchronous ECM notification is used, the amount of minimum stack
        // size for the execution of the handler can be specified.
        ECMGClient (size_t extra_handler_stack_size = 0);
        ~ECMGClient();

        // Connect to a remote ECMG. Perform all initial channel and stream negotiation.
        bool connect (const SocketAddress&,
                      uint32_t super_cas_id,
                      uint16_t ecm_channel_id,
                      uint16_t ecm_stream_id,
                      uint16_t ecm_id,
                      uint16_t nominal_cp_duration,  // unit: 100 ms, as specified by ECMG <=> SCS protocol
                      ecmgscs::ChannelStatus&,     // initial response to channel_setup
                      ecmgscs::StreamStatus&,      // initial response to stream_setup
                      const AbortInterface*,
                      ReportInterface*);

        // Synchronously generate an ECM.
        bool generateECM (uint16_t cp_number,        // current crypto-period number
                          const void* current_cw,  // 8-byte control word for current crypto-period
                          const void* next_cw,     // 8-byte control word for next crypto-period
                          const void* ac,          // access criteria, unspecified if zero
                          size_t ac_size,          // access criteria size in bytes
                          uint16_t cp_duration,      // unit: 100 ms, unspecified if zero
                          ecmgscs::ECMResponse&);  // returned ECM

        // Asynchronously generate an ECM. The notification of the ECM generation
        // or error is performed through the specified handler.
        bool submitECM (uint16_t cp_number,        // current crypto-period number
                        const void* current_cw,  // 8-byte control word for current crypto-period
                        const void* next_cw,     // 8-byte control word for next crypto-period
                        const void* ac,          // access criteria, unspecified if zero
                        size_t ac_size,          // access criteria size in bytes
                        uint16_t cp_duration,      // unit: 100 ms, unspecified if zero
                        ECMGClientHandlerInterface*);

        // Disconnect from remote ECMG. Close stream and channel.
        bool disconnect();

        // Check if the ECMG is connected
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
        ReportInterface*        _report;
        tlv::Connection <Mutex> _connection;     // connection with ECMG server
        ecmgscs::ChannelStatus  _channel_status; // initial response to channel_setup
        ecmgscs::StreamStatus   _stream_status;  // initial response to stream_setup
        Mutex                   _mutex;          // exclusive access to protected fields
        Condition               _work_to_do;     // notify receiver thread to do some work
        AsyncRequests           _async_requests;
        MessageQueue <tlv::Message, NullMutex> _response_queue;

        // Receiver thread main code
        virtual void main();

        // Report specified error message if not empty, abort connection and return false
        bool abortConnection (const std::string&);

        // Unreachable operations
        ECMGClient (const ECMGClient&);
        ECMGClient& operator= (const ECMGClient&);
    };
}
