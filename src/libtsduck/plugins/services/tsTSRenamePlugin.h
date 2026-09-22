//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to rename the transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsAbstractTransportListTable.h"
#include "tsEITProcessor.h"
#include "tsPAT.h"
#include "tsSDT.h"

namespace ts {
    //!
    //! Plugin to rename the transport stream.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TSRenamePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(TSRenamePlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool              _abort = false;        // Error (service not found, etc)
        bool              _ready = false;        // Ready to pass packets
        PID               _nit_pid = PID_NIT;    // PID for the NIT
        uint16_t          _old_ts_id = 0;        // Old transport stream id
        bool              _set_ts_id = false;    // Modify transport stream id
        uint16_t          _new_ts_id = 0;        // New transport stream id
        bool              _set_onet_id = false;  // Update original network id
        uint16_t          _new_onet_id = 0;      // New original network id
        bool              _ignore_bat = false;   // Do not modify the BAT
        bool              _ignore_eit = false;   // Do not modify the EIT's
        bool              _ignore_nit = false;   // Do not modify the NIT
        bool              _add_bat = false;      // Add a new TS entry in the BAT instead of replacing
        bool              _add_nit = false;      // Add a new TS entry in the NIT instead of replacing
        SectionDemux      _demux {duck, this};
        CyclingPacketizer _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_sdt_bat {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer _pzer_nit {duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor      _eit_process {duck, PID_EIT};

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables and descriptors
        void processPAT(PAT&);
        void processSDT(SDT&);
        void processNITBAT(AbstractTransportListTable&, bool);
    };
}
