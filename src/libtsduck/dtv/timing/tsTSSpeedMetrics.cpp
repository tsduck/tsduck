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

#include "tsTSSpeedMetrics.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TSSpeedMetrics::TSSpeedMetrics(PacketCounter packets, NanoSecond nanosecs, size_t intervals) :
    _min_packets(packets),
    _min_nanosecs(nanosecs),
    _max_intervals_num(intervals),
    _session_start(),
    _clock(),
    _intervals(),
    _next_interval(0),
    _total(),
    _start_interval(0),
    _count_interval(0),
    _remain_interval(0)
{
    start();
}


//----------------------------------------------------------------------------
// Results on an interval of time.
//----------------------------------------------------------------------------

ts::TSSpeedMetrics::Interval::Interval() :
    packets(0),
    duration(0)
{
}

void ts::TSSpeedMetrics::Interval::clear()
{
    packets = 0;
    duration = 0;
}


//----------------------------------------------------------------------------
// Start a new processing time session.
//----------------------------------------------------------------------------

void ts::TSSpeedMetrics::start()
{
    // Reset the content of all intervals to {0, 0}
    _intervals.clear();
    _intervals.resize(_max_intervals_num);
    _next_interval = 0;

    // Reset accumulated counters.
    _total.clear();

    // Get initial time reference.
    _session_start.getSystemTime();
    _clock = _session_start;

    // Initialize first interval.
    _start_interval = 0;
    _count_interval = 0;
    _remain_interval = _min_packets;
}


//----------------------------------------------------------------------------
// Report the processing of TS packets.
//----------------------------------------------------------------------------

bool ts::TSSpeedMetrics::processedPacket(PacketCounter count)
{
    // Accumulate in current interval.
    _count_interval += count;
    _remain_interval -= std::min(_remain_interval, count);

    // Is it time to reconsider the clock ?
    const bool get_clock = _remain_interval == 0;

    if (get_clock) {
        // Yes, fetch system clock.
        _clock.getSystemTime();
        const NanoSecond in_session = _clock - _session_start;
        const NanoSecond in_interval = in_session - _start_interval;

        if (in_interval < _min_nanosecs) {
            // Not enough time for an interval, precision would be affected.
            // Add more packets to the current interval.
            _remain_interval = std::max<PacketCounter>(1, _min_packets / 2);
        }
        else {
            // Enough data for this interval. Add it into accumulated data.
            // First, remove from total the data of the interval we are going to overwrite.
            assert(_next_interval < _intervals.size());
            assert(_total.duration >= _intervals[_next_interval].duration);
            assert(_total.packets >= _intervals[_next_interval].packets);
            _total.duration -= _intervals[_next_interval].duration;
            _total.packets -= _intervals[_next_interval].packets;

            // Then, add current data in the accumulated data.
            _intervals[_next_interval].packets = _count_interval;
            _intervals[_next_interval].duration = in_interval;
            _total.packets += _count_interval;
            _total.duration += in_interval;

            // Next entry to overwrite in circular buffer.
            _next_interval = (_next_interval + 1) % _intervals.size();

            // Initialize next interval (_remain_interval is already zero).
            _start_interval = in_session;
            _count_interval = 0;
        }
    }

    return get_clock;
}


//----------------------------------------------------------------------------
// Return the evaluated processing bitrate in bits/second
//----------------------------------------------------------------------------

ts::BitRate ts::TSSpeedMetrics::bitrate() const
{
    return _total.duration == 0 ? 0 : BitRate(_total.packets * PKT_SIZE_BITS * NanoSecPerSec) / _total.duration;
}
