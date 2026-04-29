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
#include <algorithm>
#include <filesystem>


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
            _duck.report().verbose(u"Module 0x%X descriptors:\n%s", module_id, describeModule(*ctx));
        }
    }

    if (_options.list_mode) {
        return;
    }
    if (!_options.dump_modules && !_options.data_carousel) {
        return;
    }

    namespace fs = std::filesystem;
    const fs::path out = fs::path(_options.out_dir.toUTF8());

    fs::path dir;
    UString leaf;
    if (_options.data_carousel) {
        // Group hierarchy: out/<download_id_hex>/<label_or_module_XXXX>.bin
        dir = out / UString::Format(u"%08X", download_id).toUTF8();
        leaf = moduleFilename(download_id, module_id);
    }
    else {
        // Object-carousel dump_modules: out/modules/module_XXXX.bin
        dir = out / "modules";
        leaf = UString::Format(u"module_%04X.bin", module_id);
    }

    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        _duck.report().error(u"Cannot create directory %s: %s",
                             UString::FromUTF8(dir.string()),
                             UString::FromUTF8(ec.message()));
        return;
    }
    const UString filename = UString::FromUTF8((dir / leaf.toUTF8()).string());
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
    _resolved_files.push_back({name, file.content.size()});
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

    const fs::path out = fs::path(_options.out_dir.toUTF8());
    const fs::path full = out / "files" / rel;

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
    UString text;

    if (_options.data_carousel) {
        text += u"Data carousel:\n";
        if (_carousel.groups().empty()) {
            text += u"  (no groups seen)\n";
        }
        else {
            for (const auto& [gid, gctx] : _carousel.groups()) {
                text += UString::Format(u"%08X/  (%d module(s), %d complete)\n",
                                        gctx.download_id, gctx.module_ids.size(), gctx.modules_complete);
                for (uint16_t mid : gctx.module_ids) {
                    const auto* mctx = _carousel.module(gid, mid);
                    const UString leaf = moduleFilename(gid, mid);
                    const uint32_t sz = mctx ? mctx->module_size : 0;
                    const UString status = (mctx && mctx->isComplete()) ? u"" : u"  [PENDING]";
                    text += UString::Format(u"  %s  (%d bytes)%s\n", leaf, sz, status);
                }
            }
        }
    }
    else {
        std::vector<FileEntry> sorted = _resolved_files;
        std::sort(sorted.begin(), sorted.end(),
                  [](const FileEntry& a, const FileEntry& b) { return a.path < b.path; });

        size_t total_bytes = 0;
        for (const auto& f : sorted) {
            total_bytes += f.size;
        }

        text += u"Carousel tree:\n";
        if (sorted.empty()) {
            text += u"  (no resolved objects)\n";
        }
        else {
            text += u"/\n";
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
                    text += UString(2 * (i + 1), u' ') + dirs[i] + u"/\n";
                }
                text += UString(2 * (dirs.size() + 1), u' ') + segs.back()
                      + UString::Format(u"  (%d bytes)\n", f.size);
                prev_dirs = dirs;
            }
        }

        text += UString::Format(u"\nFiles: %d (%d byte(s) total)\n", sorted.size(), total_bytes);
    }

    const UString groups = _carousel.listGroups();
    if (!groups.empty()) {
        text += u"\nGroups:\n";
        text += groups;
    }

    text += u"\nModules:\n";
    text += _carousel.listModules();

    for (const auto& [key, ctx] : _carousel.modules()) {
        if (ctx.descs.count() > 0) {
            text += UString::Format(u"Module 0x%X descriptors:\n", ctx.module_id);
            text += describeModule(ctx);
        }
    }

    _duck.report().info(u"%s", text);
}


ts::UString ts::DSMCCExtractor::moduleFilename(uint32_t download_id, uint16_t module_id) const
{
    const auto* ctx = _carousel.module(download_id, module_id);
    if (ctx != nullptr) {
        const size_t li = ctx->descs.search(DID_DSMCC_LABEL);
        if (li < ctx->descs.count()) {
            DSMCCLabelDescriptor ld(_duck, ctx->descs[li]);
            if (ld.isValid() && !ld.label.empty()) {
                UString safe = ld.label;
                for (auto& c : safe) {
                    if (c == u'/' || c == u'\\') {
                        c = u'_';
                    }
                }
                if (safe != u"." && safe != u"..") {
                    return safe;
                }
            }
        }
    }
    return UString::Format(u"module_%04X.bin", module_id);
}


