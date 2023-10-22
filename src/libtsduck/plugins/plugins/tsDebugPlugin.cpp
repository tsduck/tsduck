//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDebugPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"debug", ts::DebugPlugin);


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::DebugPlugin::DebugPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Debug traces", u"[options]")
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
    getIntValue(_exit_code, u"exit", EXIT_SUCCESS);
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
