//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC Object Carousel (library entry point).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDuckContext.h"
#include "tsDSMCCModuleAssembler.h"

namespace ts {

    //!
    //! DSM-CC Object Carousel library facade.
    //!
    //! Demux-agnostic entry point for extracting the content of a DSM-CC Object Carousel.
    //! Callers feed already-parsed DSM-CC messages; the carousel assembles modules,
    //! decompresses them when required, and invokes a callback with the final payload.
    //!
    class TSDUCKDLL DSMCCCarousel
    {
        TS_NOCOPY(DSMCCCarousel);

    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        explicit DSMCCCarousel(DuckContext& duck);

        //!
        //! Clear carousel state.
        //!
        void clear();

        //!
        //! Feed a DSM-CC User-to-Network Message (DSI or DII).
        //! @param [in] unm The parsed message.
        //!
        void feedUserToNetwork(const DSMCCUserToNetworkMessage& unm) { _assembler.feedUserToNetwork(unm); }

        //!
        //! Feed a DSM-CC Download Data Message (DDB).
        //! @param [in,out] ddm The parsed message. Payload may be moved out.
        //!
        void feedDownloadData(DSMCCDownloadDataMessage& ddm) { _assembler.feedDownloadData(ddm); }

        //!
        //! List the status of current modules to an output stream.
        //! @param [in,out] out Output stream.
        //!
        void listModules(std::ostream& out) const { _assembler.listModules(out); }

        //!
        //! Callback type for module completion events.
        //! Parameters: module id, decompressed payload.
        //!
        using ModuleHandler = std::function<void(uint16_t module_id, const ByteBlock& payload)>;

        //!
        //! Set a callback to be invoked when a module is fully loaded and decompressed.
        //! @param [in] handler The callback function.
        //!
        void setModuleCompletedHandler(ModuleHandler handler) { _on_module = std::move(handler); }

    private:
        DuckContext& _duck;
        DSMCCModuleAssembler _assembler;
        ModuleHandler _on_module = nullptr;

        void onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx);
    };
}  // namespace ts
