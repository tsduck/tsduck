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
#include "tsDSMCCBIOPMessage.h"
#include "tsDSMCCBIOPNameResolver.h"

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
        //! For a DSI, the carousel engine extracts the ServiceGateway location
        //! from the IOR and pre-seeds the name resolver so path resolution works
        //! as soon as the SRG module arrives.
        //! @param [in] unm The parsed message.
        //!
        void feedUserToNetwork(const DSMCCUserToNetworkMessage& unm);

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
        //! Flush any BIOP objects that were buffered while waiting for their parent
        //! directory to be parsed. Objects whose names still cannot be resolved are
        //! emitted with an empty name.
        //!
        void flushPendingObjects();

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

        //!
        //! Callback type for BIOP object events.
        //! Invoked once per BIOP message parsed from a completed module.
        //! Parameters: module id, resolved object path (joined NameComponents from
        //! the parent SRG/Directory bindings; empty if the parent has not been
        //! parsed yet), parsed BIOP message.
        //!
        using ObjectHandler = std::function<void(uint16_t module_id, const UString& name, const BIOPMessage& msg)>;

        //!
        //! Set a callback to be invoked for each BIOP object extracted from a module.
        //! @param [in] handler The callback function.
        //!
        void setObjectHandler(ObjectHandler handler) { _on_object = std::move(handler); }

        //!
        //! Enable or disable BIOP parsing of completed modules. When disabled, the
        //! module handler still fires but no object handler callbacks occur. Use
        //! this for plain data carousels (e.g. DVB-SSU) where module payloads are
        //! opaque and BIOP parsing would only produce spurious warnings.
        //! @param [in] enabled True to parse BIOP (default), false to skip.
        //!
        void setScanBIOP(bool enabled) { _scan_biop = enabled; }

    private:
        DuckContext& _duck;
        DSMCCModuleAssembler _assembler;
        ModuleHandler _on_module = nullptr;
        ObjectHandler _on_object = nullptr;
        BIOPNameResolver _names {};
        bool _scan_biop = true;

        void onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx);
        void scanBIOPObjects(uint16_t module_id, const ByteBlock& payload);
        void bootstrapFromDSI(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi);
    };
}  // namespace ts
