//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Jerome Leveque, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Monitor PID or TS bitrate plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSingleDataStatistics.h"
#include "tsjson.h"

namespace ts {
    //!
    //! Monitor PID or TS bitrate plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL BitrateMonitorPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(BitrateMonitorPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual bool handlePacketTimeout() override;

    private:
        // Default values
        static constexpr BitRate::int_t DEFAULT_BITRATE_MIN = 10;
        static constexpr BitRate::int_t DEFAULT_BITRATE_MAX = 0xFFFFFFFF;
        static constexpr size_t DEFAULT_TIME_WINDOW_SIZE = 5;

        // Type indicating status of current bitrate, regarding allowed range.
        enum RangeStatus {LOWER, IN_RANGE, GREATER};

        // Description of what is received during approximately one second.
        class Period
        {
        public:
            cn::nanoseconds duration {0};  // Actual duration in nanoseconds.
            PacketCounter   packets = 0;   // Total number of packets.
            PacketCounter   non_null = 0;  // Total number of non-null packets.

            // Constructor.
            Period() = default;

            // Clear content.
            void clear() { duration = cn::nanoseconds::zero(); packets = non_null = 0; }
        };

        // Command line options.
        bool             _full_ts = false;       // Monitor full TS.
        bool             _summary = false;       // Display a final summary.
        bool             _json_line = false;     // Use JSON log style.
        PID              _first_pid = PID_NULL;  // First monitored PID (for messages).
        size_t           _pid_count = 0;         // Number of PID's to monitor.
        PIDSet           _pids {};               // Monitored PID's.
        json::ValuePtr   _json_pids {};          // Monitored PID's in JSON format.
        UString          _tag {};                // Message tag.
        UString          _json_prefix {};        // Prefix before JSON line.
        BitRate          _min_bitrate = 0;       // Minimum allowed bitrate.
        BitRate          _max_bitrate = 0;       // Maximum allowed bitrate.
        cn::seconds      _periodic_bitrate {};   // Report bitrate at regular intervals, even if in range.
        cn::seconds      _periodic_command {};   // Run alarm command at regular intervals, even if in range.
        size_t           _window_size = 0;       // Size (in seconds) of the time window, used to compute bitrate.
        UString          _alarm_command {};      // Alarm command name.
        UString          _alarm_prefix {};       // Prefix for alarm messages.
        UString          _alarm_target {};       // "target" parameter to the alarm command.
        TSPacketLabelSet _labels_below {};       // Set these labels on all packets when bitrate is below normal.
        TSPacketLabelSet _labels_normal {};      // Set these labels on all packets when bitrate is normal.
        TSPacketLabelSet _labels_above {};       // Set these labels on all packets when bitrate is above normal.
        TSPacketLabelSet _labels_go_below {};    // Set these labels on one packet when bitrate goes below normal.
        TSPacketLabelSet _labels_go_normal {};   // Set these labels on one packet when bitrate goes back to normal.
        TSPacketLabelSet _labels_go_above {};    // Set these labels on one packet when bitrate goes above normal.

        // Working data.
        cn::seconds         _bitrate_countdown {};    // Countdown to report bitrate.
        cn::seconds         _command_countdown {};    // Countdown to run alarm command.
        RangeStatus         _last_bitrate_status = LOWER; // Status of the last bitrate, regarding allowed range.
        monotonic_time      _last_second {};          // System time at last measurement point.
        bool                _startup = false;         // Measurement in progress.
        size_t              _periods_index = 0;       // Index for packet number array.
        std::vector<Period> _periods {};              // Number of packets received during last time window, second per second.
        TSPacketLabelSet    _labels_next {};          // Set these labels on next packet.
        SingleDataStatistics<int64_t> _stats {};      // Bitrate statistics.
        SingleDataStatistics<int64_t> _net_stats {};  // Non-null bitrate statistics.

        // Compute bitrate. Report any alarm.
        void computeBitrate();

        // Check time and compute bitrate when necessary.
        void checkTime();

        // Add common JSON parts and log the message.
        void jsonLine(const UChar* status, int64_t bitrate, int64_t net_bitrate);
    };
}
