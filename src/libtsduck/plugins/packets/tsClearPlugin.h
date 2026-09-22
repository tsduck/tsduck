//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Clear packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsTOT.h"

namespace ts {
    //!
    //! Clear packet processor plugin for tsp.
    //! Extract clear (non scrambled) sequence of a transport stream
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL ClearPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ClearPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool          _abort = false;         // Error (service not found, etc)
        bool          _pass_packets = false;  // Pass packets trigger
        bool          _video_only = false;    // Check video PIDs only
        bool          _audio_only = false;    // Check audio PIDs only
        Service       _service {};            // Service name & id
        TOT           _last_tot {};           // Last received TOT
        PacketCounter _drop_after = 0;        // Number of packets after last clear
        PacketCounter _last_clear_pkt = 0;    // Last clear packet number
        PIDSet        _clear_pids {};         // List of PIDs to check for clear packets
        SectionDemux  _demux {duck, this};    // Section demux
        PacketProcessStatus _drop_status = TSP_OK;  // Status for dropped packets

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);
    };
}
