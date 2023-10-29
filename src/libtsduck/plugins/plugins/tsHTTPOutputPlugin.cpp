//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHTTPOutputPlugin.h"
#include "tsPluginRepository.h"

// Currently disabled, until complete:
// TS_REGISTER_OUTPUT_PLUGIN(u"http", ts::HTTPOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HTTPOutputPlugin::HTTPOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Act as an HTTP server and send TS packets to the incoming client", u"[options]")
{
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::getOptions()
{
    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::start()
{
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::stop()
{
    return true;
}


//----------------------------------------------------------------------------
// Send packets.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    return true;
}
