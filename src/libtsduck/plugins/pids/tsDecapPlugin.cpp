//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDecapPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"decap", ts::DecapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DecapPlugin::DecapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Decapsulate TS packets from a PID produced by encap plugin", u"[options]")
{
    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore errors such malformed encapsulated stream.");

    option(u"mute-errors", 'm');
    help(u"mute-errors",
         u"Same as --ignore-errors and also don't even display the error message.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the input PID containing all encapsulated PID's. "
         u"This is a mandatory parameter, there is no default.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DecapPlugin::getOptions()
{
    _mute_errors = present(u"mute-errors");
    _ignore_errors = _mute_errors || present(u"ignore-errors");
    getIntValue(_pid, u"pid", PID_NULL);
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DecapPlugin::start()
{
    _decap.reset(_pid);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::DecapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_decap.processPacket(pkt) || _ignore_errors || !_decap.hasError()) {
        if (_decap.hasError()) {
            if (!_mute_errors) {
                error(_decap.lastError());
            }
            _decap.resetError();
        }
        return TSP_OK;
    }
    else {
        error(_decap.lastError());
        return TSP_END;
    }
}
