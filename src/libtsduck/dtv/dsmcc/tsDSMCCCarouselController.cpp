//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCarouselController.h"
#include "tsPSIBuffer.h"
#include "tsDSMCCCompressedModuleDescriptor.h"

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DSMCCCarouselController::DSMCCCarouselController(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Clear state
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::clear()
{
    _state = State::UNMOUNTED;
    _modules.clear();
    _dsi_found = false;
}


//----------------------------------------------------------------------------
// Handle Tables (DSI, DII, DDM)
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Filter for DSM-CC User-to-Network Messages (0x3B)
    if (table.tableId() == TID_DSMCC_UNM) {
        DSMCCUserToNetworkMessage unm(_duck, table);
        if (unm.isValid()) {
            if (unm.header.message_id == DSMCC_MSGID_DSI) {
                processDSI(unm);
            }
            else if (unm.header.message_id == DSMCC_MSGID_DII) {
                processDII(unm);
            }
        }
    }
    // Filter for DSM-CC Download Data Messages (0x3C)
    else if (table.tableId() == TID_DSMCC_DDM) {
        DSMCCDownloadDataMessage ddm(_duck, table);
        if (ddm.isValid()) {
            processDDB(ddm);
        }
    }
}


//----------------------------------------------------------------------------
// Handle Sections (DDB progress tracking)
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::handleSection(SectionDemux& demux, const Section& section)
{
    if (section.tableId() == TID_DSMCC_DDM) {
        auto it = _modules.find(section.tableIdExtension());
        if (it != _modules.end()) {
            ModuleContext& ctx = it->second;
            if (ctx.status != ModuleContext::Status::COMPLETE) {
                if (ctx.markBlockReceived(section.sectionNumber())) {
                    _state = State::LOADING;
                    _duck.report().verbose(u"Module 0x%X progress: %d/%d blocks",
                                           ctx.module_id, ctx.countReceived(), ctx.expected_blocks);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process DSI (Download Server Initiate)
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::processDSI(const DSMCCUserToNetworkMessage& unm)
{
    // In a full implementation, we would parse the Service Gateway IOR here.
    // For now, we just note that the DSI is present and we are "MOUNTED".

    if (!_dsi_found) {
        _duck.report().verbose(u"New DSI Transaction ID detected: 0x%X", unm.header.transaction_id);

        _dsi_found = true;

        if (_state == State::UNMOUNTED) {
            _state = State::MOUNTING;
            checkGlobalState();
        }
    }
}


//----------------------------------------------------------------------------
// Process DII (Download Info Indication)
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::processDII(const DSMCCUserToNetworkMessage& unm)
{
    for (const auto& it : unm.modules) {
        const auto& mod_info = it.second;
        const uint16_t mod_id = mod_info.module_id;

        auto& ctx = _modules[mod_id];

        // If new or version changed, reset context
        if (ctx.status == ModuleContext::Status::UNKNOWN || ctx.module_version != mod_info.module_version) {
            ctx.module_id = mod_id;
            ctx.module_version = mod_info.module_version;
            ctx.setSize(mod_info.module_size, unm.block_size > 0 ? unm.block_size : 4066);
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
        }
    }

    checkGlobalState();
}


//----------------------------------------------------------------------------
// Process DDB (Download Data Block)
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::processDDB(DSMCCDownloadDataMessage& ddm)
{
    auto it = _modules.find(ddm.module_id);
    if (it == _modules.end()) {
        // We received a block for a module we don't know about yet (no DII).
        return;
    }

    ModuleContext& ctx = it->second;

    // Skip if version mismatch or if we already have this version completed.
    if (ctx.module_version != ddm.module_version || ctx.status == ModuleContext::Status::COMPLETE) {
        return;
    }

    _duck.report().verbose(u"Module 0x%X Complete! (Size: %d)", ctx.module_id, ddm.block_data.size());

    // Move payload for performance (avoid large memory copy)
    ctx.payload = std::move(ddm.block_data);

    if (_on_module_complete) {
        _on_module_complete(ctx);
    }

    ctx.status = ModuleContext::Status::COMPLETE;
    ctx.received_blocks.assign(ctx.expected_blocks, true);

    checkGlobalState();
}


//----------------------------------------------------------------------------
// Helper: Check Global State
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::checkGlobalState()
{
    bool all_complete = true;
    bool any_pending = false;

    if (_modules.empty()) {
        all_complete = false;
    }
    else {
        for (const auto& pair : _modules) {
            if (!pair.second.isComplete()) {
                all_complete = false;
                if (pair.second.status != ModuleContext::Status::UNKNOWN) {
                    any_pending = true;
                }
            }
        }
    }

    if (all_complete && !_modules.empty()) {
        _state = State::READY;
    }
    else if (any_pending) {
        _state = State::LOADING;
    }
    else if (_dsi_found) {
        _state = State::DISCOVERING;
    }
}


//----------------------------------------------------------------------------
// ModuleContext Implementation
//----------------------------------------------------------------------------

void ts::DSMCCCarouselController::ModuleContext::setSize(uint32_t size, uint16_t blk_size)
{
    module_size = size;
    block_size = blk_size;

    // Calculate expected blocks
    expected_blocks = (size + block_size - 1) / block_size;
    received_blocks.assign(expected_blocks, false);
}

bool ts::DSMCCCarouselController::ModuleContext::markBlockReceived(uint8_t section_num)
{
    if (section_num < received_blocks.size() && !received_blocks[section_num]) {
        received_blocks[section_num] = true;
        return true;
    }
    return false;
}


void ts::DSMCCCarouselController::listModules(std::ostream& out) const
{
    for (const auto& pair : _modules) {
        const auto& ctx = pair.second;
        out << UString::Format(u"ID: %04X | Ver: %d | Size: %6d | Blocks: %3d/%3d | Status: %s",
                               ctx.module_id, ctx.module_version, ctx.module_size,
                               ctx.countReceived(), ctx.expected_blocks,
                               ctx.isComplete() ? "COMPLETE" : "PENDING")
            << std::endl;
    }
}
