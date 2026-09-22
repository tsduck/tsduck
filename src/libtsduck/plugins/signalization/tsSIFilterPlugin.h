//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract PID's containing PSI/SI.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsCASSelectionArgs.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"

namespace ts {
    //!
    //! Plugin to extract PID's containing PSI/SI.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SIFilterPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SIFilterPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        CASSelectionArgs    _cas_args {};            // CAS selection
        bool                _pass_pmt = false;       // Pass PIDs containing PMT
        PacketProcessStatus _drop_status = TSP_DROP; // Status for dropped packets
        PIDSet              _pass_pids {};           // List of PIDs to pass
        SectionDemux        _demux {duck, this};     // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(const PAT&);
    };
}
