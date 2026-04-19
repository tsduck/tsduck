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
        //! List the status of current modules to an output stream.
        //! @param [in,out] out Output stream.
        //!
        void listModules(std::ostream& out) const;

        //!
        //! Context for a single Module in the carousel.
        //!
        struct ModuleContext {
            uint16_t module_id = 0;
            uint32_t module_size = 0;
            uint8_t module_version = 0;
            uint16_t block_size = 4066;

            size_t expected_blocks = 0;  // Calculated from module_size and block_size
            bool is_compressed = false;  // Compressed_module_descriptor detected in DII
            uint32_t original_size = 0;  // Original size from compressed_module_descriptor
            ByteBlock payload {};        // Complete module payload assembled from DDBs

            enum class Status { UNKNOWN,
                                PENDING,
                                COMPLETE };
            Status status = Status::UNKNOWN;

            //!
            //! Initialize module with expected size and block size.
            //!
            void setSize(uint32_t size, uint16_t blk_size);

            //!
            //! Check if module is fully loaded.
            //!
            bool isComplete() const { return status == Status::COMPLETE; }
        };

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

    private:
        ModuleHandler _on_module_complete = nullptr;

        DuckContext& _duck;

        // Module Tracking
        std::map<uint16_t, ModuleContext> _modules {};

        // DDB payload received before the DII that announces the module.
        // Keyed by module_id; each entry holds the version and the payload to
        // replay once the DII arrives.
        struct OrphanBlock {
            uint8_t   module_version = 0;
            ByteBlock block_data {};
        };
        std::map<uint16_t, std::vector<OrphanBlock>> _orphan_ddbs {};

        void processDII(const DSMCCUserToNetworkMessage& unm);
        void processDDB(DSMCCDownloadDataMessage& ddm);

        // Apply any buffered DDB whose version matches the freshly registered module.
        void replayOrphans(ModuleContext& ctx);
    };
}  // namespace ts
