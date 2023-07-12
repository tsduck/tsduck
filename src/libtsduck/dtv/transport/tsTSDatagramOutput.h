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
//!  Send TS packets over datagrams (UDP, SRT, RIST, etc.)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSDatagramOutputHandlerInterface.h"
#include "tsTSPacket.h"
#include "tsUDPSocket.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Options which alter the behavior of the output datagrams.
    //! Can be used as bitmasks.
    //!
    enum class TSDatagramOutputOptions {
        NONE         = 0x0000,  //!< No option.
        ALLOW_RTP    = 0x0001,  //!< Allow RTP options to build an RTP datagram.
        ALWAYS_BURST = 0x0002,  //!< Do not define option --enforce-burst, always enforce burst.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::TSDatagramOutputOptions);

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Send TS packets over datagrams (UDP, SRT, RIST, etc.)
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSDatagramOutput: private TSDatagramOutputHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TSDatagramOutput);
    public:
        //!
        //! Default number of TS packets in a UDP datagram.
        //! This value is equivalent to 1316 bytes, the maximum number of TS packets which fit
        //! (with headers) in an Ethernet MTU (1500 bytes).
        //!
        static constexpr size_t DEFAULT_PACKET_BURST = 7;

        //!
        //! Maximum number of TS packets in a UDP datagram.
        //! This value (approximately 24 kB) is not recommended since it will result in
        //! IP datagram fragmentation in most cases.
        //!
        static constexpr size_t MAX_PACKET_BURST = 128;

        //!
        //! Constructor.
        //! @param [in] flags List of options.
        //! @param [in] output Output handler for datagrams. If null, raw UDP output is used.
        //!
        explicit TSDatagramOutput(TSDatagramOutputOptions flags, TSDatagramOutputHandlerInterface* output = nullptr);

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Open and initialize the TS packet output.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(Report& report);

        //!
        //! Close the TS packet output.
        //! Flush pending packets, if any.
        //! @param [in] bitrate Current of last bitrate to compute timestamps for buffered packets. Ignored if zero.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(const BitRate& bitrate, Report& report);

        //!
        //! Send TS packets.
        //! Some of them can be buffered and sent later.
        //! @param [in] packets Address of first packet.
        //! @param [in] packet_count Number of packets to send.
        //! @param [in] bitrate Current bitrate to compute timestamps. Ignored if zero.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool send(const TSPacket* packets, size_t packet_count, const BitRate& bitrate, Report& report);

    private:
        // Configuration and command line options.
        TSDatagramOutputOptions           const _flags;    // Configuration flags.
        TSDatagramOutputHandlerInterface* const _output;   // Datagram output handler.
        bool                              const _raw_udp;  // Use raw UDP socket.

        // Common command line options.
        size_t            _pkt_burst;          // Number of TS packets per UDP message
        bool              _enforce_burst;      // Option --enforce-burst
        bool              _use_rtp;            // Use real-time transport protocol
        uint8_t           _rtp_pt;             // RTP payload type.
        bool              _rtp_fixed_sequence; // RTP sequence number starts with a fixed value
        uint16_t          _rtp_start_sequence; // RTP starting sequence number
        bool              _rtp_fixed_ssrc;     // RTP SSRC id has a fixed value
        uint32_t          _rtp_user_ssrc;      // RTP user-specified SSRC id
        PID               _pcr_user_pid;       // User-specified PCR PID.
        bool              _rs204_format;       // Use 204-byte format with Reed Solomon placeholder.

        // Command line options for raw UDP.
        IPv4SocketAddress _destination;        // Destination address/port.
        IPv4Address       _local_addr;         // Local address.
        uint16_t          _local_port;         // Local UDP source port.
        int               _ttl;                // Time to live option.
        int               _tos;                // Type of service option.
        bool              _mc_loopback;        // Multicast loopback option
        bool              _force_mc_local;     // Force multicast outgoing local interface

        // Working data.
        bool              _is_open;            // Currently in progress
        uint16_t          _rtp_sequence;       // RTP current sequence number
        uint32_t          _rtp_ssrc;           // RTP current SSRC id (constant during a session)
        PID               _pcr_pid;            // Current PCR PID.
        uint64_t          _last_pcr;           // Last PCR value in PCR PID
        uint64_t          _last_rtp_pcr;       // Last RTP timestamp in PCR units (in last datagram)
        PacketCounter     _last_rtp_pcr_pkt;   // Packet index of last datagram
        uint64_t          _rtp_pcr_offset;     // Value to substract from PCR to get RTP timestamp
        PacketCounter     _pkt_count;          // Total packet counter for output packets
        size_t            _out_count;          // Number of packets in _out_buffer
        TSPacketVector    _out_buffer;         // Buffered packets for output with --enforce-burst
        UDPSocket         _sock;               // Outgoing socket for raw UDP

        // Implementation of TSDatagramOutputHandlerInterface.
        // The object is its own handler in case of raw UDP output.
        virtual bool sendDatagram(const void* address, size_t size, Report& report) override;

        // Send contiguous packets in one single datagram.
        bool sendPackets(const TSPacket* packet, size_t count, const BitRate& bitrate, Report& report);
    };
}
