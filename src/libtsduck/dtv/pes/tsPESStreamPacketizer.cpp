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

#include "tsPESStreamPacketizer.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESStreamPacketizer::PESStreamPacketizer(const DuckContext& duck, PID pid) :
    PESPacketizer(duck, pid, this),
    _max_queued(0),
    _pes_queue()
{
}

ts::PESStreamPacketizer::~PESStreamPacketizer()
{
}


//----------------------------------------------------------------------------
// Reset the content of a packetizer. Becomes empty.
//----------------------------------------------------------------------------

void ts::PESStreamPacketizer::reset()
{
    _pes_queue.clear();
    PESPacketizer::reset();
}


//----------------------------------------------------------------------------
// Implementation of PESProviderInterface
//----------------------------------------------------------------------------

void ts::PESStreamPacketizer::providePESPacket(PacketCounter counter, PESPacketPtr& pes)
{
    if (_pes_queue.empty()) {
        pes.clear();
    }
    else {
        pes = _pes_queue.front();
        _pes_queue.pop_front();
    }
}


//----------------------------------------------------------------------------
// Add a PES packet to packetize.
//----------------------------------------------------------------------------

bool ts::PESStreamPacketizer::addPES(const PESPacketPtr& pes)
{
    if (_max_queued != 0 && _pes_queue.size() >= _max_queued) {
        return false;
    }
    else {
        _pes_queue.push_back(pes);
        return true;
    }
}


bool ts::PESStreamPacketizer::addPES(const PESPacket& pes, ShareMode mode)
{
    if (_max_queued != 0 && _pes_queue.size() >= _max_queued) {
        return false;
    }
    else {
        _pes_queue.push_back(PESPacketPtr(new PESPacket(pes, mode)));
        return true;
    }
}


//----------------------------------------------------------------------------
// Display the internal state of the packetizer, mainly for debug
//----------------------------------------------------------------------------

std::ostream& ts::PESStreamPacketizer::display(std::ostream& strm) const
{
    return AbstractPacketizer::display(strm)
        << UString::Format(u"  Additional queued PES packets: %'d", {_pes_queue.size()}) << std::endl
        << UString::Format(u"  Enqueue limit: %'d", {_max_queued}) << std::endl;
}
