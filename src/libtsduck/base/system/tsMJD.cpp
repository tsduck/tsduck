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

#include "tsMJD.h"
#include "tsBCD.h"
#include "tsTime.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// This routine converts a modified Julian date (MJD) into a base::Time.
//----------------------------------------------------------------------------

bool ts::DecodeMJD(const uint8_t* mjd, size_t mjd_size, Time& time)
{
    // Default invalid value. We use a portable constant value, not the system Epoch.
    time = Time::UnixEpoch;

    // Check buffer size
    if (mjd_size < MJD_MIN_SIZE || mjd_size > MJD_SIZE) {
        return false;
    }

    // Get day since MJD epoch.
    const uint64_t day = uint64_t(GetUInt16(mjd));
    bool valid = day != 0xFFFF; // Often used as invalid date.

    // Compute milliseconds since MJD epoch
    MilliSecond mjd_ms = day * MilliSecPerDay;
    if (mjd_size >= 3 && valid) {
        valid = IsValidBCD(mjd[2]);
        mjd_ms += DecodeBCD(mjd[2]) * MilliSecPerHour; // Hours
    }
    if (mjd_size >= 4 && valid) {
        valid = IsValidBCD(mjd[3]);
        mjd_ms += DecodeBCD(mjd[3]) * MilliSecPerMin; // Minutes
    }
    if (mjd_size >= 5 && valid) {
        valid = IsValidBCD(mjd[4]);
        mjd_ms += DecodeBCD(mjd[4]) * MilliSecPerSec; // Seconds
    }
    if (!valid) {
        return false;
    }

    // Rebuild time depending on MJD and Time epoch
    if (Time::JulianEpochOffset >= 0 || mjd_ms >= - Time::JulianEpochOffset) {
        // MJD epoch is after Time epoch or else
        // MJD time is after Time epoch, fine
        time = Time::Epoch + (mjd_ms + Time::JulianEpochOffset);
    }
    else {
        // MJD time is before Time epoch, cannot be represented.
        time = Time::Epoch;
    }
    return true;
}

//----------------------------------------------------------------------------
// This routine converts a base::Time into a modified Julian Date (MJD).
//----------------------------------------------------------------------------

bool ts::EncodeMJD(const Time& time, uint8_t* mjd, size_t mjd_size)
{
    // Check buffer size
    if (mjd_size < 2 || mjd_size > 5) {
        return false;
    }

    // Compute milliseconds since Time epoch
    MilliSecond time_ms = time - Time::Epoch;

    // Cannot represent dates earlier than MJD epoch
    if (time_ms < Time::JulianEpochOffset) {
        Zero(mjd, mjd_size);
        return false;
    }

    const uint64_t d = (time_ms - Time::JulianEpochOffset) / 1000; // seconds since MJD epoch
    PutUInt16(mjd, uint16_t(d / (24 * 3600))); // days
    if (mjd_size >= 3) {
        mjd[2] = EncodeBCD(int((d / 3600) % 24)); // hours
    }
    if (mjd_size >= 4) {
        mjd[3] = EncodeBCD(int((d / 60) % 60)); // minutes
    }
    if (mjd_size >= 5) {
        mjd[4] = EncodeBCD(int(d % 60)); // seconds
    }
    return true;
}
