//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Description of a Low-Noise Block (LNB) converter in a satellite dish.
//
//----------------------------------------------------------------------------

#include "tsLNB.h"
TSDUCK_SOURCE;

// LNB constants
const ts::LNB ts::LNB::Universal; // default values
const ts::LNB ts::LNB::Null(0, 0, 0);


//----------------------------------------------------------------------------
// Compute the intermediate frequency from a satellite carrier frequency.
// The satellite carrier frequency is used to carry the signal from the
// satellite to the dish. This value is public and is stored in the NIT
// for instance. The intermediate frequency is used to carry the signal
// from the dish's LNB to the receiver. The way this frequency is
// computed depends on the characteristics of the LNB. The intermediate
// frequency is the one that is used by the tuner in the satellite receiver.
//----------------------------------------------------------------------------

uint64_t ts::LNB::intermediateFrequency (uint64_t satellite_frequency) const
{
    if (useHighBand (satellite_frequency)) {
        return satellite_frequency - _high_frequency;
    }
    else if (satellite_frequency < _low_frequency) {
        return _low_frequency - satellite_frequency;
    }
    else {
        return satellite_frequency - _low_frequency;
    }
}


//----------------------------------------------------------------------------
// Convert to a string object
// Return a normalized representation of the LNB:
// - "freq" if the LNB has not high band.
// - "low,high,switch" if the LNB has a high band.
// In strings, all values are in MHz.
//----------------------------------------------------------------------------

ts::LNB::operator ts::UString() const
{
    if (hasHighBand()) {
        return UString::Format(u"%d,%d,%d", {_low_frequency / 1000000, _high_frequency / 1000000, _switch_frequency / 1000000});
    }
    else {
        return UString::Format("%d", {_low_frequency / 1000000});
    }
}


//----------------------------------------------------------------------------
// Interpret a string as an LNB value.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::LNB::set(const UString& s)
{
    // Split a comma-separated string
    UStringVector values;
    s.split(values);

    // Interpret values
    bool ok = false;
    if (values.size() == 1) {
        // no high band
        ok = values[0].toInteger(_low_frequency);
        _high_frequency = _switch_frequency = 0;
    }
    else if (values.size() == 3) {
        ok = values[0].toInteger(_low_frequency) && values[1].toInteger(_high_frequency) && values[2].toInteger(_switch_frequency);
    }

    if (ok) {
        // Convert values from MHz to Hz
        _low_frequency *= 1000000;
        _high_frequency *= 1000000;
        _switch_frequency *= 1000000;
    }
    else {
        // In case of errors, all values are zero
        _low_frequency = _high_frequency = _switch_frequency = 0;
    }

    return ok;
}
