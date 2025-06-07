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
//!  Generic ATSC definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Table type in ATSC Master Guide Table (MGT)
    //!
    enum : uint16_t {
        ATSC_TTYPE_TVCT_CURRENT = 0x0000,  //!< Terrestrial VCT with current_next_indicator=’1’.
        ATSC_TTYPE_TVCT_NEXT    = 0x0001,  //!< Terrestrial VCT with current_next_indicator=’0’.
        ATSC_TTYPE_CVCT_CURRENT = 0x0002,  //!< Cable VCT with current_next_indicator=’1’.
        ATSC_TTYPE_CVCT_NEXT    = 0x0003,  //!< Cable VCT with current_next_indicator=’0’.
        ATSC_TTYPE_CETT         = 0x0004,  //!< Channel ETT.
        ATSC_TTYPE_DCCSCT       = 0x0005,  //!< DCCSCT
        ATSC_TTYPE_EIT_FIRST    = 0x0100,  //!< First EIT (EIT-0).
        ATSC_TTYPE_EIT_LAST     = 0x017F,  //!< Last EIT (EIT-127).
        ATSC_TTYPE_EETT_FIRST   = 0x0200,  //!< First Event ETT (EET-0).
        ATSC_TTYPE_EETT_LAST    = 0x027F,  //!< Last Event ETT (ETT-127).
        ATSC_TTYPE_RRT_FIRST    = 0x0301,  //!< First RRT (RRT with rating_region 1).
        ATSC_TTYPE_RRT_LAST     = 0x03FF,  //!< Last RRT (RRT with rating_region 255).
        ATSC_TTYPE_DCCT_FIRST   = 0x1400,  //!< First DCCT (DCCT with dcc_id 0x00).
        ATSC_TTYPE_DCCT_LAST    = 0x14FF,  //!< Last DCCT (DCCT with dcc_id 0xFF).
    };

    //!
    //! Service type in ATSC Virtual Channel Table (VCT)
    //!
    enum : uint8_t {
        ATSC_STYPE_ANALOG_TV = 0x01,  //!< Analog Television
        ATSC_STYPE_DTV       = 0x02,  //!< ATSC Digital Television
        ATSC_STYPE_AUDIO     = 0x03,  //!< ATSC Audio
        ATSC_STYPE_DATA      = 0x04,  //!< ATSC Data Only Service
        ATSC_STYPE_SOFTWARE  = 0x05,  //!< ATSC Software Download Service
    };
}
