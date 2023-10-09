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
//!  Known video, audio or data encoding formats.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsNamesFile.h"

namespace ts {
    //!
    //! Known video, audio or data encoding formats.
    //! Commonly found in PES packets.
    //!
    enum class CodecType {
        UNDEFINED,     //!< Undefined format.
        MPEG1_VIDEO,   //!< MPEG-1 video, ISO 11172-2, ITU-T Rec H.261.
        MPEG1_AUDIO,   //!< MPEG-1 audio, ISO 11172-3.
        MPEG2_VIDEO,   //!< MPEG-2 video, ISO 13818-2, ITU-T Rec H.262.
        MPEG2_AUDIO,   //!< MPEG-2 audio layer 1 or 2, ISO 13818-3.
        MP3,           //!< MPEG-2 audio layer 3.
        AAC,           //!< Advanced Audio Coding, ISO 13818-7.
        AC3,           //!< Audio Coding 3, Dolby Digital.
        EAC3,          //!< Enhanced Audio Coding 3, Dolby Digital.
        AC4,           //!< Audio Coding 4, Dolby Digital.
        MPEG4_VIDEO,   //!< MPEG-4 video, ISO 14496-2, ITU-T Rec H.263, aka DivX.
        HEAAC,         //!< High Efficiency AAC, ISO 14496-3.
        J2K,           //!< JPEG 2000 video.
        AVC,           //!< Advanced Video Coding, ISO 14496-10, ITU-T Rec. H.264.
        HEVC,          //!< High Efficiency Video Coding, ITU-T Rec. H.265.
        VVC,           //!< Versatile Video Coding, ITU-T Rec. H.266.
        EVC,           //!< Essential Video Coding.
        LCEVC,         //!< Low Complexity Enhancement Video Coding.
        VP9,           //!< Google VP9 video.
        AV1,           //!< Alliance for Open Media Video 1.
        DTS,           //!< Digital Theater Systems audio.
        DTSHD,         //!< HD Digital Theater Systems audio, aka DTS++.
        TELETEXT,      //!< Teletext pages or subtitles, ETSI EN 300 706.
        DVB_SUBTITLES, //!< DVB subtitles, ETSI EN 300 743.
        AVS3,          //!< AVS3 video (AVS is Chinese Audio Video Standards).
    };

    //!
    //! Enumeration description of ts::CodecType (display).
    //! The version is suitable to display codec names.
    //! @see CodecTypeArgEnum
    //!
    TSDUCKDLL extern const Enumeration CodecTypeEnum;

    //!
    //! Enumeration description of ts::CodecType (command line argument).
    //! The version is suitable to define command line arguments taking codec names as parameter.
    //! @see CodecTypeEnum
    //!
    TSDUCKDLL extern const Enumeration CodecTypeArgEnum;

    //!
    //! Check if a codec type value indicates an audio stream.
    //! @param [in] ct Codec type.
    //! @return True if @a ct indicates an audio stream.
    //!
    TSDUCKDLL bool CodecTypeIsAudio(CodecType ct);

    //!
    //! Check if a codec type value indicates a video stream.
    //! @param [in] ct Codec type.
    //! @return True if @a ct indicates a video stream.
    //!
    TSDUCKDLL bool CodecTypeIsVideo(CodecType ct);

    //!
    //! Check if a codec type value indicates a subtitle stream.
    //! @param [in] ct Codec type.
    //! @return True if @a ct indicates a subtitle stream.
    //!
    TSDUCKDLL bool CodecTypeIsSubtitles(CodecType ct);

    //!
    //! Name of AVC/HEVC/VVC access unit (aka "NALunit") type.
    //! @param [in] codec One of AVC, HEVC, VVC.
    //! @param [in] ut Access unit type.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString AccessUnitTypeName(CodecType codec, uint8_t ut, NamesFlags flags = NamesFlags::NAME);
}
