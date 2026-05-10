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
#include "tsDID.h"
#include "tsDSMCCNameDescriptor.h"
#include "tsDSMCCTypeDescriptor.h"
#include "tsDSMCCInfoDescriptor.h"
#include "tsDSMCCLabelDescriptor.h"
#include "tsDSMCCContentTypeDescriptor.h"
#include "tsDSMCCCachingPriorityDescriptor.h"
#include "tsDSMCCCRC32Descriptor.h"
#include "tsDSMCCEstDownloadTimeDescriptor.h"
#include "tsDSMCCLocationDescriptor.h"
#include "tsDSMCCModuleLinkDescriptor.h"
#include "tsDSMCCSSUModuleTypeDescriptor.h"
#include "tsDSMCCSubgroupAssociationDescriptor.h"
#include "tsDSMCCGroupLinkDescriptor.h"
#include "tsDSMCCCompressedModuleDescriptor.h"
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>


namespace {
    constexpr auto FILES_SUBDIR = "files";
    constexpr auto MODULES_SUBDIR = "modules";

    ts::UString pathToUString(const std::filesystem::path& p)
    {
        return ts::UString::FromUTF8(p.string());
    }
}


ts::DSMCCExtractor::DSMCCExtractor(DuckContext& duck, const Options& options) :
    _duck(duck),
    _options(options),
    _carousel(duck),
    _demux(duck, this, nullptr)
{
    _carousel.setScanBIOP(!_options.data_carousel);

    _carousel.setModuleCompletedHandler([this](uint32_t download_id, uint16_t module_id, const ByteBlock& payload) {
        onModuleCompleted(download_id, module_id, payload);
    });

    _carousel.setGroupCompletedHandler([this](const DSMCCCarousel::GroupContext& gctx) {
        _duck.report().verbose(u"Group complete: download_id=0x%X, %d module(s)",
                               gctx.download_id, gctx.module_ids.size());
    });

    if (!_options.data_carousel) {
        _carousel.setObjectHandler([this](uint32_t download_id, uint16_t module_id, const UString& name, const BIOPMessage& msg) {
            onObjectReady(download_id, module_id, name, msg);
        });
    }
}


void ts::DSMCCExtractor::setPID(PID pid)
{
    _pid = pid;
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


void ts::DSMCCExtractor::onModuleCompleted(uint32_t download_id, uint16_t module_id, const ByteBlock& payload)
{
    _duck.report().verbose(u"Module complete: download_id=0x%X id=0x%X size=%d",
                           download_id, module_id, payload.size());

    if (!_options.list_mode && _duck.report().verbose()) {
        const auto* ctx = _carousel.module(download_id, module_id);
        if (ctx != nullptr && ctx->descs.count() > 0) {
            _duck.report().verbose(u"Module 0x%X descriptors:\n%s", module_id, renderDescriptorList(ctx->descs));
        }
    }

    if (_options.list_mode) {
        return;
    }
    if (!_options.dump_modules && !_options.data_carousel) {
        return;
    }

    auto [dir, leaf] = computeOutputPath(download_id, module_id);
    if (!ensureDir(dir)) {
        return;
    }
    const UString filename = pathToUString(dir / leaf.toUTF8());
    if (payload.saveToFile(filename, &_duck.report())) {
        _duck.report().verbose(u"  -> Saved to %s", filename);
    }
}


std::pair<std::filesystem::path, ts::UString> ts::DSMCCExtractor::computeOutputPath(uint32_t download_id, uint16_t module_id) const
{
    namespace fs = std::filesystem;
    const fs::path out = fs::path(_options.out_dir.toUTF8());
    const std::string dl_dir = UString::Format(u"%08X", download_id).toUTF8();

    if (_options.data_carousel) {
        // Group hierarchy: out/<download_id_hex>/<label_or_module_XXXX>.bin
        return {out / dl_dir, moduleFilename(download_id, module_id)};
    }
    // Object-carousel dump_modules: out/modules/<download_id_hex>/module_XXXX.bin
    return {out / MODULES_SUBDIR / dl_dir,
            UString::Format(u"module_%04X.bin", module_id)};
}


bool ts::DSMCCExtractor::ensureDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        _duck.report().error(u"Cannot create directory %s: %s",
                             pathToUString(dir),
                             UString::FromUTF8(ec.message()));
        return false;
    }
    return true;
}


