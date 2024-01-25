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

bool ts::DecodeMJD(const uint8_t* mjd, size_t mjd_size, Time& time)
{
    // Default invalid value. We use a portable constant value, not the system Epoch.
    time = Time::UnixEpoch;

    // Check buffer size
    if (mjd_size < MJD_MIN_SIZE || mjd_size > MJD_SIZE) {
        return false;
    }

    // Get day since MJD epoch.
    const cn::days::rep day = cn::days::rep(GetUInt16(mjd));
    bool valid = day != 0xFFFF; // Often used as invalid date.

    // Compute milliseconds since MJD epoch
    cn::milliseconds mjd_ms = cn::duration_cast<cn::milliseconds>(cn::days(day));
    if (mjd_size >= 3 && valid) {
        valid = IsValidBCD(mjd[2]);
        mjd_ms += cn::hours(DecodeBCD(mjd[2]));
    }
    if (mjd_size >= 4 && valid) {
        valid = IsValidBCD(mjd[3]);
        mjd_ms += cn::minutes(DecodeBCD(mjd[3]));
    }
    if (mjd_size >= 5 && valid) {
        valid = IsValidBCD(mjd[4]);
        mjd_ms += cn::seconds(DecodeBCD(mjd[4]));
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

bool ts::EncodeMJD(const Time& time, uint8_t* mjd, size_t mjd_size)
{
    // Check buffer size
    if (mjd_size < 2 || mjd_size > 5) {
        return false;
    }

    // Compute seconds since Time epoch.
    cn::seconds time_sec = cn::duration_cast<cn::seconds>(time - Time::Epoch);

    // Cannot represent dates earlier than MJD epoch
    if (time_sec < Time::JulianEpochOffset) {
        MemZero(mjd, mjd_size);
        return false;
    }

    // Conput seconds since MJD epoch
    const cn::seconds::rep d = cn::duration_cast<cn::seconds>(time_sec - Time::JulianEpochOffset).count();
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
