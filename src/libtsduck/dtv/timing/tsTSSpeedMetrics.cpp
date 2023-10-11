//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSSpeedMetrics.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TSSpeedMetrics::TSSpeedMetrics(PacketCounter packets, NanoSecond nanosecs, size_t intervals) :
    _min_packets(packets),
    _min_nanosecs(nanosecs),
    _max_intervals_num(intervals)
{
    start();
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
