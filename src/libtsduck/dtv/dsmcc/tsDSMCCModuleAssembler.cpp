//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCModuleAssembler.h"
#include "tsPSIBuffer.h"
#include "tsDSMCCCompressedModuleDescriptor.h"
#include "tsDSMCCNameDescriptor.h"
#include "tsDID.h"

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCModuleAssembler::DSMCCModuleAssembler(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Clear state
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::clear()
{
    _modules.clear();
    _orphan_ddbs.clear();
}


//----------------------------------------------------------------------------
// Feed a User-to-Network Message (DSI or DII)
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::feedUserToNetwork(const DSMCCUserToNetworkMessage& unm)
{
    if (!unm.isValid()) {
        return;
    }
    if (unm.header.message_id == DSMCC_MSGID_DII) {
        processDII(unm);
    }
}


//----------------------------------------------------------------------------
// Feed a Download Data Message (DDB)
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::feedDownloadData(DSMCCDownloadDataMessage& ddm)
{
    if (!ddm.isValid()) {
        return;
    }
    processDDB(ddm);
}


//----------------------------------------------------------------------------
// Process DII (Download Info Indication)
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::processDII(const DSMCCUserToNetworkMessage& unm)
{
    const auto* dii = unm.toDII();
    if (dii == nullptr) {
        return;
    }

    for (const auto& it : dii->modules) {
        const auto& mod_info = it.second;
        const uint16_t mod_id = mod_info.module_id;
        const ModuleKey key {dii->download_id, mod_id};

        auto& ctx = _modules[key];

        // If new or version changed, reset context
        if (ctx.status == ModuleContext::Status::UNKNOWN || ctx.module_version != mod_info.module_version) {
            const bool first_announcement = ctx.status == ModuleContext::Status::UNKNOWN;
            ctx.download_id = dii->download_id;
            ctx.module_id = mod_id;
            ctx.module_version = mod_info.module_version;
            ctx.setSize(mod_info.module_size, dii->block_size);
            ctx.status = ModuleContext::Status::PENDING;

            // Snapshot the DII's per-module descriptor list. Consumers decode
            // recognized DIDs on demand; the assembler itself only inspects
            // compressed_module_descriptor below.
            ctx.descs = mod_info.descs;

            // Look for compressed_module_descriptor
            const size_t index = mod_info.descs.search(DID_DSMCC_COMPRESSED_MODULE);
            if (index < mod_info.descs.count()) {
                DSMCCCompressedModuleDescriptor desc(_duck, mod_info.descs[index]);
                if (desc.isValid()) {
                    ctx.is_compressed = true;
                    ctx.original_size = desc.original_size;
                }
            }

            _duck.report().verbose(u"Discovered Module: download_id=0x%X id=0x%X size=%d version=%d original_size=%d",
                                   ctx.download_id, mod_id, ctx.module_size, ctx.module_version, ctx.original_size);

            // Fire discovery only on first announcement, not on version bumps —
            // group accounting expects one event per (download_id, module_id).
            if (first_announcement && _on_module_discovered) {
                _on_module_discovered(ctx.download_id, ctx.module_id);
            }

            replayOrphans(ctx);
        }
    }
}


//----------------------------------------------------------------------------
// Process DDB (Download Data Block)
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::processDDB(DSMCCDownloadDataMessage& ddm)
{
    const ModuleKey key {ddm.header.download_id, ddm.module_id};
    auto it = _modules.find(key);
    if (it == _modules.end()) {
        // DDB arrived before the DII that declares this module.
        // Buffer it and replay once the DII catches up.
        _duck.report().verbose(u"Buffering orphan DDB: download_id=0x%X module=0x%X ver=%d (%d bytes)",
                               ddm.header.download_id, ddm.module_id, ddm.module_version, ddm.block_data.size());
        _orphan_ddbs[key].push_back({ddm.module_version, std::move(ddm.block_data)});
        return;
    }

    ModuleContext& ctx = it->second;

    // Skip if version mismatch or if we already have this version completed.
    if (ctx.module_version != ddm.module_version || ctx.status == ModuleContext::Status::COMPLETE) {
        return;
    }

    // Invariant: ddm.block_data already contains the full module payload — the upstream
    // DSMCC section layer concatenates DDB blocks before handing us the DDM. Do not
    // reintroduce a per-block bitmap here unless that layer changes.
    ctx.payload = std::move(ddm.block_data);

    ctx.status = ModuleContext::Status::COMPLETE;

    if (_on_module_complete) {
        _on_module_complete(ctx);
    }
}


//----------------------------------------------------------------------------
// Replay buffered DDBs matching a freshly registered module version.
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::replayOrphans(ModuleContext& ctx)
{
    const ModuleKey key {ctx.download_id, ctx.module_id};
    auto it = _orphan_ddbs.find(key);
    if (it == _orphan_ddbs.end()) {
        return;
    }

    for (auto& orphan : it->second) {
        if (ctx.status == ModuleContext::Status::COMPLETE) {
            break;  // Already assembled; remaining orphans are redundant duplicates.
        }
        if (orphan.module_version != ctx.module_version) {
            continue;
        }
        _duck.report().verbose(u"Replaying orphan DDB: download_id=0x%X module=0x%X ver=%d (%d bytes)",
                               ctx.download_id, ctx.module_id, ctx.module_version, orphan.block_data.size());
        ctx.payload = std::move(orphan.block_data);
        ctx.status = ModuleContext::Status::COMPLETE;
        if (_on_module_complete) {
            _on_module_complete(ctx);
        }
    }
    _orphan_ddbs.erase(it);
}


//----------------------------------------------------------------------------
// ModuleContext Implementation
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::ModuleContext::setSize(uint32_t size, uint16_t blk_size)
{
    // Fall back to the DSM-CC default block size if the DII announces 0 —
    // the spec forbids it but malformed carousels show up and we must not divide by zero.
    module_size = size;
    block_size = blk_size > 0 ? blk_size : 4066;
    expected_blocks = (size + block_size - 1) / block_size;
}


const ts::DSMCCModuleAssembler::ModuleContext* ts::DSMCCModuleAssembler::module(uint32_t download_id, uint16_t module_id) const
{
    const auto it = _modules.find(ModuleKey {download_id, module_id});
    return it == _modules.end() ? nullptr : &it->second;
}


ts::UString ts::DSMCCModuleAssembler::listModules() const
{
    UString out;
    for (const auto& pair : _modules) {
        const auto& ctx = pair.second;
        const UString comp = ctx.is_compressed
            ? UString::Format(u"yes (orig %d)", ctx.original_size)
            : UString(u"no");
        out += UString::Format(u"Dl: %08X | ID: %04X | Ver: %d | Size: %6d | Blocks: %3d | Compressed: %s | Status: %s",
                               ctx.download_id, ctx.module_id, ctx.module_version, ctx.module_size,
                               ctx.expected_blocks, comp,
                               ctx.isComplete() ? u"COMPLETE" : u"PENDING");
        const size_t ni = ctx.descs.search(DID_DSMCC_NAME);
        if (ni < ctx.descs.count()) {
            const DSMCCNameDescriptor nd(_duck, ctx.descs[ni]);
            if (nd.isValid() && !nd.name.empty()) {
                out += UString::Format(u" | Name: \"%s\"", nd.name);
            }
        }
        out += u"\n";
    }
    return out;
}
