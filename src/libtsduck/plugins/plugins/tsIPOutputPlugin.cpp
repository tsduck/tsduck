//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPOutputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_OUTPUT_PLUGIN(u"ip", ts::IPOutputPlugin);


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPOutputPlugin::IPOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast", u"[options] address:port")
{
    _datagram.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Redirect all methods to _datagram.
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::isRealTime()
{
    return true;
}

bool ts::IPOutputPlugin::getOptions()
{
    return _datagram.loadArgs(duck, *this);
}

bool ts::IPOutputPlugin::start()
{
    return _datagram.open(*tsp);
}

bool ts::IPOutputPlugin::stop()
{
    return _datagram.close(tsp->bitrate(), *tsp);
}

bool ts::IPOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    return _datagram.send(packets, packet_count, tsp->bitrate(), *tsp);
}
