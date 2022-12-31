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
//!  EMM generator client.
//!
//!  Use EMMG/PDG <=> MUX protocol to inject data.
//!  An EMMGClient object acts as an EMMG/PDG.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEMMGMUX.h"
#include "tstlvConnection.h"
#include "tsUDPSocket.h"
#include "tsTablesPtr.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"

namespace ts {

    class DuckContext;

    //!
    //! A DVB-EMMG client which connects to any MUX to inject data.
    //!
    //! Restriction: Only the TCP version of the EMMG/PDG <=> MUX protocol
    //! is supported here. The UDP version is currently unsupported.
    //!
    //! @see DVB standard ETSI TS 103.197 V1.4.1 for EMMG/PDG <=> MUX protocol.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL EMMGClient: private Thread
    {
        TS_NOBUILD_NOCOPY(EMMGClient);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //!
        EMMGClient(const DuckContext& duck);

        //!
        //! Destructor.
        //!
        virtual ~EMMGClient() override;

        //!
        //! Connect to a remote MUX.
        //! Perform all initial channel and stream negotiation.
        //!
        //! @param [in] mux IP address and TCP port of the MUX.
        //! @param [in] udp If port is specified, then send data_rovision messages using UDP
        //! instead of TCP. If the IP address is not specified, use the same one as @a mux.
        //! @param [in] client_id Client id, see EMMG/PDG <=> MUX protocol.
        //! @param [in] data_channel_id Data_channel_id, see EMMG/PDG <=> MUX protocol.
        //! @param [in] data_stream_id Data_stream_id, see EMMG/PDG <=> MUX protocol.
        //! @param [in] data_id Data_id, see EMMG/PDG <=> MUX protocol.
        //! @param [in] data_type Data_type, see EMMG/PDG <=> MUX protocol.
        //! @param [in] section_format If true, send data in section format.
        //! If false, send data in TS packet format.
        //! @param [out] channel_status Initial response to channel_setup
        //! @param [out] stream_status Initial response to stream_setup
        //! @param [in] abort An interface to check if the application is interrupted.
        //! @param [in] logger Where to report errors and messages.
        //! @return True on success, false on error.
        //!
        bool connect(const IPv4SocketAddress& mux,
                     const IPv4SocketAddress& udp,
                     uint32_t client_id,
                     uint16_t data_channel_id,
                     uint16_t data_stream_id,
                     uint16_t data_id,
                     uint8_t data_type,
                     bool section_format,
                     emmgmux::ChannelStatus& channel_status,
                     emmgmux::StreamStatus& stream_status,
                     const AbortInterface* abort,
                     const tlv::Logger& logger);

        //!
        //! Send a bandwidth request.
        //! @param [in] bandwidth Requested bandwidth in kbits/second.
        //! @param [in] synchronous If true, wait for the MUX to return either an error or a bandwidth allocation.
        //! @return True on success, false on error.
        //!
        bool requestBandwidth(uint16_t bandwidth, bool synchronous = false);

        //!
        //! Get the last allocated bandwidth as returned by the MUX.
        //! @return The last allocated bandwidth in kbits/second or zero if there was none.
        //!
        uint16_t allocatedBandwidth();

        //!
        //! Get the last error response.
        //! @param [out] error_status Error code.
        //! @param [out] error_information Error information.
        //!
        void getLastErrorResponse(std::vector<uint16_t>& error_status, std::vector<uint16_t>& error_information);

        //!
        //! Send data provision.
        //!
        //! @param [in] data Data to send.
        //! @return True on success, false on error.
        //!
        bool dataProvision(const ByteBlockPtr& data);

        //!
        //! Send data provision.
        //!
        //! @param [in] data Data to send in several chunks.
        //! @return True on success, false on error.
        //!
        bool dataProvision(const std::vector<ByteBlockPtr>& data);

        //!
        //! Send data provision.
        //!
        //! @param [in] data Address of data to send.
        //! @param [in] size Size in bytes of data to send.
        //! @return True on success, false on error.
        //!
        bool dataProvision(const void* data, size_t size);

        //!
        //! Send data provision in section format.
        //!
        //! @param [in] sections Sections to send.
        //! If @a section_format was false during connect(), the sections are packetized first.
        //! @return True on success, false on error.
        //!
        bool dataProvision(const SectionPtrVector& sections);

        //!
        //! Disconnect from remote MUX.
        //! Close stream and channel.
        //! @return True on success, false on error.
        //!
        bool disconnect();

        //!
        //! Check if the EMMG is connected.
        //! @return True if the EMMG is connected.
        //!
        bool isConnected() const { return _state == CONNECTED; }

        //!
        //! Get the total number of data bytes which were sent so far.
        //! @return The total number of data bytes which were sent so far.
        //!
        uint64_t totalBytes() const { return _total_bytes; }

        //!
        //! Reset the total number of data bytes which were sent so far.
        //!
        void resetTotalBytes() { _total_bytes = 0; }

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

        // Stack size for execution of the receiver thread.
        static const size_t RECEIVER_STACK_SIZE = 128 * 1024;

        // Timeout for responses from MUX.
        static const MilliSecond RESPONSE_TIMEOUT = 5000;

        // Private members
        const DuckContext&     _duck;
        volatile State         _state;
        IPv4SocketAddress      _udp_address;
        uint64_t               _total_bytes;
        const AbortInterface*  _abort;
        tlv::Logger            _logger;
        tlv::Connection<Mutex> _connection;     // connection with MUX server
        UDPSocket              _udp_socket;     // where to send data_provision if UDP is used
        emmgmux::ChannelStatus _channel_status; // automatic response to channel_test
        emmgmux::StreamStatus  _stream_status;  // automatic response to stream_test
        Mutex                  _mutex;          // exclusive access to protected fields
        Condition              _work_to_do;     // notify receiver thread to do some work
        Condition              _got_response;   // notify application thread that a response arrived
        tlv::TAG               _last_response;  // tag of last response message
        uint16_t               _allocated_bw;   // last allocated bandwidth
        std::vector<uint16_t>  _error_status;   // last error status
        std::vector<uint16_t>  _error_info;     // last error information

        // Receiver thread main code
        virtual void main() override;

        // Prepare and wait for response.
        void cleanupResponse();
        tlv::TAG waitResponse();

        // Report specified error message if not empty, abort connection and return false
        bool abortConnection(const UString& = UString());
    };
}
