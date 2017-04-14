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
#include "tsFormat.h"
#include "tsToInteger.h"


// LNB constants
const ts::LNB ts::LNB::Universal; // default values
const ts::LNB ts::LNB::Null (0, 0, 0);


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

ts::LNB::operator std::string() const
{
    if (hasHighBand()) {
        return Format ("%" FMT_INT64 "u,%" FMT_INT64 "u,%" FMT_INT64 "u",
                       uint64_t (_low_frequency / TS_UCONST64 (1000000)),
                       uint64_t (_high_frequency / TS_UCONST64 (1000000)),
                       uint64_t (_switch_frequency / TS_UCONST64 (1000000)));
    }
    else {
        return Format ("%" FMT_INT64 "u", uint64_t (_low_frequency / TS_UCONST64 (1000000)));
    }
}


//----------------------------------------------------------------------------
// Interpret a string as an LNB value.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::LNB::set (const char* s)
{
    // Split a comma-separated string
    std::vector<std::string> values;
    SplitString (values, s);

    // Interpret values
    bool ok = false;
    if (values.size() == 1) {
        // no high band
        ok = ToInteger (_low_frequency, values[0]);
        _high_frequency = _switch_frequency = 0;
    }
    else if (values.size() == 3) {
        ok = ToInteger (_low_frequency, values[0]) &&
            ToInteger (_high_frequency, values[1]) &&
            ToInteger (_switch_frequency, values[2]);
    }

    if (ok) {
        // Convert values from MHz to Hz
        _low_frequency *= TS_UCONST64 (1000000);
        _high_frequency *= TS_UCONST64 (1000000);
        _switch_frequency *= TS_UCONST64 (1000000);
    }
    else {
        // In case of errors, all values are zero
        _low_frequency = _high_frequency = _switch_frequency = 0;
    }

    return ok;
}
