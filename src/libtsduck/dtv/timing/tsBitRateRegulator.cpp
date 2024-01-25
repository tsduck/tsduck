//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBitRateRegulator.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BitRateRegulator::BitRateRegulator(Report* report, int log_level) :
    _report(report == nullptr ? &NULLREP : report),
    _log_level(log_level)
{
}


//----------------------------------------------------------------------------
// Set a new report.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::setReport(ts::Report *report, int log_level)
{
    _report = report == nullptr ? &NULLREP : report;
    _log_level = log_level;
}


//----------------------------------------------------------------------------
// Start regulation, initialize all timers.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::start()
{
    // Compute the minimum delay between two bursts, in nano-seconds. This is a
    // limitation of the operating system. If we try to use wait on durations
    // lower than the minimum, this will introduce latencies which mess up the
    // regulation. We try to request 2 milliseconds as time precision and we
    // keep what the operating system gives.
    _burst_min = cn::milliseconds(2);
    SetTimersPrecision(_burst_min);
    _report->log(_log_level, u"minimum packet burst duration is %s", {UString::Chrono(_burst_min)});

    // Initial measurement period is one second. Will be enlarged for extra-low bitrates.
    _period_duration = cn::seconds(1);

    // Reset state
    _starting = true;
    _regulated = false;
    _burst_duration = cn::nanoseconds::zero();
    _cur_bitrate = 0;
    _cur_period = 0;
}


//----------------------------------------------------------------------------
// Handle bitrate change, compute burst duration.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::handleNewBitrate()
{
    assert(_cur_bitrate > 0);

    // Compute the number of packets per burst. Use the packets/burst from the command line or 1 by default.
    PacketCounter burst_pkt_max = _opt_burst == 0 ? 1 : _opt_burst;

    // Compute corresponding duration (in nano-seconds) between two bursts.
    assert(_cur_bitrate > 0);
    _burst_duration = PacketInterval<cn::nanoseconds>(_cur_bitrate, burst_pkt_max);

    // If the result is too small for the time precision of the operating system, recompute a larger burst duration.
    if (_burst_duration < _burst_min) {
        _burst_duration = _burst_min;
        burst_pkt_max = PacketDistance(_cur_bitrate, _burst_duration);
    }

    // New end of burst sequence.
    _burst_end = monotonic_time::clock::now() + _burst_duration;

    // Measurement period is one second by default but must be larger than 2 bursts.
    _period_duration = std::max(cn::nanoseconds(cn::seconds(1)), 2 * _burst_duration);

    _report->debug(u"new regulation, burst: %'d nano-seconds, %'d packets, measurement period: %'d nano-seconds", {_burst_duration.count(), burst_pkt_max, _period_duration.count()});
}


//----------------------------------------------------------------------------
// Process one packet in a regulated burst. Wait at end of burst.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::regulatePacket(bool& flush)
{
    // Total measurement period.
    monotonic_time now = monotonic_time::clock::now();
    cn::nanoseconds duration = cn::duration_cast<cn::nanoseconds>(now - otherPeriod().start);

    // Allowed bits in the total measurement period.
    int64_t max_bits = ((_cur_bitrate * duration.count()) / NanoSecPerSec).toInt();

    // While not enough bit credit for one packet, wait until end of current burst.
    while (otherPeriod().bits + currentPeriod().bits + int64_t(PKT_SIZE_BITS) > max_bits) {
        // Wait until scheduled end of burst.
        std::this_thread::sleep_until(_burst_end);
        // Restart a new burst, use monotonic time.
        _burst_end += _burst_duration;
        // Flush current burst
        flush = true;
        // Update measurement period and bit credit.
        now = monotonic_time::clock::now();
        duration = cn::duration_cast<cn::nanoseconds>(now - otherPeriod().start);
        max_bits = ((_cur_bitrate * duration.count()) / NanoSecPerSec).toInt();
    }

    // Switch measurement period when necessary.
    if (now - currentPeriod().start >= _period_duration) {
        // The "other" period will disappear.
        // Credit unused bits from the other period to the current period.
        cn::nanoseconds cur_duration = cn::duration_cast<cn::nanoseconds>(currentPeriod().start - otherPeriod().start);
        currentPeriod().bits -= ((_cur_bitrate * cur_duration.count()) / NanoSecPerSec).toInt() - otherPeriod().bits;
        // Current period becomes the other period.
        _cur_period ^= 1;
        // Reset the new current period.
        currentPeriod().start = now;
        currentPeriod().bits = 0;
    }

    // One more regulated packet at this bitrate.
    currentPeriod().bits += PKT_SIZE_BITS;
}


//----------------------------------------------------------------------------
// Regulate the flow, to be called at each packet.
// This version is suitable for fixed bitrate.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::regulate()
{
    bool flush = false;
    bool bitrate_changed = false;
    regulate(0, flush, bitrate_changed);
}


//----------------------------------------------------------------------------
// Regulate the flow, to be called at each packet.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::regulate(const BitRate& current_bitrate, bool& flush, bool& bitrate_changed)
{
    // Output parameters.
    flush = bitrate_changed = false;

    // Compute old and new bitrate (most often the same)
    BitRate old_bitrate = _cur_bitrate;
    _cur_bitrate = _opt_bitrate != 0 ? _opt_bitrate : current_bitrate;

    // Report initial or changed regulation state.
    if (_cur_bitrate != old_bitrate || _starting) {
        if (_cur_bitrate == 0) {
            _report->log(_log_level, u"unknown bitrate, cannot regulate.");
        }
        else {
            _report->log(_log_level, u"regulated at bitrate %'d b/s", {_cur_bitrate.toInt()});
        }
    }
    _starting = false;

    // Perform regulation.
    if (_regulated) {
        // We previously had a bitrate and we regulated the flow.
        if (_cur_bitrate == 0) {
            // No more bitrate, become unregulated
            _regulated = false;
        }
        else if (_cur_bitrate == old_bitrate) {
            // Still the same bitrate, continue to regulate.
            regulatePacket(flush);
        }
        else {
            // Got a new non-zero bitrate. Compute new burst.
            // Compute new burst duration, based on new bitrate
            handleNewBitrate();
            bitrate_changed = true;
            regulatePacket(flush);
        }
    }
    else {
        // We had no bitrate, we did not regulate.
        if (_cur_bitrate > 0) {
            // Got a non-zero bitrate -> start regulation.
            _regulated = true;
            // Start measurement of packets.
            otherPeriod().start = monotonic_time::clock::now();
            currentPeriod().start = otherPeriod().start;
            otherPeriod().bits = currentPeriod().bits = 0;
            // Setup burst duration.
            handleNewBitrate();
            bitrate_changed = true;
            regulatePacket(flush);
        }
    }
}
