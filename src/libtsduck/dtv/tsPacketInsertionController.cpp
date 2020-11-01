//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsPacketInsertionController.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::PacketInsertionController::DEFAULT_WAIT_ALERT;
constexpr size_t ts::PacketInsertionController::DEFAULT_BITRATE_RESET_PERCENT;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PacketInsertionController::PacketInsertionController(Report& report) :
    _report(report),
    _main_name(u"main stream"),
    _sub_name(u"sub-stream"),
    _main_packets(0),
    _sub_packets(0),
    _wait_alert(DEFAULT_WAIT_ALERT),
    _accel_factor(1),
    _accel_main_packets(0),
    _accel_sub_packets(0),
    _accel_max_wait(0),
    _main_bitrate(_report, _main_name),
    _sub_bitrate(_report, _sub_name)
{
}


//----------------------------------------------------------------------------
// Reset the state of the controller.
//----------------------------------------------------------------------------

void ts::PacketInsertionController::reset()
{
    _main_packets = 0;
    _sub_packets = 0;
    _accel_factor = 1;
    _accel_main_packets = 0;
    _accel_sub_packets = 0;
    _accel_max_wait = 0;
}


//----------------------------------------------------------------------------
// This class computes a bitrate based on all its successive values.
//----------------------------------------------------------------------------

ts::PacketInsertionController::BitRateControl::BitRateControl(Report& report, const UString& name) :
    _report(report),
    _name(name),
    _count(0),
    _value_0(0),
    _diffs(0),
    _average(0),
    _reset_percent(DEFAULT_BITRATE_RESET_PERCENT)
{
}

size_t ts::PacketInsertionController::BitRateControl::diffPercent(BitRate rate) const
{
    return size_t(std::abs(((int64_t(rate) - int64_t(_average)) * 100) / int64_t(_average)));
}

bool ts::PacketInsertionController::BitRateControl::setBitRate(BitRate rate)
{
    if (rate == 0) {
        // Unknown bitrate.
        if (_average != 0) {
            _report.verbose(u"%s bitrate now unknown (was %'d b/s)", {_name, _average});
        }
        _count = 0;
        _value_0 = 0;
        _diffs = 0;
        _average = 0;
        return false; // reset
    }
    else if (_count == 0 || _average == 0 || diffPercent(rate) > _reset_percent) {
        // First value or reset computation.
        if (rate != _average) {
            _report.verbose(u"%s bitrate reset to %'d b/s (was %'d b/s)", {_name, rate, _average});
        }
        _count = 1;
        _value_0 = int64_t(rate);
        _diffs = 0;
        _average = rate;
        return false; // reset
    }
    else {
        _count++;
        _diffs += int64_t(rate) - _value_0;
        const int64_t new_average = _value_0 + _diffs / _count;
        if (new_average > 0) {
            _average = BitRate(new_average);
        }
        // Report bitrate adjustment over 1% only.
        if (diffPercent(rate) > 1) {
            _report.verbose(u"%s bitrate set to %'d b/s, adjusted to %'d b/s", {_name, rate, _average});
        }
        return true; // continue
    }
}


//----------------------------------------------------------------------------
// Bitrate management.
//----------------------------------------------------------------------------

void ts::PacketInsertionController::setBitRateVariationResetThreshold(size_t percent)
{
    _main_bitrate.setResetThreshold(percent);
    _sub_bitrate.setResetThreshold(percent);
}

void ts::PacketInsertionController::setMainBitRate(BitRate rate)
{
    // In case of bitrate reset, reset the insertion strategy.
    if (!_main_bitrate.setBitRate(rate)) {
        reset();
    }
}

void ts::PacketInsertionController::setSubBitRate(BitRate rate)
{
    // In case of bitrate reset, reset the insertion strategy.
    if (!_sub_bitrate.setBitRate(rate)) {
        reset();
    }
}


//----------------------------------------------------------------------------
// Check if a packet from the sub-stream shall be inserted.
//----------------------------------------------------------------------------

bool ts::PacketInsertionController::mustInsert(size_t waiting_packets)
{
    if (_main_bitrate.getBitRate() == 0 || _sub_bitrate.getBitRate() == 0) {
        // Unknow bitrate, always insert.
        return true;
    }
    else if (_main_packets * _sub_bitrate.getBitRate() >= _sub_packets * _main_bitrate.getBitRate()) {
        // It is time to insert in all cases.
        return true;
    }
    else if (_wait_alert == 0 || waiting_packets < _wait_alert) {
        // No need to accelerate, acceleration disabled or there are not enough waiting packets.
        if (_accel_factor > 1) {
            // Stop acceleration.
            _accel_factor = 1;
            _accel_max_wait = 0;
            _report.verbose(u"waiting packets back to normal, %s bitrate back to %'d", {_sub_name, _sub_bitrate.getBitRate()});
        }
        return false;
    }

    // We are in an acceleration phase (too many waiting packets).
    if (_accel_factor == 1 || waiting_packets > _accel_max_wait) {
        // Start accelerating more.
        // If _accel_factor was 1, this is the start of the acceleration.
        // Otherwise, the number of waiting packets has increased and we need to accelerate more.
        // We keep the highest acceleration factor until the number of waiting packets decreases.
        _accel_factor++;
        _accel_main_packets = _main_packets;
        _accel_sub_packets = _sub_packets;
        _accel_max_wait = waiting_packets;
        _report.verbose(u"%'d waiting packets, accelerating %s bitrate by factor %d", {waiting_packets, _sub_name, _accel_factor});
    }

    // Use the same insertion criteria with the accelerated sub-bitrate over the current accelerated phase.
    return (_main_packets - _accel_main_packets) * _accel_factor * _sub_bitrate.getBitRate() >= (_sub_packets - _accel_sub_packets) * _main_bitrate.getBitRate();
}
