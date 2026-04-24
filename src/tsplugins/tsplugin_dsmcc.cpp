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
        PID _pid = PID_NULL;
        DSMCCExtractor::Options _ext_opts {};
        std::unique_ptr<DSMCCExtractor> _extractor {};
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"dsmcc", ts::DSMCCPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCPlugin::DSMCCPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract DSM-CC content", u"[options]")
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"PID carrying the DSM-CC carousel (DSI/DII/DDB sections). Required.");

    option(u"output-directory", 'o', STRING);
    help(u"output-directory", u"Directory where carousel files will be extracted. "
                              u"Required unless --list is set.");

    option(u"list", 'l');
    help(u"list", u"List-only mode: print the carousel tree, module table and statistics "
                  u"without writing any files. --output-directory is not required.");

    option(u"dump-modules");
    help(u"dump-modules", u"Also write raw assembled module payloads to "
                          u"<output-directory>/modules/. Mutually exclusive "
                          u"with --list and with --data-carousel.");

    option(u"data-carousel");
    help(u"data-carousel", u"Treat the PID as a plain data carousel (e.g. DVB-SSU) "
                           u"instead of an object carousel: skip BIOP parsing and "
                           u"write each completed module directly as "
                           u"<output-directory>/module_XXXX.bin. Mutually exclusive "
                           u"with --dump-modules.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::getOptions()
{
    getIntValue(_pid, u"pid");
    getValue(_ext_opts.out_dir, u"output-directory");
    _ext_opts.list_mode = present(u"list");
    _ext_opts.dump_modules = present(u"dump-modules");
    _ext_opts.data_carousel = present(u"data-carousel");

    if (_pid == PID_NULL) {
        error(u"a PID must be specified using --pid");
        return false;
    }

    if (_ext_opts.data_carousel && _ext_opts.dump_modules) {
        error(u"--data-carousel and --dump-modules are mutually exclusive");
        return false;
    }

    if (_ext_opts.list_mode && _ext_opts.dump_modules) {
        error(u"--list and --dump-modules are mutually exclusive");
        return false;
    }

    if (!_ext_opts.list_mode && _ext_opts.out_dir.empty()) {
        error(u"an output directory must be specified with --output-directory (or use --list)");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::start()
{
    duck.loadArgs(*this);
    _extractor = std::make_unique<DSMCCExtractor>(duck, _ext_opts);
    _extractor->setPID(_pid);
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::stop()
{
    if (_extractor) {
        _extractor->flush();
        _extractor.reset();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& /*pkt_data*/)
{
    _extractor->feedPacket(pkt);
    return TSP_OK;
}
