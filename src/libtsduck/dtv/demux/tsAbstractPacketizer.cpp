//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractPacketizer.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractPacketizer::AbstractPacketizer(const DuckContext& duck, PID pid) :
    _duck(duck),
    _pid(pid){
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
