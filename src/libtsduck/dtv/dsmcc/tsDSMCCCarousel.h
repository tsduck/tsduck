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
#include "tsDSMCCCompatibilityDescriptor.h"
#include <set>

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
        //! Render the status of current modules as a UString.
        //! @return One line per module (trailing newline), empty if no modules
        //! have been seen yet.
        //!
        UString listModules() const { return _assembler.listModules(); }

        //!
        //! Snapshot of all known modules, keyed by (download_id, module_id).
        //! @return The internal module map.
        //!
        const std::map<DSMCCModuleAssembler::ModuleKey, DSMCCModuleAssembler::ModuleContext>& modules() const { return _assembler.modules(); }

        //!
        //! Look up a module by (download_id, module_id).
        //! @param [in] download_id The download_id.
        //! @param [in] module_id The module_id.
        //! @return Pointer to the module context, or nullptr if not found.
        //!
        const DSMCCModuleAssembler::ModuleContext* module(uint32_t download_id, uint16_t module_id) const { return _assembler.module(download_id, module_id); }

        //!
        //! Per-group bookkeeping for the carousel. One entry per `download_id`
        //! observed in DDB headers (which equals `group_id` from the DSI's
        //! GroupInfoIndication in data-carousel mode). Object carousels
        //! produce a single synthesized group on first DII.
        //!
        struct GroupContext {
            uint32_t download_id = 0;                       //!< DDB download_id; matches GroupInfoIndication group_id in data-carousel mode.
            uint32_t group_size = 0;                        //!< Size in bytes from the DSI (0 when no DSI announces this group).
            bool announced_by_dsi = false;                  //!< True if the group was announced by a DSI's GroupInfoIndication.
            DSMCCCompatibilityDescriptor compatibility {};  //!< Per-group compatibilityDescriptor (data-carousel only).
            std::set<uint16_t> module_ids {};               //!< module_ids announced via DII for this group.
            size_t modules_complete = 0;                    //!< Modules in COMPLETE state.

            //!
            //! @return True iff at least one module is known and all known
            //! modules have reached COMPLETE.
            //!
            bool isComplete() const { return !module_ids.empty() && modules_complete == module_ids.size(); }
        };

        //!
        //! Snapshot of all known groups, keyed by download_id.
        //! @return The internal group map.
        //!
        const std::map<uint32_t, GroupContext>& groups() const { return _groups; }

        //!
        //! Render the status of current groups as a UString. One line per
        //! group; empty if no DSI/DII has been seen yet.
        //! @return Multi-line string, trailing newline per line.
        //!
        UString listGroups() const;

        //!
        //! Flush any BIOP objects that were buffered while waiting for their parent
        //! directory to be parsed. Objects whose names still cannot be resolved are
        //! emitted with an empty name.
        //!
        void flushPendingObjects();

        //!
        //! Callback type for module completion events.
        //! Parameters: `download_id` (maps to the carousel group in
        //! data-carousel mode), module id, decompressed payload.
        //!
        using ModuleHandler = std::function<void(uint32_t download_id, uint16_t module_id, const ByteBlock& payload)>;

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
        //! Callback type for group completion events.
        //! Fires exactly once per group when all known modules reach COMPLETE.
        //! Parameters: the completed GroupContext (by const ref).
        //!
        using GroupHandler = std::function<void(const GroupContext&)>;

        //!
        //! Set a callback to be invoked when all known modules in a group complete.
        //! @param [in] handler The callback function.
        //!
        void setGroupCompletedHandler(GroupHandler handler) { _on_group = std::move(handler); }

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
        GroupHandler _on_group = nullptr;
        BIOPNameResolver _names {};
        bool _scan_biop = true;

        std::map<uint32_t, GroupContext> _groups {};

        void onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx);
        void onAssemblerModuleDiscovered(uint32_t download_id, uint16_t module_id);
        void scanBIOPObjects(uint16_t module_id, const ByteBlock& payload);
        void bootstrapFromDSI(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi);
        void recordDSIGroups(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi);
    };
}  // namespace ts
