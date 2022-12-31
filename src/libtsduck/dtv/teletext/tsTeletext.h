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
//!  Common definitions for Teletext PES packets.
//!  Reference: ETSI EN 300 472 V1.3.1, "DVB; Specification for conveying
//!  ITU-R System B Teletext in DVB bitstreams"
//!  @see ETSI EN 300 472
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Size in bytes of a Teletext packet.
    //!
    constexpr size_t TELETEXT_PACKET_SIZE = 44;

    //!
    //! First EBU data_identifier value in PES packets conveying Teletext.
    //!
    constexpr uint8_t TELETEXT_PES_FIRST_EBU_DATA_ID = 0x10;

    //!
    //! Last EBU data_identifier value in PES packets conveying Teletext.
    //!
    constexpr uint8_t TELETEXT_PES_LAST_EBU_DATA_ID  = 0x1F;

    //!
    //! Teletext data unit ids.
    //! @see ETSI EN 300 472
    //!
    enum class TeletextDataUnitId : uint8_t {
        NON_SUBTITLE    = 0x02,  //!< Data_unit_id for EBU Teletext non-subtitle data.
        SUBTITLE        = 0x03,  //!< Data_unit_id for EBU Teletext subtitle data.
        INVERTED        = 0x0C,  //!< Data_unit_id for EBU EBU Teletext Inverted (extension ?).
        VPS             = 0xC3,  //!< Data_unit_id for VPS (extension ?).
        CLOSED_CAPTIONS = 0xC5,  //!< Data_unit_id for Closed Caption (extension ?).
        STUFFING        = 0xFF,  //!< Data_unit_id for stuffing data.
    };
}
