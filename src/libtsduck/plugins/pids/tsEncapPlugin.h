//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to encapsulate TS packets from several PID's into one single PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsPacketEncapsulation.h"

namespace ts {
    //!
    //! Plugin to encapsulate TS packets from several PID's into one single PID.
    //! @see DecapPlugin
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL EncapPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(EncapPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool                         _ignore_errors = false;  // Ignore encapsulation errors.
        bool                         _pack = false;           // Outer packet packing option.
        bool                         _drop_initial = false;   // Drop initial input packet before the first PCR.
        size_t                       _pack_limit = 0;         // Max limit distance.
        size_t                       _max_buffered = 0;       // Max buffered packets.
        PID                          _output_pid = PID_NULL;  // Output PID.
        PID                          _pcr_pid = PID_NULL;     // PCR reference PID.
        size_t                       _pcr_label = NPOS;       // PCR reference label.
        PIDSet                       _input_pids {};          // Input PID's.
        TSPacketLabelSet             _input_labels {};        // Input packet labels.
        PacketEncapsulation::PESMode _pes_mode = PacketEncapsulation::DISABLED;
        int32_t                      _pes_offset = 0;         // Offset value in PES Synchronous.
        PacketEncapsulation          _encap {*this};          // Encapsulation engine.
    };
}
