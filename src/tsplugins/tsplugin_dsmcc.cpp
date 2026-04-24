//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Extract DSM-CC Object Carousel content.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsDSMCCExtractor.h"
#include "tsDSMCCExtractorArgs.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DSMCCPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);

    public:
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        DSMCCExtractorArgs              _args {};
        std::unique_ptr<DSMCCExtractor> _extractor {};
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"dsmcc", ts::DSMCCPlugin);


ts::DSMCCPlugin::DSMCCPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract DSM-CC content", u"[options]")
{
    _args.defineArgs(*this);
}


bool ts::DSMCCPlugin::getOptions()
{
    return _args.loadArgs(*this);
}


bool ts::DSMCCPlugin::start()
{
    duck.loadArgs(*this);
    _extractor = std::make_unique<DSMCCExtractor>(duck, _args.options);
    _extractor->setPID(_args.pid);
    return true;
}


bool ts::DSMCCPlugin::stop()
{
    if (_extractor) {
        _extractor->flush();
        _extractor.reset();
    }
    return true;
}


ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& /*pkt_data*/)
{
    _extractor->feedPacket(pkt);
    return TSP_OK;
}
