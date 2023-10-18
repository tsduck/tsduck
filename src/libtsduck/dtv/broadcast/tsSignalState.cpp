//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsSignalState.h"
#include "tsFixedPoint.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

void ts::SignalState::clear()
{
    signal_locked = false;
    signal_strength.clear();
    signal_noise_ratio.clear();
    bit_error_rate.clear();
    packet_error_rate.clear();
}


//----------------------------------------------------------------------------
// Set a percentage value from a raw driver value.
//----------------------------------------------------------------------------

void ts::SignalState::setPercent(Variable<Value> SignalState::* field, int64_t value, int64_t min, int64_t max)
{
    if (max <= min) {
        // Invalid range.
        (this->*field).clear();
    }
    else {
        this->*field = Value(((value - min) * 100) / (max - min), Unit::PERCENT);
    }
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::SignalState::Value::toString() const
{
    switch (unit) {
        case Unit::COUNTER:
            return UString::Decimal(value);
        case Unit::PERCENT:
            return UString::Format(u"%d%%", {value});
        case Unit::MDB:
            return UString::Format(u"%s dB", {FixedPoint<int64_t, 3>(value, true)});
        default:
            return UString();
    }
}

ts::UString ts::SignalState::toString() const
{
    UString str;
    str.format(u"locked: %s", {UString::YesNo(signal_locked)});
    if (signal_strength.set()) {
        str.format(u", strength: %s", {signal_strength.value()});
    }
    if (signal_noise_ratio.set()) {
        str.format(u", SNR: %s", {signal_noise_ratio.value()});
    }
    if (bit_error_rate.set()) {
        str.format(u", BER: %s", {bit_error_rate.value()});
    }
    if (packet_error_rate.set()) {
        str.format(u", PER: %s", {packet_error_rate.value()});
    }
    return str;
}


//----------------------------------------------------------------------------
// Implementation of DisplayInterface.
//----------------------------------------------------------------------------

std::ostream& ts::SignalState::display(std::ostream& out, const UString& margin, int level) const
{
    out << margin << "Signal locked: " << UString::YesNo(signal_locked) << std::endl;
    if (signal_strength.set()) {
        out << margin << "Signal strength: " << signal_strength.value() << std::endl;
    }
    if (signal_noise_ratio.set()) {
        out << margin << "Signal/noise ratio: " << signal_noise_ratio.value() << std::endl;
    }
    if (bit_error_rate.set()) {
        out << margin << "Bit error rate: " << bit_error_rate.value() << std::endl;
    }
    if (packet_error_rate.set()) {
        out << margin << "Packet error rate: " << packet_error_rate.value() << std::endl;
    }
    return out;
}
