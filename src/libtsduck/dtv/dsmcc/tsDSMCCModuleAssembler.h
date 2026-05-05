//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC Object Carousel Module Assembler (FSM).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDuckContext.h"
#include "tsDSMCC.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsDescriptorList.h"
#include <functional>
#include <map>

namespace ts {

    //!
    //! Assembles DSM-CC Object Carousel modules from DSI/DII/DDB messages.
    //!
    //! Demux-agnostic: callers feed already-parsed DSM-CC messages via feedUserToNetwork()
    //! and feedDownloadData(). A callback is invoked when a module is fully assembled.
    //!
    class TSDUCKDLL DSMCCModuleAssembler
    {
        TS_NOCOPY(DSMCCModuleAssembler);

    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        explicit DSMCCModuleAssembler(DuckContext& duck);

        //!
        //! Clear the state of the assembler.
        //!
        void clear();

        //!
        //! Feed a DSM-CC User-to-Network Message (DSI or DII).
        //! @param [in] unm The parsed message.
        //!
        void feedUserToNetwork(const DSMCCUserToNetworkMessage& unm);

        //!
        //! Feed a DSM-CC Download Data Message (DDB).
        //! @param [in,out] ddm The parsed message. Payload may be moved out.
        //!
        void feedDownloadData(DSMCCDownloadDataMessage& ddm);

        //!
        //! Default block_size when the DII announces 0 (the spec forbids it but malformed
        //! carousels show up and we must not divide by zero).
        //!
        static constexpr uint16_t DEFAULT_BLOCK_SIZE = 4066;

        //!
        //! Composite key used internally for tracking modules.
        //!
        using ModuleKey = std::pair<uint32_t, uint16_t>;

        //!
        //! Context for a single Module in the carousel.
        //!
        struct ModuleContext {
            uint32_t download_id = 0;     //!< download_id from the DII; maps to the carousel group in data-carousel mode.
            uint16_t module_id = 0;       //!< Module identifier from the DII (only unique within a download_id).
            uint32_t module_size = 0;     //!< Total module size in bytes.
            uint8_t module_version = 0;   //!< Module version, incremented by the broadcaster on update.
            uint16_t block_size = DEFAULT_BLOCK_SIZE;  //!< Block size in bytes, announced by the DII.

            size_t expected_blocks = 0;     //!< Number of DDB blocks expected (ceil(module_size / block_size)).
            bool is_compressed = false;     //!< True if a compressed_module_descriptor is present.
            uint32_t original_size = 0;     //!< Uncompressed size from compressed_module_descriptor.
            ByteBlock payload {};           //!< Complete module payload assembled from DDBs.
            DescriptorList descs {nullptr}; //!< Copy of the DII module's user_info descriptor list. Decoded by consumers on demand.

            //!
            //! Completion status of this module.
            //!
            enum class Status {
                UNKNOWN,    //!< No information yet (no DII and no block seen).
                PENDING,    //!< DII seen, still collecting blocks.
                COMPLETE    //!< All blocks received and (if needed) decompressed.
            };
            Status status = Status::UNKNOWN;  //!< Current status of the module.

            //!
            //! Initialize module with expected size and block size.
            //! @param [in] size Total module size in bytes.
            //! @param [in] blk_size Block size in bytes.
            //!
            void setSize(uint32_t size, uint16_t blk_size);

            //!
            //! Check if module is fully loaded.
            //! @return True if the module is complete.
            //!
            bool isComplete() const { return status == Status::COMPLETE; }
        };

        //!
        //! Snapshot of all known modules, keyed by (download_id, module_id).
        //! @return The internal module map.
        //!
        const std::map<ModuleKey, ModuleContext>& modules() const { return _modules; }

        //!
        //! Look up a module by (download_id, module_id).
        //! @param [in] download_id The download_id.
        //! @param [in] module_id The module_id.
        //! @return Pointer to the module context, or nullptr if not found.
        //!
        const ModuleContext* module(uint32_t download_id, uint16_t module_id) const;

        //!
        //! Callback type for module completion events.
        //! Parameters: The completed ModuleContext.
        //!
        using ModuleHandler = std::function<void(const ModuleContext&)>;

        //!
        //! Set a callback to be invoked when a module is fully loaded.
        //! @param [in] handler The callback function.
        //!
        void setModuleCompletedHandler(ModuleHandler handler) { _on_module_complete = std::move(handler); }

        //!
        //! Callback type for module discovery events. Fired once per
        //! `(download_id, module_id)` pair when its DII is first parsed.
        //! Not fired on version bumps of an already-known module, nor on
        //! orphan-DDB replay (the discovery already happened when the DII
        //! that announced the module arrived).
        //!
        using DiscoveryHandler = std::function<void(uint32_t download_id, uint16_t module_id)>;

        //!
        //! Set a callback to be invoked when a new module is announced via DII.
        //! @param [in] handler The callback function.
        //!
        void setModuleDiscoveredHandler(DiscoveryHandler handler) { _on_module_discovered = std::move(handler); }

    private:
        ModuleHandler _on_module_complete = nullptr;
        DiscoveryHandler _on_module_discovered = nullptr;

        DuckContext& _duck;

        // (download_id, module_id) — module_id alone is only unique within a
        // DII group, so both tracking maps use the composite key. For
        // single-group streams download_id is effectively constant and this
        // behaves identically to the old single-key design. Type alias is
        // declared in the public section.

        // Module Tracking
        std::map<ModuleKey, ModuleContext> _modules {};

        // DDB payload received before the DII that announces the module.
        // Keyed by (download_id, module_id); each entry holds the version and
        // the payload to replay once the DII arrives.
        struct OrphanBlock {
            uint8_t   module_version = 0;
            ByteBlock block_data {};
        };
        std::map<ModuleKey, std::vector<OrphanBlock>> _orphan_ddbs {};

        void processDII(const DSMCCUserToNetworkMessage& unm);
        void processDDB(DSMCCDownloadDataMessage& ddm);

        // Apply any buffered DDB whose version matches the freshly registered module.
        void replayOrphans(ModuleContext& ctx);
    };
}  // namespace ts
