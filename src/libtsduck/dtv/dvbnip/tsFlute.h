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
//!  Generic FLUTE definitions (File Delivery over Unidirectional Transport).
//!  @see IETF RFC 3926
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! LCT Transport Object Identifier for FLUTE File Delivery Table (FDT).
    //! @see IETF RFC 3926, section 3.3
    //!
    constexpr uint64_t FLUTE_FDT_TOI = 0;

    //!
    //! FEC Encoding ID values for FLUTE and LCT.
    //! @see IETF RFC 3926, section 5.1.4
    //!
    enum : uint8_t {
        FEI_COMPACT_NOCODE =   0,  //!< Compact No-Code FEC (Fully-Specified)
        FEI_EXPANDABLE     = 128,  //!< Small Block, Large Block and Expandable FEC (Under-Specified)
        FEI_SMALL_BLOCK    = 129,  //!< Small Block Systematic FEC (Under-Specified)
        FEI_COMPACT        = 130,  //!< Compact FEC (Under-Secified)
    };

    //!
    //! Header Extension Types (HET) for Layered Coding Transport (LCT).
    //!
    enum : uint8_t {
        HET_MIN_VAR_SIZE   =   0,  //!< Min type value for variable-size header extensions.
        HET_NOP            =   0,  //!< No-Operation extension.
        HET_AUTH           =   1,  //!< Packet authentication extension.
        HET_TIME           =   2,  //!< Time extension.
        HET_FTI            =  64,  //!< FEC Object Transmission Information extension (ALC, RFC 5775).
        HET_NACI           =  68,  //!< NIP Actual Carrier Information (DVB-NIP, ETSI TS 103 876).
        HET_MAX_VAR_SIZE   = 127,  //!< Max type value for variable-size header extensions.
        HET_MIN_FIXED_SIZE = 128,  //!< Min type value for fixed-size header extensions (24-bit payload).
        HET_FDT            = 192,  //!< FDT Instance Header (FLUTE, RFC 3926).
        HET_CENC           = 193,  //!< FDT Instance Content Encoding extension (FLUTE, RFC 3926).
        HET_MAX_FIXED_SIZE = 255,  //!< Max type value for fixed-size header extensions (24-bit payload).
    };
}
