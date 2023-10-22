//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Modified Julian Date (MJD) utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"

namespace ts {
    //!
    //! Size in bytes of an encoded complete Modified Julian Date (MJD).
    //!
    const size_t MJD_SIZE = 5;

    //!
    //! Minimal size in bytes of an encoded Modified Julian Date (MJD), ie. date only.
    //!
    const size_t MJD_MIN_SIZE = 2;

    //!
    //! Convert a Modified Julian Date (MJD) into a ts::Time.
    //! @param [in] mjd Address of a 2-to-5 bytes area, in the format specified by a TDT.
    //! @param [in] mjd_size Size in bytes of the @a mjd area.
    //! @param [out] time Return time.
    //! @return True on success, false in case of error.
    //!
    TSDUCKDLL bool DecodeMJD(const uint8_t* mjd, size_t mjd_size, Time& time);

    //!
    //! Convert a ts::Time into a Modified Julian Date (MJD).
    //! @param [in] time Input time.
    //! @param [out] mjd Address of a writeable 2-to-5 bytes area.
    //! @param [in] mjd_size Size in bytes of the @a mjd area.
    //! @return True on success, false in case of error.
    //!
    TSDUCKDLL bool EncodeMJD(const Time& time, uint8_t* mjd, size_t mjd_size);
}
