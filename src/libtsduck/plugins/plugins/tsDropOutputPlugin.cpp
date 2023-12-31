//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDropOutputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_OUTPUT_PLUGIN(u"drop", ts::DropOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DropOutputPlugin::DropOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Drop output packets", u"[options]")
{
}


//----------------------------------------------------------------------------
// Output plugin methods
//----------------------------------------------------------------------------

bool ts::DropOutputPlugin::send(const TSPacket*, const TSPacketMetadata* pkt_data, size_t)
{
    return true;
}
