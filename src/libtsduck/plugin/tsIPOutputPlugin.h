//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  IP output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsUDPSocket.h"

namespace ts {
    //!
    //! IP output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL IPOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(IPOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        IPOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

        //! @cond nodoxygen
        // A dummy storage value to force inclusion of this module when using the static library.
        static const int REFERENCE;
        //! @endcond

    private:
        UString        _destination;        // Destination address/port.
        UString        _local_addr;         // Local address.
        uint16_t       _local_port;         // Local UDP source port.
        int            _ttl;                // Time to live option.
        int            _tos;                // Type of service option.
        size_t         _pkt_burst;          // Number of TS packets per UDP message
        bool           _enforce_burst;      // Option --enforce-burst
        bool           _use_rtp;            // Use real-time transport protocol
        uint8_t        _rtp_pt;             // RTP payload type.
        bool           _rtp_fixed_sequence; // RTP sequence number starts with a fixed value
        uint16_t       _rtp_start_sequence; // RTP starting sequence number
        uint16_t       _rtp_sequence;       // RTP current sequence number
        bool           _rtp_fixed_ssrc;     // RTP SSRC id has a fixed value
        uint32_t       _rtp_user_ssrc;      // RTP user-specified SSRC id
        uint32_t       _rtp_ssrc;           // RTP current SSRC id (constant during a session)
        PID            _pcr_user_pid;       // User-specified PCR PID.
        PID            _pcr_pid;            // Current PCR PID.
        uint64_t       _last_pcr;           // Last PCR value in PCR PID
        uint64_t       _last_rtp_pcr;       // Last RTP timestamp in PCR units (in last datagram)
        PacketCounter  _last_rtp_pcr_pkt;   // Packet index of last datagram
        uint64_t       _rtp_pcr_offset;     // Value to substract from PCR to get RTP timestamp
        PacketCounter  _pkt_count;          // Total packet counter for output packets
        UDPSocket      _sock;               // Outgoing socket
        size_t         _out_count;          // Number of packets in _out_buffer
        TSPacketVector _out_buffer;         // Buffered packets for output with --enforce-burst

        // Send contiguous packets in one single datagram.
        bool sendDatagram(const TSPacket* pkt, size_t packet_count);
    };
}
