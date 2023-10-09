//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPESOneShotPacketizer.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESOneShotPacketizer::PESOneShotPacketizer(const DuckContext& duck, PID pid) :
    PESStreamPacketizer(duck, pid)
{
}

ts::PESOneShotPacketizer::~PESOneShotPacketizer()
{
}


//----------------------------------------------------------------------------
// Get complete cycle as one list of packets
//----------------------------------------------------------------------------

void ts::PESOneShotPacketizer::getPackets(TSPacketVector& packets)
{
    packets.clear();
    while (!empty()) {
        packets.resize(packets.size() + 1);
        PESStreamPacketizer::getNextPacket(packets[packets.size() - 1]);
    }
}


//----------------------------------------------------------------------------
// Hidden methods
//----------------------------------------------------------------------------

bool ts::PESOneShotPacketizer::getNextPacket(TSPacket&)
{
    return false;
}
