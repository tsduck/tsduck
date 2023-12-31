//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_NOBUILD_NOCOPY(ECMGClient);
    public:
        //!
        //! Constructor.
        //! @param [in] protocol Instance of ECMG <=> SCS protocol to use.
        //! A reference to the protocol instance is kept inside the object.
        //! @param [in] extra_handler_stack_size If asynchronous ECM notification is used,
        //! an internal thread is created. This parameter gives the minimum amount of stack
        //! size for the execution of the handler. Zero for defaults.
        //!
        ECMGClient(const ecmgscs::Protocol& protocol, size_t extra_handler_stack_size = 0);

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
        static constexpr size_t RECEIVER_STACK_SIZE = 128 * 1024;

        // Maximum number of messages in response queue
        static constexpr size_t RESPONSE_QUEUE_SIZE = 10;

        // Timeout for responses from ECMG (except ECM generation)
        static constexpr cn::seconds RESPONSE_TIMEOUT = cn::seconds(5);

        // List of asynchronous ECM requests: key=cp_number, value=handler
        using AsyncRequests = std::map <uint16_t, ECMGClientHandlerInterface*>;

        // Private members
        const ecmgscs::Protocol&    _protocol;
        State                       _state = INITIAL;
        const AbortInterface*       _abort = nullptr;
        tlv::Logger                 _logger {};
        tlv::Connection<null_mutex> _connection {_protocol, true, 3}; // connection with ECMG server
        ecmgscs::ChannelStatus      _channel_status {_protocol};      // initial response to channel_setup
        ecmgscs::StreamStatus       _stream_status {_protocol};       // initial response to stream_setup
        std::recursive_mutex        _mutex {};                        // exclusive access to protected fields
        std::condition_variable_any _work_to_do {};                   // notify receiver thread to do some work
        AsyncRequests               _async_requests {};
        MessageQueue<tlv::Message, null_mutex> _response_queue {RESPONSE_QUEUE_SIZE};

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
