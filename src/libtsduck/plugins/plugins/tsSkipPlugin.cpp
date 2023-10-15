//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSkipPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"skip", ts::SkipPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SkipPlugin::SkipPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Skip leading TS packets of a stream", u"[options] count")
{
    option(u"", 0, UNSIGNED, 1, 1);
    help(u"", u"Number of leading packets to skip.");

    option(u"stuffing", 's');
    help(u"stuffing", u"Replace excluded leading packets with stuffing (null packets) instead of removing them.\n");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SkipPlugin::getOptions()
{
    getIntValue(_skip_count, u"");
    _use_stuffing = present(u"stuffing");
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SkipPlugin::processPacket(TSPacket&, TSPacketMetadata&)
{
    if (tsp->pluginPackets() >= _skip_count) {
        return TSP_OK;
    }
    else if (_use_stuffing) {
        return TSP_NULL;
    }
    else {
        return TSP_DROP;
    }
}
