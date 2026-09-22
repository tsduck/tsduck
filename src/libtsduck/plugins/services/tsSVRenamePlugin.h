//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to rename a service.
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
    //! Plugin to rename a service.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SVRenamePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SVRenamePlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool              _abort = false;      // Error (service not found, etc)
        bool              _pat_found = false;  // PAT was found, ready to pass packets
        uint16_t          _ts_id = 0;          // Tranport stream id
        Service           _new_service {};     // New service name & id
        Service           _old_service {};     // Old service name & id
        bool              _ignore_bat = false; // Do not modify the BAT
        bool              _ignore_eit = false; // Do not modify the EIT's
        bool              _ignore_nit = false; // Do not modify the NIT
        SectionDemux      _demux {duck, this};
        CyclingPacketizer _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_pmt {duck, PID_NULL, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_sdt_bat {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_nit {duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor      _eit_process {duck, PID_EIT};

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables and descriptors
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);
        void processNITBAT(AbstractTransportListTable&);
        void processNITBATDescriptorList(DescriptorList&);
    };
}
