//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPESStreamPacketizer.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESStreamPacketizer::PESStreamPacketizer(const DuckContext& duck, PID pid) :
    PESPacketizer(duck, pid, this)
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
