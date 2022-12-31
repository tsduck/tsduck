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
