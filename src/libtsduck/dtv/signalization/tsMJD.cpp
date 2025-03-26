//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// The representation of a DVB date is a 16-bit number of days since the
// origin of the Modified Julian Dates, 17 Nov 1858. The maximum value 0xFFFF
// represents 22 Apr 2038. On March 2025, with this fatal date approaching,
// it has been decided to extend the representation up to year 2128.
//
// If the most significant bit of the 16-bit value is zero, then the actual
// MJD is wrapped after 0x10000.
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
    cn::days::rep day = cn::days::rep(GetUInt16(mjd));

    // Trick to use MJD dates after 22 Apr 2038.
    if (day < 0x8000) {
        day += 0x1'0000;
    }

    // Compute milliseconds since MJD epoch
    cn::milliseconds mjd_ms = cn::duration_cast<cn::milliseconds>(cn::days(day));
    if (mjd_fmt == MJD_FULL) {
        if (IsValidBCD(mjd[2]) && IsValidBCD(mjd[3]) && IsValidBCD(mjd[4])) {
            mjd_ms += cn::hours(DecodeBCD(mjd[2])) + cn::minutes(DecodeBCD(mjd[3])) + cn::seconds(DecodeBCD(mjd[4]));
        }
        else {
            // Invalid BCD representation of hh::mm:ss. The typical use case is a date
            // field with all bits set to 1, meaning "unspecified date".
            return false;
        }
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
    const cn::seconds::rep secs = cn::duration_cast<cn::seconds>(time_sec - Time::JulianEpochOffset).count();
    const cn::seconds::rep days = secs / (24 * 3600);

    // Trick to use MJD dates after 22 Apr 2038 : days must be in range 0x8000 to 0x17FFF.
    // The actual 16-bit value is the 16 lsb of days.
    if (days < 0x8000 || days > 0x1'7FFF) {
        MemZero(mjd, MJDSize(mjd_fmt));
        return false;
    }

    // Now we can encode.
    PutUInt16(mjd, uint16_t(days));
    if (mjd_fmt == MJD_FULL) {
        mjd[2] = EncodeBCD(int((secs / 3600) % 24)); // hours
        mjd[3] = EncodeBCD(int((secs / 60) % 60));   // minutes
        mjd[4] = EncodeBCD(int(secs % 60));          // seconds
    }
    return true;
}
