//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSValve.h"


//----------------------------------------------------------------------------
// Reset the transmission.
//----------------------------------------------------------------------------

void ts::TSValve::reset(const TSValveArgs& args, PacketProcessStatus initial, PacketProcessStatus first)
{

}


//----------------------------------------------------------------------------
// Change the transmission state.
//----------------------------------------------------------------------------

void ts::TSValve::change(PacketProcessStatus new_status)
{

}


//----------------------------------------------------------------------------
// Process one TS packet.
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::TSValve::processPacket(const TSPacket& pkt)
{

    return ts::TSP_OK; //@@@@
}
