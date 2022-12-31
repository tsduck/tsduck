//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSignalState.h"
#include "tsFixedPoint.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SignalState::SignalState() :
    signal_locked(false),
    signal_strength(),
    signal_noise_ratio(),
    bit_error_rate(),
    packet_error_rate()
{
}

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
