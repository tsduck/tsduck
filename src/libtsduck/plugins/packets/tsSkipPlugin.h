//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to skip TS packets until a specified condition is met.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSClock.h"

namespace ts {
    //!
    //! Plugin to skip TS packets until a specified condition is met.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SkipPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SkipPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _use_stuffing = false;
        PacketCounter    _skip_packets = 0;
        PacketCounter    _skip_unit_start = 0;
        PacketCounter    _skip_null_seq = 0;
        cn::milliseconds _skip_msec {};
        TSClockArgs      _ts_clock_args {};

        // Working data:
        bool             _started = false;          // Condition is met, pass packets
        PID              _previous_pid = PID_NULL;  // PID of previous packet
        PacketCounter    _unit_start_cnt = 0;       // Payload unit start counter
        PacketCounter    _null_seq_cnt = 0;         // Sequence of null packets counter
        TSClock          _ts_clock {duck};          // Compute playout time
    };
}
