//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Extract DSM-CC Object Carousel content.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsDSMCCCarouselController.h"
#include "tsZlib.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DSMCCPlugin: public ProcessorPlugin, private TableHandlerInterface, private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);

    public:
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool _abort = false;  // Error (not found, etc).
        PID _pid = PID_NULL;  // Carousel PID.
        DSMCCCarouselController _controller {duck};
        SectionDemux _demux {duck, this, this};  // Section filter for Tables and Sections

        UString _opt_out_dir {};

        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };
}  // namespace ts

TS_REGISTER_PROCESSOR_PLUGIN(u"dsmcc", ts::DSMCCPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCPlugin::DSMCCPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract DSM-CC content", u"[options]")
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"Specifies the PID carrying DSM-CC Object Carousel. This is a required parameter.");

    option(u"output-directory", 'o', STRING);
    help(u"output-directory", u"Specify the directory where carousel files will be extracted. This is a required parameter.");
}

//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::getOptions()
{
    getIntValue(_pid, u"pid");
    verbose(u"get options pid: %n", _pid);

    getValue(_opt_out_dir, u"output-directory");
    verbose(u"get options output-directory: %s", _opt_out_dir);

    if (_pid == PID_NULL) {
        error(u"a PID must be specified using --pid");
        return false;
    }

    if (_opt_out_dir.empty()) {
        error(u"an output directory must be specified with --output-directory");
        return false;
    }
    return true;
}
//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::start()
{
    verbose(u"start");

    duck.loadArgs(*this);
    getIntValue(_pid, u"pid", PID_NULL);

    // Reinitialize the plugin state.
    _abort = false;
    _controller.clear();

    _demux.reset();
    _demux.setTableHandler(this);
    _demux.setSectionHandler(this);

    _controller.setModuleCompletedHandler([this](const DSMCCCarouselController::ModuleContext& ctx) {
        verbose(u"Module Complete: ID 0x%X (Size: %d)", ctx.module_id, ctx.payload.size());

        ByteBlock uncompressed;
        if (ctx.is_compressed) {
            if (Zlib::Decompress(uncompressed, ctx.payload)) {
                verbose(u"  -> Decompressed size: %d", uncompressed.size());
                // Additional check if original_size was provided in DII
                if (ctx.original_size > 0 && uncompressed.size() != ctx.original_size) {
                    warning(u"Module 0x%X: Decompressed size mismatch! Expected %d, got %d", ctx.module_id, ctx.original_size, uncompressed.size());
                }
            }
            else {
                error(u"Module 0x%X: Decompression failed!", ctx.module_id);
                uncompressed = ctx.payload;
            }
        }
        else {
            uncompressed = ctx.payload;
        }

        if (!_opt_out_dir.empty()) {
            UString filename = UString::Format(u"%s/module_%04X.bin", _opt_out_dir, ctx.module_id);
            if (uncompressed.saveToFile(filename)) {
                verbose(u"  -> Saved to %s", filename);
            }
            else {
                error(u"Error saving module to %s", filename);
            }
        }
    });

    if (_pid != PID_NULL) {
        verbose(u"_demux.addPID: %n", _pid);
        _demux.addPID(_pid);
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::stop()
{
    verbose(u"stop");

    std::stringstream ss;
    _controller.listModules(ss);
    UString status = UString::FromUTF8(ss.str());
    if (!status.empty()) {
        verbose(u"Final Module Status:\n%s", status);
    }
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    _controller.handleTable(demux, table);
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete section is available.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::handleSection(SectionDemux& demux, const Section& section)
{
    _controller.handleSection(demux, section);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return _abort ? TSP_END : TSP_OK;
}
