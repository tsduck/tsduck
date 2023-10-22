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
//!  Common definitions for IEEE Organizationally Unique Identifier (OUI).
//!
//----------------------------------------------------------------------------

#pragma once

namespace ts {
    //!
    //! Some IEEE-assigned Organizationally Unique Identifier (OUI) values.
    //!
    enum class OUI {
        DVB      = 0x00015A,  //!< OUI for Digital Video Broadcasting
        SKARDIN  = 0x001222,  //!< OUI for Skardin (UK)
        LOGIWAYS = 0x002660,  //!< OUI for Logiways
    };
}
