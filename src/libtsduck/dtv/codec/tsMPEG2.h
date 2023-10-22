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
//!  Basic definitions for MPEG-2 video coding standard.
//!  @see ISO/IEC 113818-2, H.262
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! PES start code values (after start code prefix 00 00 01)
    //!
    enum : uint8_t {
        PST_PICTURE         = 0x00,  //!< Picture PES start code.
        PST_SLICE_MIN       = 0x01,  //!< First slice PES start code.
        PST_SLICE_MAX       = 0xAF,  //!< Last slice PES start code.
        PST_RESERVED_B0     = 0xB0,  //!< Reserved PES start code.
        PST_RESERVED_B1     = 0xB1,  //!< Reserved PES start code.
        PST_USER_DATA       = 0xB2,  //!< User data PES start code.
        PST_SEQUENCE_HEADER = 0xB3,  //!< Sequence header PES start code.
        PST_SEQUENCE_ERROR  = 0xB4,  //!< Sequence error PES start code.
        PST_EXTENSION       = 0xB5,  //!< Extension PES start code.
        PST_RESERVED_B6     = 0xB6,  //!< Reserved PES start code.
        PST_SEQUENCE_END    = 0xB7,  //!< End of sequence PES start code.
        PST_GROUP           = 0xB8,  //!< Group PES start code.
        PST_SYSTEM_MIN      = 0xB9,  //!< First stream id value (SID_* in tsPES.h).
        PST_SYSTEM_MAX      = 0xFF,  //!< Last stream id value (SID_* in tsPES.h).
    };

    //!
    //! Video macroblock width in pixels.
    //! Valid for:
    //! - ISO 11172-2 (MPEG-1 video)
    //! - ISO 13818-2 (MPEG-2 video)
    //! - ISO 14496-10 (MPEG-4 Advanced Video Coding, AVC, ITU H.264)
    //!
    constexpr size_t MACROBLOCK_WIDTH = 16;

    //!
    //! Video macroblock height in pixels.
    //! @see MACROBLOCK_WIDTH
    //!
    constexpr size_t MACROBLOCK_HEIGHT = 16;

    //!
    //! Frame rate values (in MPEG-1/2 video sequence).
    //!
    enum {
        FPS_23_976 = 0x01,  //!< 23.976 fps (24000/1001)
        FPS_24     = 0x02,  //!< 24 fps
        FPS_25     = 0x03,  //!< 25 fps
        FPS_29_97  = 0x04,  //!< 29.97 fps (30000/1001)
        FPS_30     = 0x05,  //!< 30 fps
        FPS_50     = 0x06,  //!< 50 fps
        FPS_59_94  = 0x07,  //!< 59.94 fps (60000/1001)
        FPS_60     = 0x08,  //!< 60 fps
    };

    //!
    //! Aspect ratio values (in MPEG-1/2 video sequence header).
    //!
    enum {
        AR_SQUARE = 1,  //!< 1/1 MPEG video aspect ratio.
        AR_4_3    = 2,  //!< 4/3 MPEG video aspect ratio.
        AR_16_9   = 3,  //!< 16/9 MPEG video aspect ratio.
        AR_221    = 4,  //!< 2.21/1 MPEG video aspect ratio.
    };

    //!
    //! Chroma format values (in MPEG-1/2 video sequence header).
    //!
    enum {
        CHROMA_MONO = 0,  //!< Monochrome MPEG video.
        CHROMA_420  = 1,  //!< Chroma 4:2:0 MPEG video.
        CHROMA_422  = 2,  //!< Chroma 4:2:2 MPEG video.
        CHROMA_444  = 3,  //!< Chroma 4:4:4 MPEG video.
    };
}
