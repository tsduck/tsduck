//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCExtractor.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsBinaryTable.h"
#include <algorithm>
#include <filesystem>
#include <sstream>


ts::DSMCCExtractor::DSMCCExtractor(DuckContext& duck, const Options& options) :
    _duck(duck),
    _options(options),
    _carousel(duck),
    _demux(duck, this, nullptr)
{
    _carousel.setScanBIOP(!_options.data_carousel);

    _carousel.setModuleCompletedHandler([this](uint16_t module_id, const ByteBlock& payload) {
        onModuleCompleted(module_id, payload);
    });

    if (!_options.data_carousel) {
        _carousel.setObjectHandler([this](uint16_t module_id, const UString& name, const BIOPMessage& msg) {
            onObjectReady(module_id, name, msg);
        });
    }
}


void ts::DSMCCExtractor::setPID(PID pid)
{
    if (pid != PID_NULL) {
        _duck.report().verbose(u"DSMCC extractor: adding PID %n to demux", pid);
        _demux.addPID(pid);
    }
}


void ts::DSMCCExtractor::feedPacket(const TSPacket& pkt)
{
    _demux.feedPacket(pkt);
}


void ts::DSMCCExtractor::flush()
{
    _carousel.flushPendingObjects();

    if (_options.list_mode) {
        printListSummary();
        return;
    }

    std::stringstream ss;
    _carousel.listModules(ss);
    UString status = UString::FromUTF8(ss.str());
    if (!status.empty()) {
        _duck.report().verbose(u"Final Module Status:\n%s", status);
    }
}


void ts::DSMCCExtractor::handleTable(SectionDemux&, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_DSMCC_UNM: {
            DSMCCUserToNetworkMessage unm(_duck, table);
            _carousel.feedUserToNetwork(unm);
            break;
        }
        case TID_DSMCC_DDM: {
            DSMCCDownloadDataMessage ddm(_duck, table);
            _carousel.feedDownloadData(ddm);
            break;
        }
        default:
            break;
    }
}


void ts::DSMCCExtractor::onModuleCompleted(uint16_t module_id, const ByteBlock& payload)
{
    _duck.report().verbose(u"Module complete: ID 0x%X (size: %d)", module_id, payload.size());

    if (_options.list_mode) {
        return;
    }
    if (!_options.dump_modules && !_options.data_carousel) {
        return;
    }

    namespace fs = std::filesystem;
    // Data-carousel mode writes flat under the output directory; object-carousel
    // mode with dump_modules writes under a modules/ sibling of files/.
    const fs::path dir = _options.data_carousel
        ? fs::path(_options.out_dir.toUTF8())
        : fs::path(_options.out_dir.toUTF8()) / "modules";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        _duck.report().error(u"Cannot create directory %s: %s",
                             UString::FromUTF8(dir.string()),
                             UString::FromUTF8(ec.message()));
        return;
    }
    const UString filename = UString::FromUTF8((dir / UString::Format(u"module_%04X.bin", module_id).toUTF8()).string());
    if (payload.saveToFile(filename, &_duck.report())) {
        _duck.report().verbose(u"  -> Saved to %s", filename);
    }
}


void ts::DSMCCExtractor::onObjectReady(uint16_t module_id, const UString& name, const BIOPMessage& msg)
{
    _duck.report().verbose(u"Module 0x%X BIOP object: name=\"%s\" kind=\"%s\"",
                           module_id,
                           name.empty() ? u"(unresolved)" : name,
                           UString::FromUTF8(msg.kindTag()));

    if (name.empty() || msg.kindTag() != BIOPObjectKind::FILE) {
        return;
    }
    const auto& file = static_cast<const BIOPFileMessage&>(msg);
    _files.push_back({name, file.content.size()});
    if (!_options.list_mode) {
        extractFile(name, file);
    }
}


void ts::DSMCCExtractor::extractFile(const UString& name, const BIOPFileMessage& file)
{
    namespace fs = std::filesystem;

    // The resolver returns paths starting with '/'. Split into segments and
    // reject path-traversal / current-directory tokens so a crafted carousel
    // can't escape the output directory.
    UStringVector parts;
    name.split(parts, u'/', true, true);

    fs::path rel;
    for (const auto& seg : parts) {
        if (seg == u".." || seg == u".") {
            _duck.report().warning(u"Refusing unsafe path: %s", name);
            return;
        }
        rel /= seg.toUTF8();
    }
    if (rel.empty()) {
        return;
    }

    const fs::path full = fs::path(_options.out_dir.toUTF8()) / "files" / rel;

    std::error_code ec;
    fs::create_directories(full.parent_path(), ec);
    if (ec) {
        _duck.report().error(u"Cannot create directory %s: %s",
                             UString::FromUTF8(full.parent_path().string()),
                             UString::FromUTF8(ec.message()));
        return;
    }

    const UString out_path = UString::FromUTF8(full.string());
    if (file.content.saveToFile(out_path, &_duck.report())) {
        _duck.report().verbose(u"Extracted %s (%d bytes)", out_path, file.content.size());
    }
}


void ts::DSMCCExtractor::printListSummary()
{
    std::stringstream ss;

    if (_options.data_carousel) {
        ss << "Data carousel: BIOP parsing disabled." << std::endl;
    }
    else {
        std::vector<FileEntry> sorted = _files;
        std::sort(sorted.begin(), sorted.end(),
                  [](const FileEntry& a, const FileEntry& b) { return a.path < b.path; });

        size_t total_bytes = 0;
        for (const auto& f : sorted) {
            total_bytes += f.size;
        }

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

        ss << std::endl
           << UString::Format(u"Files: %d (%d byte(s) total)", sorted.size(), total_bytes).toUTF8()
           << std::endl;
    }

    ss << std::endl << "Modules:" << std::endl;
    _carousel.listModules(ss);

    _duck.report().info(u"%s", UString::FromUTF8(ss.str()));
}
