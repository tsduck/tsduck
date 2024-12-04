//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPPacketPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"ip", ts::IPPacketPlugin);


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPPacketPlugin::IPPacketPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast, and pass them to next plugin", u"[options] address:port")
{
    _datagram.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Redirect all methods to _datagram.
//----------------------------------------------------------------------------

bool ts::IPPacketPlugin::isRealTime()
{
    return true;
}

bool ts::IPPacketPlugin::getOptions()
{
    return _datagram.loadArgs(duck, *this);
}

bool ts::IPPacketPlugin::start()
{
    return _datagram.open(*this);
}

bool ts::IPPacketPlugin::stop()
{
    return _datagram.close(tsp->bitrate(), false, *this);
}

ts::ProcessorPlugin::Status ts::IPPacketPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _datagram.send(&pkt, &pkt_data, 1, tsp->bitrate(), *this) ? TSP_OK : TSP_END;
}
