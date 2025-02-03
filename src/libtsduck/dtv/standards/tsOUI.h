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
//!  Common definitions for IEEE Organizationally Unique Identifier (OUI).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {
    //!
    //! Some IEEE-assigned Organizationally Unique Identifier (OUI) values.
    //!
    enum class OUI {
        DVB      = 0x00015A,  //!< OUI for Digital Video Broadcasting
        SKARDIN  = 0x001222,  //!< OUI for Skardin (UK)
        LOGIWAYS = 0x002660,  //!< OUI for Logiways
    };

    //!
    //! Get the name of an IEEE-assigned Organizationally Unique Identifier (OUI).
    //! @param [in] oui 24-bit OUI value.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString OUIName(uint32_t oui, NamesFlags flags = NamesFlags::NAME);
}
