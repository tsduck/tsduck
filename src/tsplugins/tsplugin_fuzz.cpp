//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Transport stream fuzzing (random corruption).
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSFuzzing.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class FuzzPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FuzzPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSFuzzingArgs _fuzz_opt {};
        TSFuzzing _fuzzer {duck};
    };
}

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

ts::ProcessorPlugin::Status ts::FuzzPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _fuzzer.processPacket(pkt) ? TSP_OK : TSP_END;
}
