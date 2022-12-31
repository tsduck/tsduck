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
//!  Definition of the various DTV standards which are used in TSDuck.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Bit masks for standards, used to qualify the signalization.
    //! @ingroup mpeg
    //!
    enum class Standards : uint16_t {
        NONE  = 0x00,  //!< No known standard
        MPEG  = 0x01,  //!< Defined by MPEG, common to all standards
        DVB   = 0x02,  //!< Defined by ETSI/DVB.
        SCTE  = 0x04,  //!< Defined by ANSI/SCTE.
        ATSC  = 0x08,  //!< Defined by ATSC.
        ISDB  = 0x10,  //!< Defined by ISDB.
        JAPAN = 0x20,  //!< Defined in Japan only (typically in addition to ISDB).
        ABNT  = 0x40,  //!< Defined by ABNT (Brazil, typically in addition to ISDB).
    };

    //!
    //! Return a string representing a list of standards.
    //! @param [in] standards A bit mask of standards.
    //! @return A string representing the standards.
    //!
    TSDUCKDLL UString StandardsNames(Standards standards);
}

TS_ENABLE_BITMASK_OPERATORS(ts::Standards);
