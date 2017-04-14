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
//
//  This abstract interface must be implemented by classes which need to be
//  notified of PES packets using a PESDemux. All hooks are optional, ie.
//  they have an empty default implementation.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPESPacket.h"
#include "tsAudioAttributes.h"
#include "tsVideoAttributes.h"
#include "tsAVCAttributes.h"
#include "tsAC3Attributes.h"

namespace ts {

    class PESDemux;

    class TSDUCKDLL PESHandlerInterface
    {
    public:
        // This hook is invoked when a complete PES packet is available.
        virtual void handlePESPacket (PESDemux&, const PESPacket&) {}

        // This hook is invoked when a video start code is encountered.
        // The specified offset points to the start code (00 00 01 xx)
        // in the PES packet payload. The specified size points to the next
        // start code.
        virtual void handleVideoStartCode (PESDemux&, const PESPacket&, uint8_t start_code, size_t offset, size_t size) {}

        // This hook is invoked when new video attributes are found in a video PID
        virtual void handleNewVideoAttributes (PESDemux&, const PESPacket&, const VideoAttributes&) {}

        // This hook is invoked when an AVC (ISO 14496-10, ITU H.264) access unit (aka "NALunit") is found
        virtual void handleAVCAccessUnit (PESDemux&, const PESPacket&, uint8_t nal_unit_type, size_t offset, size_t size) {}

        // This hook is invoked when new AVC attributes are found in a video PID
        virtual void handleNewAVCAttributes (PESDemux&, const PESPacket&, const AVCAttributes&) {}

        // This hook is invoked when new audio attributes are found in an audio PID
        virtual void handleNewAudioAttributes (PESDemux&, const PESPacket&, const AudioAttributes&) {}

        // This hook is invoked when new AC-3 attributes are found in an audio PID
        virtual void handleNewAC3Attributes (PESDemux&, const PESPacket&, const AC3Attributes&) {}

        // Virtual destructor
        virtual ~PESHandlerInterface () {}
    };
}