void ts::DSMCCExtractor::onObjectReady(uint32_t download_id, uint16_t module_id, const UString& name, const BIOPMessage& msg)
{
    _duck.report().verbose(u"Module 0x%X BIOP object: name=\"%s\" kind=\"%s\"",
                           module_id,
                           name.empty() ? u"(unresolved)" : name,
                           UString::FromUTF8(msg.kindTag()));

    const std::string kind = msg.kindTag();
    const bool is_file = kind == BIOPObjectKind::FILE;
    const bool is_dir = kind == BIOPObjectKind::DIRECTORY;
    const BIOPFileMessage* file = is_file ? static_cast<const BIOPFileMessage*>(&msg) : nullptr;
    const size_t content_size = file ? file->content.size() : 0;

    _objects_by_module[{download_id, module_id}].push_back(_objects.size());
    _objects.push_back({download_id, module_id, name, kind, content_size});

    if (_options.list_mode || name.empty()) {
        return;
    }
    if (file) {
        extractFile(name, *file);
    }
    else if (is_dir) {
        extractDirectory(name);
    }
}


void ts::DSMCCExtractor::extractDirectory(const UString& name)
{
    const auto full = safeOutputPath(name);
    if (full && ensureDir(*full)) {
        _duck.report().verbose(u"Created directory %s", pathToUString(*full));
    }
}


void ts::DSMCCExtractor::extractFile(const UString& name, const BIOPFileMessage& file)
{
    const auto full = safeOutputPath(name);
    if (!full || !ensureDir(full->parent_path())) {
        return;
    }
    const UString out_path = pathToUString(*full);
    if (file.content.saveToFile(out_path, &_duck.report())) {
        _duck.report().verbose(u"Extracted %s (%d bytes)", out_path, file.content.size());
    }
}


std::optional<std::filesystem::path> ts::DSMCCExtractor::safeOutputPath(const UString& name) const
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
            return std::nullopt;
        }
        rel /= seg.toUTF8();
    }
    if (rel.empty()) {
        return std::nullopt;
    }
    return fs::path(_options.out_dir.toUTF8()) / FILES_SUBDIR / rel;
}


void ts::DSMCCExtractor::printListSummary()
{
    UString text = renderCarousels();
    if (!_options.data_carousel) {
        text += renderFileTreeFinal();
    }
    _duck.report().info(u"%s", text);
}


ts::UString ts::DSMCCExtractor::renderCarousels() const
{
    UString out;
    bool first = true;
    for (const auto& [dl, gctx] : _carousel.groups()) {
        if (!first) {
            out += u"\n";
        }
        out += renderGroupBlock(gctx);
        first = false;
    }
    return out;
}


ts::UString ts::DSMCCExtractor::renderGroupBlock(const DSMCCCarousel::GroupContext& gctx) const
{
    UString out;
    out += UString::Format(u"* DSM-CC Carousel, PID 0x%04X (%d)\n", _pid, _pid);
    out += UString::Format(u"%sDownload id: 0x%08X (%d)\n", u"  ", gctx.download_id, gctx.download_id);
    out += UString::Format(u"%sModules discovered: %d\n", u"  ", gctx.module_ids.size());
    out += UString::Format(u"%sGroup complete: %s\n", u"  ", gctx.isComplete() ? u"yes" : u"no");
    for (uint16_t mid : gctx.module_ids) {
        const auto* mctx = _carousel.module(gctx.download_id, mid);
        if (mctx != nullptr) {
            out += renderModuleBlock(*mctx);
        }
    }
    return out;
}


