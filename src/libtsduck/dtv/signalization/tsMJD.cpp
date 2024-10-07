//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMJD.h"
#include "tsBCD.h"
#include "tsTime.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// This routine converts a modified Julian date (MJD) into a base::Time.
//----------------------------------------------------------------------------

bool ts::DecodeMJD(const uint8_t* mjd, MJDFormat mjd_fmt, Time& time)
{
    // Default invalid value. We use a portable constant value, not the system Epoch.
    time = Time::UnixEpoch;

    // Check MJD format.
    if (mjd_fmt != MJD_DATE && mjd_fmt != MJD_FULL) {
        return false;
    }

    // Get day since MJD epoch.
    const cn::days::rep day = cn::days::rep(GetUInt16(mjd));
    bool valid = day != 0xFFFF; // Often used as invalid date.

    // Compute milliseconds since MJD epoch
    cn::milliseconds mjd_ms = cn::duration_cast<cn::milliseconds>(cn::days(day));
    if (valid && mjd_fmt == MJD_FULL) {
        valid = IsValidBCD(mjd[2]) && IsValidBCD(mjd[3]) && IsValidBCD(mjd[4]);
        mjd_ms += cn::hours(DecodeBCD(mjd[2])) + cn::minutes(DecodeBCD(mjd[3])) + cn::seconds(DecodeBCD(mjd[4]));
    }
    if (!valid) {
        return false;
    }

    // Rebuild time depending on MJD and Time epoch
    if (Time::JulianEpochOffset >= cn::milliseconds::zero() || mjd_ms >= - Time::JulianEpochOffset) {
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

bool ts::EncodeMJD(const Time& time, uint8_t* mjd, MJDFormat mjd_fmt)
{
    // Check MJD format.
    if (mjd_fmt != MJD_DATE && mjd_fmt != MJD_FULL) {
        return false;
    }

    // Compute seconds since Time epoch.
    cn::seconds time_sec = cn::duration_cast<cn::seconds>(time - Time::Epoch);

    // Cannot represent dates earlier than MJD epoch
    if (time_sec < Time::JulianEpochOffset) {
        MemZero(mjd, MJDSize(mjd_fmt));
        return false;
    }

    // Compute seconds since MJD epoch
    const cn::seconds::rep d = cn::duration_cast<cn::seconds>(time_sec - Time::JulianEpochOffset).count();
    PutUInt16(mjd, uint16_t(d / (24 * 3600)));    // days
    if (mjd_fmt == MJD_FULL) {
        mjd[2] = EncodeBCD(int((d / 3600) % 24)); // hours
        mjd[3] = EncodeBCD(int((d / 60) % 60));   // minutes
        mjd[4] = EncodeBCD(int(d % 60));          // seconds
    }
    return true;
}
