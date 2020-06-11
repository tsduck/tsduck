//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsDebugPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_PROCESSOR_PLUGIN(u"debug", ts::DebugPlugin);


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::DebugPlugin::DebugPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Debug traces", u"[options]"),
    _tag()
{
    option(u"tag", 't', STRING);
    help(u"tag", u"'string'",
         u"Message tag to be displayed with each debug message. "
         u"Useful when the plugin is used several times in the same process.");
}


//----------------------------------------------------------------------------
// Get options methods
//----------------------------------------------------------------------------

bool ts::DebugPlugin::getOptions()
{
    _tag = value(u"tag");
    if (!_tag.empty()) {
        _tag += u": ";
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DebugPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    tsp->verbose(u"%sPID: 0x%0X, labels: %s, timestamp: %s", {
                 _tag, pkt.getPID(),
                 pkt_data.labelsString(),
                 pkt_data.inputTimeStampString()});
    return TSP_OK;
}
