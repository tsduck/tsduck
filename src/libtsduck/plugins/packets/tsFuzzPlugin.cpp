//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFuzzPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"fuzz", ts::FuzzPlugin);


//----------------------------------------------------------------------------
// All methods are redirected to the TSFuzzing class.
//----------------------------------------------------------------------------

ts::FuzzPlugin::FuzzPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Introduce random errors in the transport stream", u"[options]")
{
    _fuzz_opt.defineArgs(*this);
}

bool ts::FuzzPlugin::getOptions()
{
    return _fuzz_opt.loadArgs(duck, *this);
}

bool ts::FuzzPlugin::start()
{
    return _fuzzer.start(_fuzz_opt);
}

ts::PacketProcessStatus ts::FuzzPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _fuzzer.processPacket(pkt) ? TSP_OK : TSP_END;
}
