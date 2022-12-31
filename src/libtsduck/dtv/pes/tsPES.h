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
        SID_MP4_FLEXM  = 0xFB, //!< Stream id for MPEG-4 FlexMux stream
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