ts::UString ts::DSMCCExtractor::renderModuleBlock(const DSMCCModuleAssembler::ModuleContext& ctx) const
{
    const UString size_extra = ctx.is_compressed
        ? UString::Format(u", original size: %d bytes", ctx.original_size)
        : UString();

    UString out;
    out += UString::Format(u"%s- Module 0x%04X\n", u"  ", ctx.module_id);
    out += UString::Format(u"%sVersion: %d, blocks: %d\n", u"    ", ctx.module_version, ctx.expected_blocks);
    out += UString::Format(u"%sSize: %d bytes%s\n", u"    ", ctx.module_size, size_extra);
    out += UString::Format(u"%sCompression: %s\n", u"    ", ctx.is_compressed ? u"yes" : u"no");
    out += UString::Format(u"%sStatus: %s\n", u"    ", ctx.isComplete() ? u"COMPLETE" : u"PENDING");
    if (!_options.data_carousel) {
        out += renderObjectsForModule(ctx.download_id, ctx.module_id);
    }
    out += renderDescriptorList(ctx.descs);
    return out;
}


ts::UString ts::DSMCCExtractor::renderObjectsForModule(uint32_t download_id, uint16_t module_id) const
{
    UString out;
    const auto it = _objects_by_module.find({download_id, module_id});
    const size_t n = (it == _objects_by_module.end()) ? 0 : it->second.size();
    out += UString::Format(u"%sBIOP objects: %d\n", u"    ", n);
    if (n == 0) {
        return out;
    }
    for (size_t i = 0; i < n; ++i) {
        const auto& obj = _objects[it->second[i]];
        out += UString::Format(u"%s- Object %d: \"%s\" [%s]\n",
                               u"    ",
                               i,
                               obj.path.empty() ? u"(unresolved)" : obj.path,
                               UString::FromUTF8(obj.kind));
    }
    return out;
}


ts::UString ts::DSMCCExtractor::renderDescriptorList(const DescriptorList& descs) const
{
    UString out;
    for (size_t i = 0; i < descs.count(); ++i) {
        out += renderOneDescriptor(i, descs[i]);
    }
    return out;
}


