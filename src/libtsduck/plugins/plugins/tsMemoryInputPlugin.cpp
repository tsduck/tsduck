//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMemoryInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsPluginEventData.h"

TS_REGISTER_INPUT_PLUGIN(u"memory", ts::MemoryInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MemoryInputPlugin::MemoryInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Direct memory input from an application", u"[options]")
{
    setIntro(u"Developer plugin: This plugin is useful only to C++, Java or Python developers "
             u"who run a TSProcessor pipeline inside their applications and want this application "
             u"to directly interact with the input of the pipeline.");

    option(u"event-code", 'e', UINT32);
    help(u"event-code",
         u"Signal a plugin event with the specified code each time the plugin needs input packets. "
         u"The event data is an instance of PluginEventData pointing to the input buffer. "
         u"The application shall handle the event, waiting for input packets as long as necessary. "
         u"Returning zero packet (or not handling the event) means end if input.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::MemoryInputPlugin::getOptions()
{
    getIntValue(_event_code, u"event-code");
    return true;
}


//----------------------------------------------------------------------------
// Receive packets method.
//----------------------------------------------------------------------------

size_t ts::MemoryInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets)
{
    // Prepare an event data block pointing to the input buffer.
    PluginEventData data(buffer->b, 0, PKT_SIZE * max_packets);
    tsp->signalPluginEvent(_event_code, &data);
    return data.size() / PKT_SIZE;
}
