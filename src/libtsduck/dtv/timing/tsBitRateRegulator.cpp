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

#include "tsBitRateRegulator.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BitRateRegulator::BitRateRegulator(Report* report, int log_level) :
    _report(report == nullptr ? NullReport::Instance() : report),
    _log_level(log_level),
    _starting(false),
    _regulated(false),
    _opt_burst(0),
    _opt_bitrate(0),
    _cur_bitrate(0),
    _burst_min(0),
    _burst_duration(0),
    _burst_end(),
    _periods(),
    _period_duration(NanoSecPerSec),
    _cur_period(0)
{
}


//----------------------------------------------------------------------------
// Set a new report.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::setReport(ts::Report *report, int log_level)
{
    _report = report == nullptr ? NullReport::Instance() : report;
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
    _burst_min = Monotonic::SetPrecision(2 * NanoSecPerMilliSec);
    _report->log(_log_level, u"minimum packet burst duration is %'d nano-seconds", {_burst_min});

    // Initial measurement period is one second. Will be enlarged for extra-low bitrates.
    _period_duration = NanoSecPerSec;

    // Reset state
    _starting = true;
    _regulated = false;
    _burst_duration = 0;
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
    _burst_duration = ((NanoSecPerSec * PKT_SIZE_BITS * burst_pkt_max) / _cur_bitrate).toInt();

    // If the result is too small for the time precision of the operating system, recompute a larger burst duration.
    if (_burst_duration < _burst_min) {
        _burst_duration = _burst_min;
        burst_pkt_max = ((_burst_duration * _cur_bitrate) / (NanoSecPerSec * PKT_SIZE_BITS)).toInt();
    }

    // New end of burst sequence.
    _burst_end.getSystemTime();
    _burst_end += _burst_duration;

    // Measurement period is one second by default but must be larger than 2 bursts.
    _period_duration = std::max(NanoSecPerSec, 2 * _burst_duration);

    _report->debug(u"new regulation, burst: %'d nano-seconds, %'d packets, measurement period: %'d nano-seconds", {_burst_duration, burst_pkt_max, _period_duration});
}


//----------------------------------------------------------------------------
// Process one packet in a regulated burst. Wait at end of burst.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::regulatePacket(bool& flush)
{
    // Total measurement period.
    Monotonic now(true);
    NanoSecond duration = now - otherPeriod().start;

    // Allowed bits in the total measurement period.
    int64_t max_bits = ((_cur_bitrate * duration) / NanoSecPerSec).toInt();

    // While not enough bit credit for one packet, wait until end of current burst.
    while (otherPeriod().bits + currentPeriod().bits + int64_t(PKT_SIZE_BITS) > max_bits) {
        // Wait until scheduled end of burst.
        _burst_end.wait();
        // Restart a new burst, use monotonic time.
        _burst_end += _burst_duration;
        // Flush current burst
        flush = true;
        // Update measurement period and bit credit.
        now.getSystemTime();
        duration = now - otherPeriod().start;
        max_bits = ((_cur_bitrate * duration) / NanoSecPerSec).toInt();
    }

    // Switch measurement period when necessary.
    if (now - currentPeriod().start >= _period_duration) {
        // The "other" period will disappear.
        // Credit unused bits from the other period to the current period.
        currentPeriod().bits -= ((_cur_bitrate * (currentPeriod().start - otherPeriod().start)) / NanoSecPerSec).toInt() - otherPeriod().bits;
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
            otherPeriod().start.getSystemTime();
            currentPeriod().start = otherPeriod().start;
            otherPeriod().bits = currentPeriod().bits = 0;
            // Setup burst duration.
            handleNewBitrate();
            bitrate_changed = true;
            regulatePacket(flush);
        }
    }
}
