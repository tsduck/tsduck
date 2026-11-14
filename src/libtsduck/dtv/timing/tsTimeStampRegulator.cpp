//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeStampRegulator.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TimeStampRegulator::~TimeStampRegulator()
{
}


//----------------------------------------------------------------------------
// Re-initialize state.
//----------------------------------------------------------------------------

bool ts::TimeStampRegulator::reset()
{
    _burst_pkt_cnt = 0;
    _started = _no_timestamp = false;
    return true;
}


//----------------------------------------------------------------------------
// Regulate the flow, to be called at each packet.
//----------------------------------------------------------------------------

void ts::TimeStampRegulator::regulate(TSPacket& pkt, TSPacketMetadata& mdata, const BitRate& current_bitrate)
{
    bool flush = false;

    if (mdata.hasInputTimeStamp()) {
        // Timestamp value.
        const PCR timestamp = mdata.getInputTimeStamp();

        // Detect timestamp wrap-down. Even if timestamps use PCR units, they rarely are true PCR values.
        // Therefore, there is no reason that they wrap at PCR scale. The may wrap at any time, or not wrap at all.
        // When a timestamp is lower than the previous one, there was a wrap-down but we don't know where it
        // wrapped and we can't compute the actual interval. In that case, we simply reset the regulation.
        if (_started && timestamp < _last_timestamp) {
            report().verbose(u"timestamp values wrapped down, restarting regulation");
            _started = false;
        }

        if (!_started) {
            // Initialize regulation at the first timestamp.
            _started = true;
            _first_clock = _last_clock = monotonic_time::clock::now();
            _first_timestamp = _last_timestamp = timestamp;

            // Compute minimum wait if none is set.
            if (_minimum_wait <= cn::microseconds::zero()) {
                setMinimimWait(DEFAULT_MIN_WAIT);
            }
        }
        else {
            // Compute the number of PCR units since the first timestamp.
            const PCR duration = timestamp - _first_timestamp;

            // Compute due system clock, the expected system time for this PCR.
            const monotonic_time clock_due(_first_clock + cn::duration_cast<monotonic_time::duration>(duration));

            // Do not wait less than the user-specified minimum.
            if (clock_due - _last_clock >= _minimum_wait) {
                // Wait until system time for current timestamp.
                _last_clock = clock_due;
                std::this_thread::sleep_until(_last_clock);
                // Always flush after wait.
                flush = true;
            }
        }

        // Always keep last timestamp value.
        _last_timestamp = timestamp;
    }
    else if (!_no_timestamp) {
        // On packets without timestamp, display a warning once only.
        _no_timestamp = true;
        report().warning(u"found packets without input timestamps, regulation may not work");
    }

    // One more packet in current burst.
    if (++_burst_pkt_cnt >= _requested_burst) {
        flush = true;
    }

    // Reset packet counter at end of each burst.
    if (flush) {
        _burst_pkt_cnt = 0;
    }

    // Set when packets should be flushed to next plugin.
    mdata.setFlush(flush);
}
