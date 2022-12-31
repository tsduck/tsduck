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
//!  Abstract base class for output plugins sending real-time datagrams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"

namespace ts {
    //!
    //! Abstract base class for output plugins sending real-time datagrams.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractDatagramOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(AbstractDatagramOutputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

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

    protected:
        //!
        //! Options which alter the behavior of the output plugin.
        //! Can be used as bitmasks.
        //!
        enum Options {
            NONE      = 0x0000,  //!< No option.
            ALLOW_RTP = 0x0001,  //!< Allow RTP options to build an RTP datagram.
        };

        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] flags List of options.
        //!
        AbstractDatagramOutputPlugin(TSP* tsp, const UString& description, const UString& syntax, Options flags);

        //!
        //! Enable or disable the 204-byte format with placeholder for 16-byte Reed-Solomon trailer.
        //! @param [in] on RS204 mode to set.
        //!
        void setRS204Format(bool on) { _rs204_format = on; }

        //!
        //! Send a datagram message.
        //! Must be implemented by subclasses.
        //! @param [in] address Address of datagram.
        //! @param [in] size Size in bytes of datagram.
        //! @return True on success, false on error.
        //!
        virtual bool sendDatagram(const void* address, size_t size) = 0;

    private:
        // Configuration and command line options.
        const Options  _flags;              // Configuration flags.
        size_t         _pkt_burst;          // Number of TS packets per UDP message
        bool           _enforce_burst;      // Option --enforce-burst
        bool           _use_rtp;            // Use real-time transport protocol
        uint8_t        _rtp_pt;             // RTP payload type.
        bool           _rtp_fixed_sequence; // RTP sequence number starts with a fixed value
        uint16_t       _rtp_start_sequence; // RTP starting sequence number
        bool           _rtp_fixed_ssrc;     // RTP SSRC id has a fixed value
        uint32_t       _rtp_user_ssrc;      // RTP user-specified SSRC id
        PID            _pcr_user_pid;       // User-specified PCR PID.
        bool           _rs204_format;       // Use 204-byte format with Reed Solomon placeholder.

        // Working data.
        uint16_t       _rtp_sequence;       // RTP current sequence number
        uint32_t       _rtp_ssrc;           // RTP current SSRC id (constant during a session)
        PID            _pcr_pid;            // Current PCR PID.
        uint64_t       _last_pcr;           // Last PCR value in PCR PID
        uint64_t       _last_rtp_pcr;       // Last RTP timestamp in PCR units (in last datagram)
        PacketCounter  _last_rtp_pcr_pkt;   // Packet index of last datagram
        uint64_t       _rtp_pcr_offset;     // Value to substract from PCR to get RTP timestamp
        PacketCounter  _pkt_count;          // Total packet counter for output packets
        size_t         _out_count;          // Number of packets in _out_buffer
        TSPacketVector _out_buffer;         // Buffered packets for output with --enforce-burst

        // Send a buffer of TS packets.
        bool sendPackets(const TSPacket* packet, size_t count);
    };
}
