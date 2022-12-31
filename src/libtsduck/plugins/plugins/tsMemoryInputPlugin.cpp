//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
    InputPlugin(tsp_, u"Direct memory input from an application", u"[options]"),
    _event_code(0)
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
