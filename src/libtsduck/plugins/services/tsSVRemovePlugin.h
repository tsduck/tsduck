//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to remove a service.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsAbstractTransportListTable.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"

namespace ts {
    //!
    //! Plugin to remove a service.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SVRemovePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SVRemovePlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool                _abort = false;          // Error (service not found, etc)
        bool                _ready = false;          // Ready to pass packets
        bool                _transparent = false;    // Transparent mode, pass all packets
        Service             _service {};             // Service name & id
        bool                _ignore_absent = false;  // Ignore service if absent
        bool                _ignore_bat = false;     // Do not modify the BAT
        bool                _ignore_eit = false;     // Do not modify the EIT's
        bool                _ignore_nit = false;     // Do not modify the NIT
        PacketProcessStatus _drop_status = TSP_DROP; // Status for dropped packets
        PIDSet              _drop_pids {};           // List of PIDs to drop
        PIDSet              _ref_pids {};            // List of other referenced PIDs
        SectionDemux        _demux {duck, this};     // Section demux
        CyclingPacketizer   _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer   _pzer_sdt_bat {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer   _pzer_nit {duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor        _eit_process {duck, PID_EIT};

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables and descriptors
        void processPAT(PAT&);
        void processSDT(SDT&);
        void processPMT(PMT&);
        void processNITBAT(AbstractTransportListTable&);
        void processNITBATDescriptorList(DescriptorList&);

        // Mark all ECM PIDs from the specified descriptor list in the specified PID set
        void addECMPID(const DescriptorList&, PIDSet&);
    };
}
