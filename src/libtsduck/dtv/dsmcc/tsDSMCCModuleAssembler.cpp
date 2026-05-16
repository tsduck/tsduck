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
#include "tsDID.h"
#include "tsMemory.h"

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
        _duck.report().verbose(u"Buffering orphan DDB: download_id=0x%X module=0x%X ver=%d block=%d (%d bytes)",
                               ddm.header.download_id, ddm.module_id, ddm.module_version, ddm.block_number, ddm.block_data.size());
        _orphan_ddbs[key].push_back({ddm.module_version, ddm.block_number, std::move(ddm.block_data)});
        return;
    }

    ModuleContext& ctx = it->second;

    // Skip if version mismatch or if we already have this version completed.
    if (ctx.module_version != ddm.module_version || ctx.status == ModuleContext::Status::COMPLETE) {
        return;
    }

    insertBlock(ctx, ddm.block_number, ddm.block_data);
}


//----------------------------------------------------------------------------
// Insert a single block into a module. Returns true if module became complete.
//----------------------------------------------------------------------------

bool ts::DSMCCModuleAssembler::insertBlock(ModuleContext& ctx, uint16_t block_number, const ByteBlock& data)
{
    // Skip duplicate blocks.
    if (ctx.received_blocks.count(block_number) > 0) {
        return false;
    }

    // Ensure payload buffer is allocated.
    if (ctx.payload.size() < ctx.module_size) {
        ctx.payload.resize(ctx.module_size, 0);
    }

    // Copy block data to the correct offset within the module payload.
    const size_t offset = static_cast<size_t>(block_number) * ctx.block_size;
    const size_t copy_size = std::min(data.size(), static_cast<size_t>(ctx.module_size) - offset);
    if (offset < ctx.module_size && copy_size > 0) {
        MemCopy(ctx.payload.data() + offset, data.data(), copy_size);
        ctx.received_blocks.insert(block_number);
    }

    // Check if all blocks have been received.
    if (ctx.received_blocks.size() >= ctx.expected_blocks) {
        ctx.payload.resize(ctx.module_size);
        ctx.status = ModuleContext::Status::COMPLETE;
        if (_on_module_complete) {
            _on_module_complete(ctx);
        }
        return true;
    }
    return false;
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
            break;
        }
        if (orphan.module_version != ctx.module_version) {
            continue;
        }

        _duck.report().verbose(u"Replaying orphan DDB: download_id=0x%X module=0x%X ver=%d block=%d (%d bytes)",
                               ctx.download_id, ctx.module_id, ctx.module_version, orphan.block_number, orphan.block_data.size());

        insertBlock(ctx, orphan.block_number, orphan.block_data);
    }
    _orphan_ddbs.erase(it);
}


//----------------------------------------------------------------------------
// ModuleContext Implementation
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::ModuleContext::setSize(uint32_t size, uint16_t blk_size)
{
    module_size = size;
    block_size = blk_size > 0 ? blk_size : DEFAULT_BLOCK_SIZE;
    expected_blocks = (size + block_size - 1) / block_size;
}


const ts::DSMCCModuleAssembler::ModuleContext* ts::DSMCCModuleAssembler::module(uint32_t download_id, uint16_t module_id) const
{
    const auto it = _modules.find(ModuleKey {download_id, module_id});
    return it == _modules.end() ? nullptr : &it->second;
}


