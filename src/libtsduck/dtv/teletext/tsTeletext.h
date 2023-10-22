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
