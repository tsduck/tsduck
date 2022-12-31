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
    _state(INITIAL),
    _opt_bitrate(0),
    _cur_bitrate(0),
    _opt_burst(0),
    _burst_pkt_max(0),
    _burst_pkt_cnt(0),
    _burst_min(0),
    _burst_duration(0),
    _burst_end(),
    _bitrate_start(),
    _bitrate_pkt_cnt(0)
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
    // Compute the minimum delay between two bursts, in nano-seconds.
    // This is a limitation of the operating system. If we try to use
    // wait on durations lower than the minimum, this will introduce
    // latencies which mess up the regulation. We try to request 2
    // milliseconds as time precision and we keep what the operating
    // system gives.

    _burst_min = Monotonic::SetPrecision(2000000); // 2 milliseconds in nanoseconds

    _report->log(_log_level, u"minimum packet burst duration is %'d nano-seconds", {_burst_min});

    // Reset state
    _state = INITIAL;
    _cur_bitrate = 0;
    _burst_pkt_max = 0;
    _burst_pkt_cnt = 0;
    _burst_duration = 0;
}


//----------------------------------------------------------------------------
// Handle bitrate change, compute burst duration.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::handleNewBitrate()
{
    // Assume that the packets/burst is the one specified on the command line.
    _burst_pkt_max = _opt_burst == 0 ? 1 : _opt_burst;

    // Compute corresponding duration (in nano-seconds) between two bursts.
    assert(_cur_bitrate > 0);
    _burst_duration = ((NanoSecPerSec * PKT_SIZE_BITS * _burst_pkt_max) / _cur_bitrate).toInt();

    // If the result is too small for the time precision of the operating
    // system, recompute a larger burst duration
    if (_burst_duration < _burst_min) {
        _burst_duration = _burst_min;
        _burst_pkt_max = ((_burst_duration * _cur_bitrate) / (NanoSecPerSec * PKT_SIZE_BITS)).toInt();
    }

    _report->debug(u"new regulation, burst: %'d nano-seconds, %'d packets", {_burst_duration, _burst_pkt_max});

    // Register start of bitrate sequence.
    _bitrate_pkt_cnt = 0;
    _bitrate_start.getSystemTime();
}


//----------------------------------------------------------------------------
// Process one packet in a regulated burst. Wait at end of burst.
//----------------------------------------------------------------------------

void ts::BitRateRegulator::regulatePacket(bool& flush, bool smoothen)
{
    // Check if end of current burst. Take care, _burst_pkt_cnt may be already zero.
    if ((_burst_pkt_cnt == 0 || --_burst_pkt_cnt == 0) && smoothen && _bitrate_pkt_cnt > 0) {
        // In the middle of a sequence with same bitrate, we try to smoothen the regulation.
        // Because of rounding, we tend to pass slightly less packets than requested.
        // See if we need to add some packets from time to time.
        const Monotonic now(true);
        // Number of packets we should have passed since beginning of sequence of this bitrate:
        const PacketCounter expected = PacketDistance(_cur_bitrate, (now - _bitrate_start) / NanoSecPerMilliSec);
        if (expected > _bitrate_pkt_cnt) {
            // We should have passed more than we did, increase this burst size.
            _burst_pkt_cnt = expected - _bitrate_pkt_cnt;
        }
    }

    // Recheck end of burst, just in case we added some more packets to smoothen.
    if (_burst_pkt_cnt == 0) {
        // Wait until scheduled end of burst.
        _burst_end.wait();
        // Restart a new burst, use monotonic time
        _burst_pkt_cnt = _burst_pkt_max;
        _burst_end += _burst_duration;
        // Flush current burst
        flush = true;
    }

    // One more regulated packet at this bitrate.
    _bitrate_pkt_cnt++;
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

    if (_cur_bitrate != old_bitrate || _state == INITIAL) {
        // Initial state or new bitrate
        if (_cur_bitrate == 0) {
            _report->log(_log_level, u"unknown bitrate, cannot regulate.");
        }
        else {
            _report->log(_log_level, u"regulated at bitrate %'d b/s", {_cur_bitrate.toInt()});
        }
    }

    // Process with state machine
    switch (_state) {

        case INITIAL: {
            // Initial state, will become either regulated or unregulated
            if (_cur_bitrate == 0) {
                // No bitrate -> unregulated
                _state = UNREGULATED;
            }
            else {
                // Got a non-zero bitrate -> start regulation
                _state = REGULATED;
                // Compute initial burst duration: first, compute burst time
                handleNewBitrate();
                // Get initial clock
                _burst_end.getSystemTime();
                // Compute end time of next burst
                _burst_end += _burst_duration;
                // We are at the start of the burst, initialize countdown
                _burst_pkt_cnt = _burst_pkt_max;
                // Transmit first packet of burst
                bitrate_changed = true;
                regulatePacket(flush, false);
            }
            break;
        }

        case UNREGULATED: {
            // We had no bitrate, we did not regulate
            if (_cur_bitrate > 0) {
                // Finally got a bitrate.
                // Transmit this packet without regulation and flush.
                // Will regulate next time, starting with empty (flushed) buffer.
                _state = INITIAL;
                bitrate_changed = true;
                flush = true;
            }
            break;
        }

        case REGULATED: {
            // We previously had a bitrate and we regulated the flow.
            if (_cur_bitrate == 0) {
                // No more bitrate, become unregulated
                _state = UNREGULATED;
            }
            else if (_cur_bitrate == old_bitrate) {
                // Still the same bitrate, continue to burst
                regulatePacket(flush, true);
            }
            else {
                // Got a new non-zero bitrate. Compute new burst.
                // Revert to start time of current burst, based on previous burst duration.
                _burst_end -= _burst_duration;
                // Compute the estimated due elapsed time in current burst, based on
                // previous burst duration and proportion of passed packets.
                const NanoSecond elapsed = _burst_duration - (_burst_duration * _burst_pkt_max) / _burst_pkt_cnt;
                // Compute new burst duration, based on new bitrate
                handleNewBitrate();
                // Adjust end time of current burst. We want to close the current burst
                // as soon as possible to restart based on new bitrate.
                if (elapsed >= _burst_min) {
                    // We already passed more packets that required for minimum delay.
                    // Close current burst now.
                    _burst_end += elapsed;
                    _burst_pkt_cnt = 0;
                }
                else {
                    // We have to wait a bit more to respect the minimum delay.
                    // Compute haw many packets we should pass for the remaining time,
                    // based on the new bitrate.
                    _burst_end += _burst_min;
                    _burst_pkt_cnt = (((_burst_min - elapsed) * _cur_bitrate) / (NanoSecPerSec * PKT_SIZE_BITS)).toInt();
                }
                // Report that the bitrate has changed
                bitrate_changed = true;
                regulatePacket(flush, false);
            }
            break;
        }

        default: {
            assert(false);
        }
    }
}