ts::UString ts::DSMCCExtractor::renderOneDescriptor(size_t index, const Descriptor& raw) const
{
    const size_t bytes = raw.payloadSize();
    UString header;
    UString body;

    switch (raw.tag()) {
        case DID_DSMCC_TYPE: {
            DSMCCTypeDescriptor desc(_duck, raw);
            header = u"Type";
            if (desc.isValid()) body += UString::Format(u"      Module type: %s\n", desc.type);
            break;
        }
        case DID_DSMCC_NAME: {
            DSMCCNameDescriptor desc(_duck, raw);
            header = u"Name";
            if (desc.isValid()) body += UString::Format(u"      Name: \"%s\"\n", desc.name);
            break;
        }
        case DID_DSMCC_INFO: {
            DSMCCInfoDescriptor desc(_duck, raw);
            header = u"Info";
            if (desc.isValid()) {
                body += UString::Format(u"      Language: %s\n", desc.language_code);
                body += UString::Format(u"      Info: \"%s\"\n", desc.info);
            }
            break;
        }
        case DID_DSMCC_LABEL: {
            DSMCCLabelDescriptor desc(_duck, raw);
            header = u"Label";
            if (desc.isValid()) body += UString::Format(u"      Label: \"%s\"\n", desc.label);
            break;
        }
        case DID_DSMCC_CONTENT_TYPE: {
            DSMCCContentTypeDescriptor desc(_duck, raw);
            header = u"Content Type";
            if (desc.isValid()) body += UString::Format(u"      Content type: \"%s\"\n", desc.content_type);
            break;
        }
        case DID_DSMCC_CACHING_PRIORITY: {
            DSMCCCachingPriorityDescriptor desc(_duck, raw);
            header = u"Caching Priority";
            if (desc.isValid()) {
                body += UString::Format(u"      Priority: %d\n", desc.priority_value);
                body += UString::Format(u"      Transparency: %d\n", desc.transparency_level);
            }
            break;
        }
        case DID_DSMCC_CRC32: {
            DSMCCCRC32Descriptor desc(_duck, raw);
            header = u"CRC32";
            if (desc.isValid()) body += UString::Format(u"      CRC32: 0x%08X\n", desc.crc32);
            break;
        }
        case DID_DSMCC_EST_DOWNLOAD_TIME: {
            DSMCCEstDownloadTimeDescriptor desc(_duck, raw);
            header = u"Estimated Download Time";
            if (desc.isValid()) body += UString::Format(u"      Estimated time: %d s\n", desc.est_download_time);
            break;
        }
        case DID_DSMCC_LOCATION: {
            DSMCCLocationDescriptor desc(_duck, raw);
            header = u"Location";
            if (desc.isValid()) body += UString::Format(u"      Location tag: 0x%02X\n", desc.location_tag);
            break;
        }
        case DID_DSMCC_MODULE_LINK: {
            DSMCCModuleLinkDescriptor desc(_duck, raw);
            header = u"Module Link";
            if (desc.isValid()) {
                body += UString::Format(u"      Position: %d\n", desc.position);
                body += UString::Format(u"      Linked module: 0x%04X (%d)\n", desc.module_id, desc.module_id);
            }
            break;
        }
        case DID_DSMCC_SSU_MODULE_TYPE: {
            DSMCCSSUModuleTypeDescriptor desc(_duck, raw);
            header = u"SSU Module Type";
            if (desc.isValid()) body += UString::Format(u"      SSU module type: 0x%02X\n", desc.ssu_module_type);
            break;
        }
        case DID_DSMCC_SUBGROUP_ASSOCIATION: {
            DSMCCSubgroupAssociationDescriptor desc(_duck, raw);
            header = u"Subgroup Association";
            if (desc.isValid()) body += UString::Format(u"      Subgroup tag: 0x%010X\n", desc.subgroup_tag);
            break;
        }
        case DID_DSMCC_GROUP_LINK: {
            DSMCCGroupLinkDescriptor desc(_duck, raw);
            header = u"Group Link";
            if (desc.isValid()) {
                body += UString::Format(u"      Position: %d\n", desc.position);
                body += UString::Format(u"      Group id: 0x%08X (%d)\n", desc.group_id, desc.group_id);
            }
            break;
        }
        case DID_DSMCC_COMPRESSED_MODULE: {
            DSMCCCompressedModuleDescriptor desc(_duck, raw);
            header = u"Compressed Module";
            if (desc.isValid()) {
                body += UString::Format(u"      Compression method: 0x%02X\n", desc.compression_method);
                body += UString::Format(u"      Original size: %d bytes\n", desc.original_size);
            }
            break;
        }
        default:
            header = UString::Format(u"Unknown (tag 0x%02X)", raw.tag());
            body = UString::Format(u"      Raw: %d bytes\n", bytes);
            break;
    }

    UString out;
    out += UString::Format(u"    - Descriptor %d: %s, %d bytes\n", index, header, bytes);
    out += body;
    return out;
}


