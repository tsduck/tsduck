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
//!  Common definitions for MPEG PES (Packetized Elementary Stream) layer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! PES packet start code prefix (24 bits).
    //!
    constexpr uint32_t PES_START = 0x000001;


    //---------------------------------------------------------------------
    //! Stream id values, as used in PES header.
    //---------------------------------------------------------------------

    enum : uint8_t {
        SID_PSMAP      = 0xBC, //!< Stream id for Program stream map
        SID_PRIV1      = 0xBD, //!< Stream id for Private stream 1
        SID_PAD        = 0xBE, //!< Stream id for Padding stream
        SID_PRIV2      = 0xBF, //!< Stream id for Private stream 2
        SID_AUDIO      = 0xC0, //!< Stream id for Audio stream, with number
        SID_AUDIO_MASK = 0x1F, //!< Stream id for Mask to get audio stream number
        SID_VIDEO      = 0xE0, //!< Stream id for Video stream, with number
        SID_VIDEO_MASK = 0x0F, //!< Stream id for Mask to get video stream number
        SID_ECM        = 0xF0, //!< Stream id for ECM stream
        SID_EMM        = 0xF1, //!< Stream id for EMM stream
        SID_DSMCC      = 0xF2, //!< Stream id for DSM-CC data
        SID_ISO13522   = 0xF3, //!< Stream id for ISO 13522 (hypermedia)
        SID_H222_1_A   = 0xF4, //!< Stream id for H.222.1 type A
        SID_H222_1_B   = 0xF5, //!< Stream id for H.222.1 type B
        SID_H222_1_C   = 0xF6, //!< Stream id for H.222.1 type C
        SID_H222_1_D   = 0xF7, //!< Stream id for H.222.1 type D
        SID_H222_1_E   = 0xF8, //!< Stream id for H.222.1 type E
        SID_ANCILLARY  = 0xF9, //!< Stream id for Ancillary stream
        SID_MP4_SLPACK = 0xFA, //!< Stream id for MPEG-4 SL-packetized stream
        SID_MP4_M4MUX  = 0xFB, //!< Stream id for MPEG-4 M4Mux stream
        SID_METADATA   = 0xFC, //!< Stream id for MPEG-7 metadata stream
        SID_EXTENDED   = 0xFD, //!< Stream id for Extended stream id
        SID_RESERVED   = 0xFE, //!< Stream id for Reserved value
        SID_PSDIR      = 0xFF, //!< Stream id for Program stream directory
    };

    //!
    //! Check if a stream id value indicates a video stream.
    //! @param [in] sid Stream id as found in a PES header.
    //! @return True if @a sid indicates a video stream.
    //!
    TSDUCKDLL inline bool IsVideoSID(uint8_t sid)
    {
        return (sid & ~SID_VIDEO_MASK) == SID_VIDEO;
    }

    //!
    //! Check if a stream id value indicates an audio stream.
    //! @param [in] sid Stream id as found in a PES header.
    //! @return True if @a sid indicates an audio stream.
    //!
    TSDUCKDLL inline bool IsAudioSID(uint8_t sid)
    {
        return (sid & ~SID_AUDIO_MASK) == SID_AUDIO;
    }

    //!
    //! Check if a stream id value indicates a PES packet with long header.
    //! @param [in] sid Stream id as found in a PES header.
    //! @return True if @a sid indicates a PES packet with long header.
    //!
    TSDUCKDLL bool IsLongHeaderSID(uint8_t sid);
}
