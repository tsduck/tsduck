//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsIPProtocols.h"
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
        size_t            _pkt_burst = DEFAULT_PACKET_BURST; // Number of TS packets per UDP message
        bool              _enforce_burst = false;      // Option --enforce-burst
        bool              _use_rtp = false;            // Use real-time transport protocol
        uint8_t           _rtp_pt = RTP_PT_MP2T;       // RTP payload type.
        bool              _rtp_fixed_sequence = false; // RTP sequence number starts with a fixed value
        uint16_t          _rtp_start_sequence = 0;     // RTP starting sequence number
        bool              _rtp_fixed_ssrc = false;     // RTP SSRC id has a fixed value
        uint32_t          _rtp_user_ssrc = 0;          // RTP user-specified SSRC id
        PID               _pcr_user_pid = PID_NULL;    // User-specified PCR PID.
        bool              _rs204_format = false;       // Use 204-byte format with Reed Solomon placeholder.

        // Command line options for raw UDP.
        IPv4SocketAddress _destination {};             // Destination address/port.
        IPv4Address       _local_addr {};              // Local address.
        uint16_t          _local_port = IPv4SocketAddress::AnyPort; // Local UDP source port.
        int               _ttl = 0;                    // Time to live option.
        int               _tos = -1;                   // Type of service option.
        bool              _mc_loopback = true;         // Multicast loopback option
        bool              _force_mc_local = false;     // Force multicast outgoing local interface
        size_t            _send_bufsize = 0;           // Socket send buffer size.

        // Working data.
        bool              _is_open = false;            // Currently in progress
        uint16_t          _rtp_sequence = 0;           // RTP current sequence number
        uint32_t          _rtp_ssrc = 0;               // RTP current SSRC id (constant during a session)
        PID               _pcr_pid = PID_NULL;         // Current PCR PID.
        uint64_t          _last_pcr = INVALID_PCR;     // Last PCR value in PCR PID
        uint64_t          _last_rtp_pcr = INVALID_PCR; // Last RTP timestamp in PCR units (in last datagram)
        PacketCounter     _last_rtp_pcr_pkt = 0;       // Packet index of last datagram
        uint64_t          _rtp_pcr_offset = 0;         // Value to substract from PCR to get RTP timestamp
        PacketCounter     _pkt_count = 0;              // Total packet counter for output packets
        size_t            _out_count = 0;              // Number of packets in _out_buffer
        TSPacketVector    _out_buffer {};              // Buffered packets for output with --enforce-burst
        UDPSocket         _sock {};                    // Outgoing socket for raw UDP

        // Implementation of TSDatagramOutputHandlerInterface.
        // The object is its own handler in case of raw UDP output.
        virtual bool sendDatagram(const void* address, size_t size, Report& report) override;

        // Send contiguous packets in one single datagram.
        bool sendPackets(const TSPacket* packet, size_t count, const BitRate& bitrate, Report& report);
    };
}