ts::UString ts::DSMCCExtractor::renderFileTreeFinal() const
{
    UString out;
    out += u"\n* Carousel Tree\n";

    // FILE path prefixes will emit their parent dirs implicitly during the tree walk.
    // Skip DIRECTORY entries whose path is already covered, otherwise the dir line
    // would be emitted twice. Standalone DIRECTORY entries (empty dirs) are kept.
    std::set<UString> implicit_dirs;
    for (const auto& obj : _objects) {
        if (obj.kind != BIOPObjectKind::FILE || obj.path.empty()) {
            continue;
        }
        UStringVector segs;
        obj.path.split(segs, u'/', true, true);
        UString prefix;
        for (size_t k = 0; k + 1 < segs.size(); ++k) {
            prefix += u"/";
            prefix += segs[k];
            implicit_dirs.insert(prefix);
        }
    }

    std::vector<const ObjectEntry*> entries;
    size_t file_count = 0;
    size_t total_bytes = 0;
    for (const auto& obj : _objects) {
        if (obj.path.empty()) {
            continue;
        }
        if (obj.kind == BIOPObjectKind::FILE) {
            entries.push_back(&obj);
            ++file_count;
            total_bytes += obj.size;
        }
        else if (obj.kind == BIOPObjectKind::DIRECTORY && !implicit_dirs.contains(obj.path)) {
            entries.push_back(&obj);
        }
    }
    if (entries.empty()) {
        out += u"  (no resolved files)\n";
        return out;
    }

    std::sort(entries.begin(), entries.end(),
              [](const ObjectEntry* a, const ObjectEntry* b) { return a->path < b->path; });

    // Per-directory leaf-name padding so size columns line up under the same parent.
    std::map<UString, size_t> max_leaf;
    for (const auto* e : entries) {
        if (e->kind != BIOPObjectKind::FILE) {
            continue;
        }
        UStringVector segs;
        e->path.split(segs, u'/', true, true);
        if (segs.empty()) {
            continue;
        }
        UString dir;
        for (size_t k = 0; k + 1 < segs.size(); ++k) {
            dir += u"/";
            dir += segs[k];
        }
        max_leaf[dir] = std::max(max_leaf[dir], segs.back().length());
    }

    out += u"  /\n";
    UStringVector prev_dirs;
    for (const auto* e : entries) {
        UStringVector segs;
        e->path.split(segs, u'/', true, true);
        if (segs.empty()) {
            continue;
        }
        UStringVector dirs(segs.begin(), segs.end() - 1);
        size_t common = 0;
        while (common < dirs.size() && common < prev_dirs.size() && dirs[common] == prev_dirs[common]) {
            ++common;
        }
        for (size_t k = common; k < dirs.size(); ++k) {
            out += UString(2 * (k + 2), u' ') + dirs[k] + u"/\n";
        }

        const UString indent(2 * (dirs.size() + 2), u' ');
        if (e->kind == BIOPObjectKind::DIRECTORY) {
            out += indent + segs.back() + u"/\n";
            // Track this dir as emitted so a subsequent file under it skips re-emit.
            prev_dirs = dirs;
            prev_dirs.push_back(segs.back());
        }
        else {
            UString dir_key;
            for (size_t k = 0; k + 1 < segs.size(); ++k) {
                dir_key += u"/";
                dir_key += segs[k];
            }
            const size_t pad = max_leaf[dir_key];
            UString leaf = segs.back();
            if (leaf.length() < pad) {
                leaf += UString(pad - leaf.length(), u' ');
            }
            out += indent + leaf + UString::Format(u" (%d bytes)\n", e->size);
            prev_dirs = dirs;
        }
    }

    out += UString::Format(u"\n  Files: %d\n", file_count);
    out += UString::Format(u"  Total size: %d bytes\n", total_bytes);
    return out;
}


ts::UString ts::DSMCCExtractor::moduleFilename(uint32_t download_id, uint16_t module_id) const
{
    const auto* ctx = _carousel.module(download_id, module_id);
    if (ctx != nullptr) {
        const size_t label_desc_idx = ctx->descs.search(DID_DSMCC_LABEL);
        if (label_desc_idx < ctx->descs.count()) {
            DSMCCLabelDescriptor label_desc(_duck, ctx->descs[label_desc_idx]);
            if (label_desc.isValid() && !label_desc.label.empty()) {
                UString safe_filename = label_desc.label;
                for (auto& c : safe_filename) {
                    if (c == u'/' || c == u'\\') {
                        c = u'_';
                    }
                }
                if (safe_filename != u"." && safe_filename != u"..") {
                    return safe_filename;
                }
            }
        }
    }
    return UString::Format(u"module_%04X.bin", module_id);
}
