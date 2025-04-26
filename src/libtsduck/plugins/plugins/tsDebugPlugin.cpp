//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDebugPlugin.h"
#include "tsPluginRepository.h"
#include "tsEnvironment.h"

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

    option(u"bad-alloc");
    help(u"bad-alloc", u"Simulate a memory allocation failure on the first debugged packet.");

    option(u"environment-variable", 0, STRING);
    help(u"environment-variable", u"name", u"Monitor modification of the specified environment variable.");

    option(u"exception");
    help(u"exception", u"Throw an exception on the first debugged packet.");

    option(u"exit", 0, INT32);
    help(u"exit", u"Exit application with the specified integer code on the first debugged packet.");

    option(u"packet", 'p', UNSIGNED);
    help(u"packet", u"Index of the first debugged packet. Zero by default.");

    option(u"segfault");
    help(u"segfault", u"Simulate a segmentation fault on the first debugged packet.");

    option(u"tag", 't', STRING);
    help(u"tag", u"'string'",
         u"Message tag to be displayed with each debug message. "
         u"Useful when the plugin is used several times in the same process.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DebugPlugin::getOptions()
{
    _bad_alloc = present(u"bad-alloc");
    _segfault = present(u"segfault");
    _exception = present(u"exception");
    _exit = present(u"exit");
    getIntValue(_exit_code, u"exit", EXIT_SUCCESS);
    getIntValue(_packet, u"packet", 0);
    getValue(_env_name, u"environment-variable");
    getValue(_tag, u"tag");
    if (!_tag.empty()) {
        _tag += u": ";
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DebugPlugin::start()
{
    _env_value.clear();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DebugPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (tsp->pluginPackets() < _packet) {
        return TSP_OK; // not yet the first debugged packet
    }
    if (!_env_name.empty()) {
        const UString value(GetEnvironment(_env_name));
        if (value != _env_value) {
            info(u"%spacket %'d: %s=\"%s\" (was \"%s\")", _tag, tsp->pluginPackets(), _env_name, value, _env_value);
            _env_value = value;
        }
    }
    if (_exception) {
        throw std::exception();
    }
    if (_segfault) {
        *_null = 0;
    }
    if (_bad_alloc) {
        info(u"simulating a memory allocation failure");
        for (;;) {
            new std::vector<uint8_t>(1'000'000'000);
        }
    }
    if (_exit) {
        std::exit(_exit_code);
    }
    verbose(u"%sPID: 0x%0X, labels: %s, timestamp: %s, packets in plugin: %'d, in thread: %'d",
            _tag, pkt.getPID(),
            pkt_data.labelsString(),
            pkt_data.inputTimeStampString(),
            tsp->pluginPackets(),
            tsp->totalPacketsInThread());
    return TSP_OK;
}
