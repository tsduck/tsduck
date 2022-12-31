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

#include "tsDebugPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"debug", ts::DebugPlugin);


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::DebugPlugin::DebugPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Debug traces", u"[options]"),
    _tag(),
    _null(nullptr),
    _segfault(false),
    _exit(false),
    _exit_code(0)
{
    setIntro(u"A number of debug actions are executed for each packet. "
             u"By default, a debug-level message is displayed for each packet. "
             u"Use --only-label to select packets to debug.");

    option(u"exit", 0, INT32);
    help(u"exit", u"Exit application with the specified integer code on the first debugged packet.");

    option(u"segfault");
    help(u"segfault", u"Simulate a segmentation fault on the first debugged packet.");

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
    _segfault = present(u"segfault");
    _exit = present(u"exit");
    getIntValue(_exit_code, u"exit");
    getValue(_tag, u"tag");
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
    if (_segfault) {
        *_null = 0;
    }
    if (_exit) {
        ::exit(_exit_code);
    }
    tsp->verbose(u"%sPID: 0x%0X, labels: %s, timestamp: %s, packets in plugin: %'d, in thread: %'d", {
                 _tag, pkt.getPID(),
                 pkt_data.labelsString(),
                 pkt_data.inputTimeStampString(),
                 tsp->pluginPackets(),
                 tsp->totalPacketsInThread()});
    return TSP_OK;
}
