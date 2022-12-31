//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Evaluate metrics on TS processing speed.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsMonotonic.h"

namespace ts {
    //!
    //! TS processing speed metrics evaluation.
    //! @ingroup mpeg
    //!
    //! An instance of this class evaluates the reading or processing speed
    //! of a TS. This is different from the TS bitrate, as evaluated by the
    //! class PCRAnalyzer, when the TS is not a live one but a file for
    //! instance.
    //!
    //! This class is useful to perform actions at regular intervals, based
    //! on wall clock time, not TS clock. Instead of checking the system
    //! time at every packet, which is far from efficient, an application can
    //! predict an average number of packets to process before performing the
    //! repetitive action.
    //!
    //! Warning: The processing speed of the TS is typically not stable.
    //! Buffer sizes impact the latency for instance. As well as any external
    //! factor in the operating system. So, predictions on packet distance
    //! are only hints at best. This class shall be considered as an optimization
    //! on the number of times the system time is probed, not a precise prediction.
    //!
    //! Principle of operation:
    //! - We work on a monotonic clock and we count packets.
    //! - We compute an "current" processing bitrate, not an average bitrate
    //!   from the beginning.
    //! - The bitrate is computed over the last period of at least 2 seconds
    //!   and at least 2000 packets (default values).
    //! - This "current period" is in fact made of a sliding windows of 20
    //!   elementary interval. Each interval must extend over at least 100
    //!   packets and 100 milliseconds.
    //!
    class TSDUCKDLL TSSpeedMetrics
    {
        TS_NOCOPY(TSSpeedMetrics);
    public:
        //!
        //! Default minimum packets to accumulate per interval.
        //! See the description of the class.
        //!
        static const PacketCounter MIN_PACKET_PER_INTERVAL = 100;

        //!
        //! Default minimum number of nanoseconds per interval.
        //! See the description of the class.
        //!
        static const NanoSecond MIN_NANOSEC_PER_INTERVAL = 100 * NanoSecPerMilliSec;

        //!
        //! Default number of intervals in the sliding window of bitrate computation.
        //! See the description of the class.
        //!
        static const size_t INTERVAL_COUNT = 20;

        //!
        //! Constructor.
        //! @param [in] packets Minimum packets to accumulate per interval.
        //! @param [in] nanosecs Minimum number of nanoseconds per interval.
        //! @param [in] intervals Max number of sliding time intervals.
        //!
        TSSpeedMetrics(PacketCounter packets = MIN_PACKET_PER_INTERVAL,
                       NanoSecond nanosecs = MIN_NANOSEC_PER_INTERVAL,
                       size_t intervals = INTERVAL_COUNT);

        //!
        //! Start a new processing time session.
        //!
        void start();

        //!
        //! Report the processing of some TS packets by the application.
        //! @param [in] count Number of processed packets, one by default.
        //! @return True if we just fetched the value of the clock.
        //!
        bool processedPacket(PacketCounter count = 1);

        //!
        //! Get the evaluated processing bitrate in bits/second based on 188-byte packets.
        //! @return The evaluated TS bitrate in bits/second or zero if not available.
        //!
        BitRate bitrate() const;

        //!
        //! Get the duration of the session, since start(), in nanoseconds.
        //! The value is the most precise when processedPacket() just returned true.
        //! @return The duration of the session, since start(), in nanoseconds.
        //!
        NanoSecond sessionNanoSeconds() const { return _clock - _session_start; }

    private:
        // Results on an interval of time.
        struct Interval
        {
            Interval();
            void clear();

            PacketCounter packets;   // Number of processed packets.
            NanoSecond    duration;  // Processing duration.
        };

        // Configuration data:
        PacketCounter _min_packets;        // Minimum packets to accumulate per interval.
        NanoSecond    _min_nanosecs;       // Minimum number of nanoseconds per interval.
        size_t        _max_intervals_num;  // Max number of sliding time intervals.
        // Clocks:
        Monotonic     _session_start;      // The clock when start() was called.
        Monotonic     _clock;              // The reference clock.
        // Accumulated data since beginning of session:
        std::vector<Interval> _intervals;  // Accumulate results, circular buffer.
        size_t        _next_interval;      // Next interval to overwrite.
        Interval      _total;              // Accumulated values in all _intervals.
        // Description of current interval:
        NanoSecond    _start_interval;     // Start time of interval, from _start_session.
        PacketCounter _count_interval;     // Number of processed packets in current interval.
        PacketCounter _remain_interval;    // Number of packets to process in this interval before checking the clock.
    };
}
