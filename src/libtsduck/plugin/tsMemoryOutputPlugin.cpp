//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsMemoryOutputPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"memory", ts::MemoryOutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::MemoryOutputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MemoryOutputPlugin::MemoryOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Direct memory output to an application", u"[options]"),
    _port(0),
    _handler(nullptr)
{
    setIntro(u"Developer plugin: This plugin is useful only to C++, Java or Python developers "
             u"who run a TSProcessor pipeline inside their applications and want this application "
             u"to directly interact with the output of the pipeline.");

    option(u"port", 'p', UINT16);
    help(u"port",
         u"A 'port number' for the memory communication with the application. "
         u"If there is only one instance of TSProcessor running in the application, "
         u"the default value (zero) is just fine.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::getOptions()
{
    getIntValue(_port, u"port");
    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::start()
{
    _handler = MemoryPluginProxy::Instance()->getOutputPushHandler(_port);
    tsp->debug(u"memory output plugin started on port %d in %s mode", {_port, _handler != nullptr ? u"push" : u"pull"});
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::stop()
{
    if (_handler == nullptr) {
        MemoryPluginProxy::Instance()->abortPullOutput(_port);
    }
    return true;
}


//----------------------------------------------------------------------------
// Send packets method.
//----------------------------------------------------------------------------

bool ts::MemoryOutputPlugin::send(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packet_count)
{
    if (_handler != nullptr) {
        // Push mode: call the application to push the packets.
        return _handler->pushPackets(this, packets, metadata, packet_count);
    }
    else {
        // Pull mode: wait for the application to pull everything from our buffer.
        return MemoryPluginProxy::Instance()->putPulledOutputPackets(_port, packets, metadata, packet_count);
    }
}
