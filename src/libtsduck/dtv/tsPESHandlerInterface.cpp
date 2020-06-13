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

#include "tsPESHandlerInterface.h"
TSDUCK_SOURCE;

// Default implementation.

void ts::PESHandlerInterface::handlePESPacket(PESDemux& demux, const PESPacket& packet)
{
}

void ts::PESHandlerInterface::handleVideoStartCode(PESDemux& demux, const PESPacket& packet, uint8_t start_code, size_t offset, size_t size)
{
}

void ts::PESHandlerInterface::handleNewVideoAttributes(PESDemux& demux, const PESPacket& packet, const VideoAttributes& attr)
{
}

void ts::PESHandlerInterface::handleAVCAccessUnit(PESDemux& demux, const PESPacket& packet, uint8_t nal_unit_type, size_t offset, size_t size)
{
}

void ts::PESHandlerInterface::handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size)
{
}

void ts::PESHandlerInterface::handleNewAVCAttributes(PESDemux& demux, const PESPacket& packet, const AVCAttributes& attr)
{
}

void ts::PESHandlerInterface::handleNewAudioAttributes(PESDemux& demux, const PESPacket& packet, const AudioAttributes& attr)
{
}

void ts::PESHandlerInterface::handleNewAC3Attributes(PESDemux& demux, const PESPacket& packet, const AC3Attributes& attr)
{
}
