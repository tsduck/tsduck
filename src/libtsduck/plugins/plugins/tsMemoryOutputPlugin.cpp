//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMemoryOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsPluginEventData.h"

TS_REGISTER_OUTPUT_PLUGIN(u"memory", ts::MemoryOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MemoryOutputPlugin::MemoryOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Direct memory output to an application", u"[options]")
{
    setIntro(u"Developer plugin: This plugin is useful only to C++, Java or Python developers "
             u"who run a TSProcessor pipeline inside their applications and want this application "
             u"to directly interact with the output of the pipeline.");

    option(u"event-code", 'e', UINT32);
    help(u"event-code",
         u"Signal a plugin event with the specified code each time the plugin output packets. "
         u"The event data is an instance of PluginEventData pointing to the output packets. "
         u"If an event handler sets the error indicator in the event data, the transmission is aborted.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::getOptions()
{
    getIntValue(_event_code, u"event-code");
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    // Prepare an event data block pointing to the output packets.
    PluginEventData data(packets->b, PKT_SIZE * packet_count);
    tsp->signalPluginEvent(_event_code, &data);
    return !data.hasError();
}
