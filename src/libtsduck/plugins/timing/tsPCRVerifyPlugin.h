//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to verify PCR values.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to verify PCR values.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCRVerifyPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRVerifyPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext() = default;
            uint64_t      pcr_value = INVALID_PCR;                 // Last PCR value in this PID.
            PacketCounter pcr_packet = 0;                          // Packet index containing last PCR.
            uint64_t      pcr_timestamp = INVALID_PCR;             // Input timestamp of packet containing last PCR (or INVALID _PCR).
            TimeSource    pcr_timesource = TimeSource::UNDEFINED;  // Source of input time stamp.
        };

        // Command line options.
        bool    _absolute = false;     // Use PCR absolute value, not micro-second
        bool    _input_synch = false;  // Use input-synchronous verification, base on input timestamps
        BitRate _bitrate = 0;          // Expected bitrate (0 if unknown)
        int64_t _jitter_max = 0;       // Max accepted jitter in PCR units
        int64_t _jitter_unreal = 0;    // Max realistic jitter
        bool    _time_stamp = false;   // Display time stamps
        PIDSet  _pid_list {};          // Array of pid values to filter

        // Working data.
        PacketCounter            _nb_pcr_ok = 0;         // Number of PCR without jitter
        PacketCounter            _nb_pcr_nok = 0;        // Number of PCR with jitter
        PacketCounter            _nb_pcr_unchecked = 0;  // Number of unchecked PCR (no previous ref)
        std::map<PID,PIDContext> _stats {};              // Per-PID statistics

        // PCR units per micro-second.
        static constexpr int64_t PCR_PER_MICRO_SEC = int64_t(PCRTraits::TICKS) / cn::microseconds::period::den;
        static constexpr int64_t DEFAULT_JITTER_MAX_US = 1000; // 1000 us = 1 ms
        static constexpr int64_t DEFAULT_JITTER_UNREAL_US = 10 * cn::microseconds::period::den; // 10 seconds
        static constexpr int64_t DEFAULT_JITTER_MAX = DEFAULT_JITTER_MAX_US * PCR_PER_MICRO_SEC;
        static constexpr int64_t DEFAULT_JITTER_UNREAL = DEFAULT_JITTER_UNREAL_US * PCR_PER_MICRO_SEC;
    };
}
