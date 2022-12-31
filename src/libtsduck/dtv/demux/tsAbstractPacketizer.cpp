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

#include "tsAbstractPacketizer.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractPacketizer::AbstractPacketizer(const DuckContext& duck, PID pid) :
    _duck(duck),
    _pid(pid),
    _continuity(0),
    _packet_count(0)
{
}

ts::AbstractPacketizer::~AbstractPacketizer()
{
}


//----------------------------------------------------------------------------
// Reset the content of a packetizer. Becomes empty.
//----------------------------------------------------------------------------

void ts::AbstractPacketizer::reset()
{
    // Subclasses should do more....
}


//----------------------------------------------------------------------------
// Configure a TS packet with continuity and PID.
//----------------------------------------------------------------------------

void ts::AbstractPacketizer::configurePacket(TSPacket& pkt, bool nullify)
{
    if (nullify) {
        pkt = NullPacket;
    }
    else {
        pkt.setPID(_pid);
        pkt.setCC(_continuity);
        _continuity = (_continuity + 1) & 0x0F;
    }
    _packet_count++;
}


//----------------------------------------------------------------------------
// Display the internal state of the packetizer, mainly for debug
//----------------------------------------------------------------------------

std::ostream& ts::AbstractPacketizer::display(std::ostream& strm) const
{
    return strm
        << UString::Format(u"  PID: %d (0x%X)", {_pid, _pid}) << std::endl
        << "  Next CC: " << int(_continuity) << std::endl
        << UString::Format(u"  Output packets: %'d", {_packet_count}) << std::endl;
}
