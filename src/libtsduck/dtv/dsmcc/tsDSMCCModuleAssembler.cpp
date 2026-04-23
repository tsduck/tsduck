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

        auto& ctx = _modules[mod_id];

        // If new or version changed, reset context
        if (ctx.status == ModuleContext::Status::UNKNOWN || ctx.module_version != mod_info.module_version) {
            ctx.module_id = mod_id;
            ctx.module_version = mod_info.module_version;
            ctx.setSize(mod_info.module_size, dii->block_size);
            ctx.status = ModuleContext::Status::PENDING;

            // Look for compressed_module_descriptor
            const size_t index = mod_info.descs.search(DID_DSMCC_COMPRESSED_MODULE);
            if (index < mod_info.descs.count()) {
                DSMCCCompressedModuleDescriptor desc(_duck, mod_info.descs[index]);
                if (desc.isValid()) {
                    ctx.is_compressed = true;
                    ctx.original_size = desc.original_size;
                }
            }

            _duck.report().verbose(u"Discovered Module ID: 0x%X, Size: %d, Version: %d, Original Size: %d",
                                   mod_id, ctx.module_size, ctx.module_version, ctx.original_size);

            replayOrphans(ctx);
        }
    }
}


//----------------------------------------------------------------------------
// Process DDB (Download Data Block)
//----------------------------------------------------------------------------

void ts::DSMCCModuleAssembler::processDDB(DSMCCDownloadDataMessage& ddm)
{
    auto it = _modules.find(ddm.module_id);
    if (it == _modules.end()) {
        // DDB arrived before the DII that declares this module.
        // Buffer it and replay once the DII catches up.
        _duck.report().verbose(u"Buffering orphan DDB: Module 0x%X Ver %d (%d bytes)",
                               ddm.module_id, ddm.module_version, ddm.block_data.size());
        _orphan_ddbs[ddm.module_id].push_back({ddm.module_version, std::move(ddm.block_data)});
        return;
    }

    ModuleContext& ctx = it->second;

    // Skip if version mismatch or if we already have this version completed.
    if (ctx.module_version != ddm.module_version || ctx.status == ModuleContext::Status::COMPLETE) {
        return;
    }

    _duck.report().verbose(u"Module 0x%X Complete! (Size: %d)", ctx.module_id, ddm.block_data.size());

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
    auto it = _orphan_ddbs.find(ctx.module_id);
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
        _duck.report().verbose(u"Replaying orphan DDB: Module 0x%X Ver %d (%d bytes)",
                               ctx.module_id, ctx.module_version, orphan.block_data.size());
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


void ts::DSMCCModuleAssembler::listModules(std::ostream& out) const
{
    for (const auto& pair : _modules) {
        const auto& ctx = pair.second;
        UString comp;
        if (ctx.is_compressed) {
            comp = UString::Format(u"yes (orig %d)", ctx.original_size);
        }
        else {
            comp = u"no";
        }
        out << UString::Format(u"ID: %04X | Ver: %d | Size: %6d | Blocks: %3d | Compressed: %s | Status: %s\n",
                               ctx.module_id, ctx.module_version, ctx.module_size,
                               ctx.expected_blocks, comp,
                               ctx.isComplete() ? u"COMPLETE" : u"PENDING");
    }
}
