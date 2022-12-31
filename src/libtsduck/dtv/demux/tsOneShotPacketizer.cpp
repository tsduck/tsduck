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

#include "tsOneShotPacketizer.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::OneShotPacketizer::OneShotPacketizer(const DuckContext& duck, PID pid, bool do_stuffing, const BitRate& bitrate) :
    CyclingPacketizer(duck, pid, do_stuffing ? StuffingPolicy::ALWAYS : StuffingPolicy::AT_END, bitrate)
{
}

ts::OneShotPacketizer::~OneShotPacketizer()
{
}


//----------------------------------------------------------------------------
// Get complete cycle as one list of packets
//----------------------------------------------------------------------------

void ts::OneShotPacketizer::getPackets(TSPacketVector& packets)
{
    packets.clear();

    if (storedSectionCount() > 0) {
        do {
            packets.resize(packets.size() + 1);
            CyclingPacketizer::getNextPacket(packets[packets.size() - 1]);
        } while (!atCycleBoundary());
    }
}


//----------------------------------------------------------------------------
// Hidden methods
//----------------------------------------------------------------------------

bool ts::OneShotPacketizer::getNextPacket(TSPacket&)
{
    return false;
}
