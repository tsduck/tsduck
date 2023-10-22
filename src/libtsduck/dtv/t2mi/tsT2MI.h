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
//!  Common definitions for T2-MI (DVB-T2 Modulator Interface).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Size in bytes of a T2-MI packet header.
    //!
    constexpr size_t T2MI_HEADER_SIZE = 6;

    //!
    //! T2-MI packet types.
    //! @see ETSI EN 102 773, section 5.1.
    //!
    enum class T2MIPacketType : uint8_t {
        BASEBAND_FRAME        = 0x00, //!< Baseband Frame.
        AUX_IQ_DATA           = 0x01, //!< Auxiliary stream I/Q data.
        ARBITRARY_CELL        = 0x02, //!< Arbitrary cell insertion.
        L1_CURRENT            = 0x10, //!< L1-current.
        L1_FUTURE             = 0x11, //!< L1-future.
        P2_BIAS_BALANCING     = 0x12, //!< P2 bias balancing cells.
        DVBT2_TIMESTAMP       = 0x20, //!< DVB-T2 timestamp.
        INDIVIDUAL_ADDRESSING = 0x21, //!< Individual addressing.
        FEF_NULL              = 0x30, //!< FEF part: Null.
        FEF_IQ_DATA           = 0x31, //!< FEF part: I/Q data.
        FEF_COMPOSITE         = 0x32, //!< FEF part: composite.
        FEF_SUBPART           = 0x33, //!< FEF sub-part.
        INVALID_TYPE          = 0xFF  //!< Invalid T2MI packet (non standard value).
    };

    //!
    //! Size in bytes of a DVB-T2 Base Band Header.
    //! See ETSI EN 302 765, section 5.1.7.
    //!
    constexpr size_t T2_BBHEADER_SIZE = 10;
}
