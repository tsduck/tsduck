//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020, Anthony Delannoy
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
//!  SRT input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once

#if !defined(TS_NOSRT)

#include "tsPlugin.h"
#include "tsSRTSocket.h"
#include "tsTime.h"

namespace ts {
    //!
    //! SRT input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL SRTInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(SRTInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        SRTInputPlugin(TSP* tsp);

        // Implementation of plugin API.
        virtual bool getOptions(void) override;
        virtual bool start(void) override;
        virtual bool stop(void) override;
        virtual bool isRealTime(void) override { return true; }
        virtual BitRate getBitrate(void) override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput(void) override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

    private:
        SRTSocket     _sock;
        UString       _source;             // Source address/port
        MilliSecond   _eval_time;          // Bitrate evaluation interval in milli-seconds
        MilliSecond   _display_time;       // Bitrate display interval in milli-seconds
        Time          _next_display;       // Next bitrate display time
        Time          _start;              // UTC date of first received packet
        PacketCounter _packets;            // Number of received packets since _start
        Time          _start_0;            // Start of previous bitrate evaluation period
        PacketCounter _packets_0;          // Number of received packets since _start_0
        Time          _start_1;            // Start of previous bitrate evaluation period
        PacketCounter _packets_1;          // Number of received packets since _start_1
        size_t        _inbuf_count;        // Remaining TS packets in inbuf
        size_t        _inbuf_next;         // Index in inbuf of next TS packet to return
        uint8_t       _inbuf[IP_MAX_PACKET_SIZE]; // Input buffer
    };
}

#endif /* TS_NOSRT */
