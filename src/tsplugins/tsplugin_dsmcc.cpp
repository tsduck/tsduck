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
#include "tsDSMCCCarousel.h"
#include <filesystem>


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DSMCCPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(DSMCCPlugin);

    public:
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PID _pid = PID_NULL;
        UString _opt_out_dir {};
        DSMCCCarousel _carousel {duck};
        SectionDemux _demux {duck, this, nullptr};

        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Write a resolved BIOP File object to disk under _opt_out_dir.
        // Rejects names containing ".." or "." segments; creates intermediate directories.
        void extractFile(const UString& name, const BIOPFileMessage& file);
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

    _carousel.clear();
    _demux.reset();
    _demux.setTableHandler(this);

    _carousel.setModuleCompletedHandler([this](uint16_t module_id, const ByteBlock& payload) {
        verbose(u"Module complete: ID 0x%X (size: %d)", module_id, payload.size());
        namespace fs = std::filesystem;
        const fs::path dir = fs::path(_opt_out_dir.toUTF8()) / "modules";
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            error(u"Cannot create directory %s: %s",
                  UString::FromUTF8(dir.string()),
                  UString::FromUTF8(ec.message()));
            return;
        }
        const UString filename = UString::FromUTF8((dir / UString::Format(u"module_%04X.bin", module_id).toUTF8()).string());
        if (payload.saveToFile(filename, this)) {
            verbose(u"  -> Saved to %s", filename);
        }
    });

    _carousel.setObjectHandler([this](uint16_t module_id, const UString& name, const BIOPMessage& msg) {
        verbose(u"Module 0x%X BIOP object: name=\"%s\" kind=\"%s\"",
                module_id,
                name.empty() ? u"(unresolved)" : name,
                UString::FromUTF8(msg.kindTag()));

        if (!name.empty() && msg.kindTag() == BIOPObjectKind::FILE) {
            extractFile(name, static_cast<const BIOPFileMessage&>(msg));
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

    _carousel.flushPendingObjects();

    std::stringstream ss;
    _carousel.listModules(ss);
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
    switch (table.tableId()) {
        case TID_DSMCC_UNM: {
            DSMCCUserToNetworkMessage unm(duck, table);
            _carousel.feedUserToNetwork(unm);
            break;
        }
        case TID_DSMCC_DDM: {
            DSMCCDownloadDataMessage ddm(duck, table);
            _carousel.feedDownloadData(ddm);
            break;
        }
        default:
            break;
    }
}


//----------------------------------------------------------------------------
// Extract a resolved BIOP File to disk.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::extractFile(const UString& name, const BIOPFileMessage& file)
{
    namespace fs = std::filesystem;

    // The resolver returns paths starting with '/'. Split into segments and
    // reject path-traversal / current-directory tokens so a crafted carousel
    // can't escape _opt_out_dir.
    UStringVector parts;
    name.split(parts, u'/', true, true);  // trim, remove_empty

    fs::path rel;
    for (const auto& seg : parts) {
        if (seg == u".." || seg == u".") {
            warning(u"Refusing unsafe path: %s", name);
            return;
        }
        rel /= seg.toUTF8();
    }
    if (rel.empty()) {
        return;  // name was just "/" or all-empty segments
    }

    const fs::path full = fs::path(_opt_out_dir.toUTF8()) / "files" / rel;

    std::error_code ec;
    fs::create_directories(full.parent_path(), ec);
    if (ec) {
        error(u"Cannot create directory %s: %s",
              UString::FromUTF8(full.parent_path().string()),
              UString::FromUTF8(ec.message()));
        return;
    }

    const UString out_path = UString::FromUTF8(full.string());
    if (file.content.saveToFile(out_path, this)) {
        verbose(u"Extracted %s (%d bytes)", out_path, file.content.size());
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DSMCCPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& /*pkt_data*/)
{
    _demux.feedPacket(pkt);
    return TSP_OK;
}
