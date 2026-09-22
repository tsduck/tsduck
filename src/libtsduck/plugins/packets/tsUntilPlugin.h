//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to copy TS packets until a specified condition is met.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSClock.h"

namespace ts {
    //!
    //! Plugin to copy TS packets until a specified condition is met.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL UntilPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(UntilPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _exclude_last = false;
        PacketCounter    _packet_max = 0;
        PacketCounter    _unit_start_max = 0;
        PacketCounter    _null_seq_max = 0;
        cn::milliseconds _msec_max {};
        TSClockArgs      _ts_clock_args {};

        // Working data:
        PacketCounter    _unit_start_cnt = 0;       // Payload unit start counter
        PacketCounter    _null_seq_cnt = 0;         // Sequence of null packets counter
        PID              _previous_pid = PID_NULL;  // PID of previous packet
        bool             _terminated = false;       // Final condition is met
        bool             _transparent = false;      // Pass all packets, no longer check conditions
        TSClock          _ts_clock {duck};          // Compute playout time
    };
}
