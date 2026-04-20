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
        bool _opt_list = false;
        bool _opt_dump_modules = false;

        struct FileEntry {
            UString path {};
            size_t  size = 0;
        };
        std::vector<FileEntry> _files {};

        DSMCCCarousel _carousel {duck};
        SectionDemux _demux {duck, this, nullptr};

        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Write a resolved BIOP File object to disk under _opt_out_dir.
        // Rejects names containing ".." or "." segments; creates intermediate directories.
        void extractFile(const UString& name, const BIOPFileMessage& file);

        // Print the collected file tree, module table and summary stats to stdout via info().
        void printListSummary();
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
    help(u"output-directory", u"Directory where carousel files will be extracted. "
                              u"Required unless --list is set.");

    option(u"list", 'l');
    help(u"list", u"List-only mode: print the carousel tree, module table and statistics "
                  u"without writing any files. --output-directory is not required.");

    option(u"dump-modules");
    help(u"dump-modules", u"Also write raw assembled module payloads to "
                          u"<output-directory>/modules/. Ignored with --list.");
}

//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::DSMCCPlugin::getOptions()
{
    getIntValue(_pid, u"pid");
    getValue(_opt_out_dir, u"output-directory");
    _opt_list = present(u"list");
    _opt_dump_modules = present(u"dump-modules");

    verbose(u"get options pid: %n", _pid);
    verbose(u"get options output-directory: %s", _opt_out_dir);
    verbose(u"get options list: %s, dump-modules: %s", _opt_list, _opt_dump_modules);

    if (_pid == PID_NULL) {
        error(u"a PID must be specified using --pid");
        return false;
    }

    if (!_opt_list && _opt_out_dir.empty()) {
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
    verbose(u"start");

    duck.loadArgs(*this);
    getIntValue(_pid, u"pid", PID_NULL);

    _carousel.clear();
    _files.clear();
    _demux.reset();
    _demux.setTableHandler(this);

    _carousel.setModuleCompletedHandler([this](uint16_t module_id, const ByteBlock& payload) {
        verbose(u"Module complete: ID 0x%X (size: %d)", module_id, payload.size());
        if (_opt_list || !_opt_dump_modules) {
            return;
        }
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

        if (name.empty() || msg.kindTag() != BIOPObjectKind::FILE) {
            return;
        }
        const auto& file = static_cast<const BIOPFileMessage&>(msg);
        _files.push_back({name, file.content.size()});
        if (!_opt_list) {
            extractFile(name, file);
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

    if (_opt_list) {
        printListSummary();
        return true;
    }

    std::stringstream ss;
    _carousel.listModules(ss);
    UString status = UString::FromUTF8(ss.str());
    if (!status.empty()) {
        verbose(u"Final Module Status:\n%s", status);
    }
    return true;
}


//----------------------------------------------------------------------------
// Print the collected file tree, module table and summary stats.
//----------------------------------------------------------------------------

void ts::DSMCCPlugin::printListSummary()
{
    std::vector<FileEntry> sorted = _files;
    std::sort(sorted.begin(), sorted.end(),
              [](const FileEntry& a, const FileEntry& b) { return a.path < b.path; });

    size_t total_bytes = 0;
    for (const auto& f : sorted) {
        total_bytes += f.size;
    }

    std::stringstream ss;
    ss << "Carousel tree:" << std::endl;
    if (sorted.empty()) {
        ss << "  (no resolved objects)" << std::endl;
    }
    else {
        ss << "/" << std::endl;
        UStringVector prev_dirs;
        for (const auto& f : sorted) {
            UStringVector segs;
            f.path.split(segs, u'/', true, true);
            if (segs.empty()) {
                continue;
            }
            UStringVector dirs(segs.begin(), segs.end() - 1);
            size_t common = 0;
            while (common < dirs.size() && common < prev_dirs.size() && dirs[common] == prev_dirs[common]) {
                ++common;
            }
            for (size_t i = common; i < dirs.size(); ++i) {
                ss << std::string(2 * (i + 1), ' ') << dirs[i].toUTF8() << "/" << std::endl;
            }
            ss << std::string(2 * (dirs.size() + 1), ' ')
               << segs.back().toUTF8()
               << UString::Format(u"  (%d bytes)", f.size).toUTF8()
               << std::endl;
            prev_dirs = dirs;
        }
    }

    ss << std::endl << "Modules:" << std::endl;
    _carousel.listModules(ss);

    ss << std::endl
       << UString::Format(u"Statistics: %d file(s), %d byte(s) total",
                          sorted.size(), total_bytes).toUTF8()
       << std::endl;

    info(u"%s", UString::FromUTF8(ss.str()));
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
