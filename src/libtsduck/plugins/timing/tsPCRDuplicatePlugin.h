//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to duplicate PCR values from a PID into a new PCR-only PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to duplicate PCR values from a PID into a new PCR-only PID.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCRDuplicatePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRDuplicatePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        PID           _ref_pid_arg = PID_NULL;  // Reference PCR source.
        size_t        _ref_label = NPOS;        // Label which indicates the reference PID.
        PID           _new_pid = PID_NULL;      // New PID to create.

        // Working data.
        bool          _pending_pcr = false;     // Insert a new PCR when possible.
        bool          _pid_conflict = false;    // New PID alread found on input.
        PID           _ref_pid = PID_NULL;      // Current reference PCR source.
        PacketCounter _ref_packet = 0;          // Packet index of last PCR in reference PID.
        uint64_t      _ref_pcr = INVALID_PCR;   // Last PCR value in reference PID.
        PacketCounter _total_pcr = 0;           // Number of PCR's in input PID.
        PacketCounter _missed_pcr = 0;          // Number of input PCR's not duplicated in output PID.
    };
}
