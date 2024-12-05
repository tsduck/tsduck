//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        NONE    = 0x0000,  //!< No known standard
        MPEG    = 0x0001,  //!< Defined by MPEG, common to all standards
        DVB     = 0x0002,  //!< Defined by ETSI/DVB.
        SCTE    = 0x0004,  //!< Defined by ANSI/SCTE.
        ATSC    = 0x0008,  //!< Defined by ATSC.
        ISDB    = 0x0010,  //!< Defined by ISDB.
        JAPAN   = 0x0020,  //!< Defined in Japan only (typically in addition to ISDB).
        ABNT    = 0x0040,  //!< Defined by ABNT (Brazil, typically in addition to ISDB).
        DVBONLY = 0x8000   //!< Used with DVB. Means strict DVB, cannot be the DVB subset as used by ISDB.
    };

    //!
    //! Return a string representing a list of standards.
    //! @param [in] standards A bit mask of standards.
    //! @return A string representing the standards.
    //!
    TSDUCKDLL UString StandardsNames(Standards standards);

    //!
    //! Check compatibility between standards.
    //! Some standards are compatible, they can be used together. Example: MPEG and DVB.
    //! Some standards are incompatible and cannot be used together. Example: DVB and ATSC.
    //! @param [in] std Set of standards.
    //! @return True if all standards in @a std are compatible with each other.
    //!
    TSDUCKDLL bool CompatibleStandards(Standards std);
}

TS_ENABLE_BITMASK_OPERATORS(ts::Standards);
