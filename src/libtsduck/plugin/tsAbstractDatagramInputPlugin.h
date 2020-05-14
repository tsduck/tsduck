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
//!  Abstract base class for input plugins receiving real-time datagrams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Abstract base class for input plugins receiving real-time datagrams.
    //! The input bitrate is computed from the received bytes and wall-clock time.
    //! TS packets are located in each received datagram, skipping potential headers.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractDatagramInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(AbstractDatagramInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] buffer_size Size in bytes of input buffer.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! Must be large enough to contain the largest datagram.
        //!
        AbstractDatagramInputPlugin(TSP* tsp, size_t buffer_size, const UString& description = UString(), const UString& syntax = UString());

        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;

    protected:
        //!
        //! Receive a datagram message.
        //! Must be implemented by subclasses.
        //! @param [out] buffer Address of the buffer for the received message.
        //! @param [in] buffer_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a buffer_size.
        //! @return True on success, false on error.
        //!
        virtual bool receiveDatagram(void* buffer, size_t buffer_size, size_t& ret_size) = 0;

    private:
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
        ByteBlock     _inbuf;              // Input buffer
    };
}
