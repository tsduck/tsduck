//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to remove orphan PID's (not referenced in any table).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! Plugin to remove orphan PID's (not referenced in any table).
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL RMOrphanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(RMOrphanPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PacketProcessStatus _drop_status = TSP_DROP;  // Status for dropped packets
        PIDSet              _pass_pids {};            // List of PIDs to pass
        SectionDemux        _demux {duck, this};      // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Reference a PID or a list of predefined PID's.
        void passPID(PID pid);
        void passPredefinedPIDs(Standards standards, PID first, PID last);

        // Adds all ECM/EMM PIDs from the specified descriptor list.
        void addCA(const DescriptorList& dlist, TID parent_table);
    };
}
