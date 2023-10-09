//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
