//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPacketInsertionController.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PacketInsertionController::PacketInsertionController(Report& report) :
    _report(report)
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
    _name(name)
{
}

size_t ts::PacketInsertionController::BitRateControl::diffPercent(const BitRate& rate) const
{
    return size_t((((rate - _average) * 100) / _average).abs().toInt());
}

bool ts::PacketInsertionController::BitRateControl::setBitRate(const BitRate& rate)
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
        _value_0 = rate;
        _diffs = 0;
        _average = rate;
        return false; // reset
    }
    else {
        _count++;
        _diffs += rate - _value_0;
        const BitRate new_average = _value_0 + _diffs / _count;
        if (new_average > 0) {
            _average = new_average;
        }
        // Report bitrate adjustment over 5% only.
        if (diffPercent(rate) > 5) {
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

void ts::PacketInsertionController::setMainBitRate(const BitRate& rate)
{
    // In case of bitrate reset, reset the insertion strategy.
    if (!_main_bitrate.setBitRate(rate)) {
        reset();
    }
}

void ts::PacketInsertionController::setSubBitRate(const BitRate& rate)
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
