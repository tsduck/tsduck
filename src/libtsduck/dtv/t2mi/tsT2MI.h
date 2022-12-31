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