ts::UString ts::DSMCCExtractor::describeModule(const DSMCCModuleAssembler::ModuleContext& ctx) const
{
    UString out;
    size_t other = 0;

    auto tryDesc = [&](DID did, auto decode) {
        const size_t i = ctx.descs.search(did);
        if (i < ctx.descs.count()) {
            decode(i);
        }
    };

    tryDesc(DID_DSMCC_NAME, [&](size_t i) {
        DSMCCNameDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Name: \"%s\"\n", d.name);
    });
    tryDesc(DID_DSMCC_TYPE, [&](size_t i) {
        DSMCCTypeDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Type: \"%s\"\n", d.type);
    });
    tryDesc(DID_DSMCC_INFO, [&](size_t i) {
        DSMCCInfoDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Info: [%s] \"%s\"\n", d.language_code, d.info);
    });
    tryDesc(DID_DSMCC_LABEL, [&](size_t i) {
        DSMCCLabelDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Label: \"%s\"\n", d.label);
    });
    tryDesc(DID_DSMCC_CONTENT_TYPE, [&](size_t i) {
        DSMCCContentTypeDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Content-Type: \"%s\"\n", d.content_type);
    });
    tryDesc(DID_DSMCC_CACHING_PRIORITY, [&](size_t i) {
        DSMCCCachingPriorityDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Caching: priority=%d transparency=%d\n", d.priority_value, d.transparency_level);
    });
    tryDesc(DID_DSMCC_CRC32, [&](size_t i) {
        DSMCCCRC32Descriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"CRC32: 0x%08X\n", d.crc32);
    });
    tryDesc(DID_DSMCC_EST_DOWNLOAD_TIME, [&](size_t i) {
        DSMCCEstDownloadTimeDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Est-Download: %ds\n", d.est_download_time);
    });
    tryDesc(DID_DSMCC_LOCATION, [&](size_t i) {
        DSMCCLocationDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Location: 0x%02X\n", d.location_tag);
    });
    tryDesc(DID_DSMCC_MODULE_LINK, [&](size_t i) {
        DSMCCModuleLinkDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Module-Link: pos=%d module=0x%X\n", d.position, d.module_id);
    });
    tryDesc(DID_DSMCC_SSU_MODULE_TYPE, [&](size_t i) {
        DSMCCSSUModuleTypeDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"SSU-Type: 0x%02X\n", d.ssu_module_type);
    });
    tryDesc(DID_DSMCC_SUBGROUP_ASSOCIATION, [&](size_t i) {
        DSMCCSubgroupAssociationDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Subgroup: 0x%010X\n", d.subgroup_tag);
    });
    tryDesc(DID_DSMCC_GROUP_LINK, [&](size_t i) {
        DSMCCGroupLinkDescriptor d(_duck, ctx.descs[i]);
        if (d.isValid()) out += UString::Format(u"Group-Link: pos=%d group=0x%X\n", d.position, d.group_id);
    });

    // Count unrecognized (non-compressed) DIDs.
    const std::initializer_list<DID> known = {
        DID_DSMCC_NAME, DID_DSMCC_TYPE, DID_DSMCC_INFO, DID_DSMCC_LABEL,
        DID_DSMCC_CONTENT_TYPE, DID_DSMCC_CACHING_PRIORITY, DID_DSMCC_CRC32,
        DID_DSMCC_EST_DOWNLOAD_TIME, DID_DSMCC_LOCATION, DID_DSMCC_MODULE_LINK,
        DID_DSMCC_SSU_MODULE_TYPE, DID_DSMCC_SUBGROUP_ASSOCIATION, DID_DSMCC_GROUP_LINK,
        DID_DSMCC_COMPRESSED_MODULE,
    };
    for (size_t i = 0; i < ctx.descs.count(); ++i) {
        const DID did = ctx.descs[i].tag();
        bool recognized = false;
        for (DID k : known) {
            if (did == k) { recognized = true; break; }
        }
        if (!recognized) {
            ++other;
        }
    }
    if (other > 0) {
        out += UString::Format(u"Other: %d descriptor(s) (raw)\n", other);
    }

    return out;
}
