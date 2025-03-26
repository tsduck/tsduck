//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Modified Julian Date (MJD) utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"

namespace ts {
    //!
    //! Origin of Modified Julian Dates (MJD).
    //! The origin of MJD is 17 Nov 1858 00:00:00.
    //! The UNIX epoch (1 Jan 1970) is 40587 days from julian time origin.
    //!
    constexpr uint32_t MJD_EPOCH = 40587;

    //!
    //! Storage format of Modified Julian Dates as used by DVB.
    //!
    //! The original DVB format is a weird one, a mixture of binary format (for the date)
    //! and Binary Coded Decimal (BCD) for the time. The full version, date and time, uses
    //! 5 bytes. The short version, date only, uses 2 bytes.
    //!
    //! Implementation guidelines: when new formats are defined, make sure to define
    //! values which, modulo 10, are equal to the corresponding storage size. If new
    //! formats introduce variable sizes, then reimplement the function MJDSize();
    //!
    enum MJDFormat {
        MJD_FULL = 5,  //!< Original DVB format, date and time.
        MJD_DATE = 2,  //!< Original DVB format, date only.
    };

    //!
    //! Get the size in bytes of a MJD value, depending on its format.
    //! @param [in] fmt MJD storage format.
    //! @return Size in bytes of the MJD values in this format.
    //!
    TSDUCKDLL inline size_t MJDSize(MJDFormat fmt) { return size_t(fmt) % 10; }

    //!
    //! Convert a Modified Julian Date (MJD) into a ts::Time.
    //! @param [in] mjd Address of a 2-to-5 bytes area, in the format specified by a TDT.
    //! @param [in] fmt Format of the MJD in the @a mjd area.
    //! @param [out] time Return time.
    //! @return True on success, false in case of error.
    //!
    TSDUCKDLL bool DecodeMJD(const uint8_t* mjd, MJDFormat fmt, Time& time);

    //!
    //! Convert a ts::Time into a Modified Julian Date (MJD).
    //! @param [in] time Input time.
    //! @param [out] mjd Address of a writeable 2-to-5 bytes area.
    //! @param [in] fmt Format of the MJD in the @a mjd area.
    //! @return True on success, false in case of error (typically out of range date).
    //!
    TSDUCKDLL bool EncodeMJD(const Time& time, uint8_t* mjd, MJDFormat fmt);
}
