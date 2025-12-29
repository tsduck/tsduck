//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Stream type values, as used in the PMT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsREGID.h"
#include "tsNames.h"

namespace ts {

    class DuckContext;
    class DescriptorList;

    //!
    //! Stream type values, as used in the PMT.
    //!
    enum : uint8_t {
        ST_NULL             = 0x00, //!< Invalid stream type value, used to indicate an absence of value
        ST_MPEG1_VIDEO      = 0x01, //!< MPEG-1 Video
        ST_MPEG2_VIDEO      = 0x02, //!< MPEG-2 Video
        ST_MPEG1_AUDIO      = 0x03, //!< MPEG-1 Audio
        ST_MPEG2_AUDIO      = 0x04, //!< MPEG-2 Audio
        ST_PRIV_SECT        = 0x05, //!< MPEG-2 Private sections
        ST_PES_PRIV         = 0x06, //!< MPEG-2 PES private data
        ST_MHEG             = 0x07, //!< MHEG
        ST_DSMCC            = 0x08, //!< DSM-CC
        ST_MPEG2_ATM        = 0x09, //!< MPEG-2 over ATM
        ST_DSMCC_MPE        = 0x0A, //!< DSM-CC Multi-Protocol Encapsulation
        ST_DSMCC_UN         = 0x0B, //!< DSM-CC User-to-Network messages
        ST_DSMCC_SD         = 0x0C, //!< DSM-CC Stream Descriptors
        ST_DSMCC_SECT       = 0x0D, //!< DSM-CC Sections
        ST_MPEG2_AUX        = 0x0E, //!< MPEG-2 Auxiliary
        ST_AAC_AUDIO        = 0x0F, //!< Advanced Audio Coding (ISO 13818-7)
        ST_MPEG4_VIDEO      = 0x10, //!< MPEG-4 Video
        ST_MPEG4_AUDIO      = 0x11, //!< MPEG-4 Audio
        ST_MPEG4_PES        = 0x12, //!< MPEG-4 SL or M4Mux in PES packets
        ST_MPEG4_SECT       = 0x13, //!< MPEG-4 SL or M4Mux in sections
        ST_DSMCC_DLOAD      = 0x14, //!< DSM-CC Synchronized Download Protocol
        ST_MDATA_PES        = 0x15, //!< MPEG-7 MetaData in PES packets
        ST_MDATA_SECT       = 0x16, //!< MPEG-7 MetaData in sections
        ST_MDATA_DC         = 0x17, //!< MPEG-7 MetaData in DSM-CC Data Carousel
        ST_MDATA_OC         = 0x18, //!< MPEG-7 MetaData in DSM-CC Object Carousel
        ST_MDATA_DLOAD      = 0x19, //!< MPEG-7 MetaData in DSM-CC Sync Downl Proto
        ST_MPEG2_IPMP       = 0x1A, //!< MPEG-2 IPMP stream
        ST_AVC_VIDEO        = 0x1B, //!< AVC video
        ST_MPEG4_AUDIO_RAW  = 0x1C, //!< ISO/IEC 14496-3 Audio, without using any additional transport syntax, such as DST, ALS and SLS.
        ST_MPEG4_TEXT       = 0x1D, //!< ISO/IEC 14496-17 Text
        ST_AUX_VIDEO        = 0x1E, //!< Auxiliary video stream as defined in ISO/IEC 23002-3
        ST_AVC_SUBVIDEO_G   = 0x1F, //!< SVC video sub-bitstream of an AVC video stream, Annex G of ISO 14496-10
        ST_AVC_SUBVIDEO_H   = 0x20, //!< MVC video sub-bitstream of an AVC video stream, Annex H of ISO 14496-10
        ST_J2K_VIDEO        = 0x21, //!< JPEG 2000 video stream ISO/IEC 15444-1
        ST_MPEG2_3D_VIEW    = 0x22, //!< Additional view ISO/IEC 13818-2 video stream for stereoscopic 3D services
        ST_AVC_3D_VIEW      = 0x23, //!< Additional view ISO/IEC 14496-10 video stream for stereoscopic 3D services
        ST_HEVC_VIDEO       = 0x24, //!< HEVC video
        ST_HEVC_SUBVIDEO    = 0x25, //!< HEVC temporal video subset of an HEVC video stream
        ST_AVC_SUBVIDEO_I   = 0x26, //!< MVCD video sub-bitstream of an AVC video stream, Annex I of ISO 14496-10
        ST_EXT_MEDIA        = 0x27, //!< Timeline and External Media Information Stream
        ST_HEVC_SUBVIDEO_G  = 0x28, //!< HEVC enhancement sub-partition, Annex G of ISO 23008-2
        ST_HEVC_SUBVIDEO_TG = 0x29, //!< HEVC temporal enhancement sub-partition, Annex G of ISO 23008-2
        ST_HEVC_SUBVIDEO_H  = 0x2A, //!< HEVC enhancement sub-partition, Annex H of ISO 23008-2
        ST_HEVC_SUBVIDEO_TH = 0x2B, //!< HEVC temporal enhancement sub-partition, Annex H of ISO 23008 - 2
        ST_GREEN            = 0x2C, //!< Green access units carried in MPEG-2 sections
        ST_MPH3D_MAIN       = 0x2D, //!< ISO 23008-3 Audio with MHAS transport syntax - main stream
        ST_MPH3D_AUX        = 0x2E, //!< ISO 23008-3 Audio with MHAS transport syntax - auxiliary stream
        ST_QUALITY          = 0x2F, //!< Quality access units carried in sections
        ST_MEDIA_ORCHESTR   = 0x30, //!< Media Orchestration Access Units carried in sections
        ST_HEVC_TILESET     = 0x31, //!< HEVC substream containing Motion Constrained Tile Set
        ST_JPEG_XS_VIDEO    = 0x32, //!< JPEG XS video stream conforming to ISO/IEC 21122-2
        ST_VVC_VIDEO        = 0x33, //!< VVC/H.266 video or VVC/H.266 temporal subvideo
        ST_VVC_VIDEO_SUBSET = 0x34, //!< VVC/H.266 temporal video subset of a VVC video stream
        ST_EVC_VIDEO        = 0x35, //!< EVC video or EVC temporal sub-video
        ST_LCEVC_VIDEO      = 0x36, //!< LCEVC video stream according to ISO/IEC 23094-2
        ST_CHINESE_VIDEO    = 0x42, //!< Chinese Video Standard
        ST_IPMP             = 0x7F, //!< IPMP stream
        ST_DGC_II_VIDEO     = 0x80, //!< DigiCipher II Video
        ST_AC3_AUDIO        = 0x81, //!< AC-3 Audio (ATSC only)
        ST_AC3_TRUEHD_AUDIO = 0x83, //!< ATSC AC-3 True HD Audio
        ST_AC3_PLUS_AUDIO   = 0x84, //!< ATSC AC-3+ Audio
        ST_SCTE35_SPLICE    = 0x86, //!< SCTE 35 splice information tables
        ST_EAC3_AUDIO       = 0x87, //!< Enhanced-AC-3 Audio (ATSC only)
        ST_A52B_AC3_AUDIO   = 0x91, //!< A52b/AC-3 Audio
        ST_MS_VIDEO         = 0xA0, //!< MSCODEC Video
        ST_VC1              = 0xEA, //!< Private ES (VC-1)

        // Valid after a "HDMV" registration descriptor.

        ST_LPCM_AUDIO       = 0x80, //!< LPCM Audio
        ST_HDMV_AC3         = 0x81, //!< HDMV AC-3 Audio
        ST_DTS_AUDIO        = 0x82, //!< HDMV DTS Audio
        ST_HDMV_AC3_TRUEHD  = 0x83, //!< HDMV AC-3 True HD Audio
        ST_HDMV_AC3_PLUS    = 0x84, //!< HDMV AC-3+ Audio
        ST_DTS_HS_AUDIO     = 0x85, //!< DTS-HD Audio
        ST_DTS_HD_MA_AUDIO  = 0x86, //!< DTS-HD Master Audio
        ST_HDMV_EAC3        = 0x87, //!< HDMV Enhanced-AC-3 Audio
        ST_DTS_AUDIO_8A     = 0x8A, //!< DTS Audio
        ST_SUBPIC_PGS       = 0x90, //!< Subpicture PGS
        ST_IGS              = 0x91, //!< IGS
        ST_DVD_SUBTITLES    = 0x92, //!< DVD_SPU vls Subtitle
        ST_SDDS_AUDIO       = 0x94, //!< SDDS Audio
        ST_HDMV_AC3_PLS_SEC = 0xA1, //!< HDMV AC-3+ Secondary Audio
        ST_DTS_HD_SEC       = 0xA2, //!< DTS-HD Secondary Audio

        // Valid after an appropriate AVS registration descriptor.

        ST_AVS2_VIDEO       = 0xD2, //!< AVS2 video
        ST_AVS2_AUDIO       = 0xD3, //!< AVS2 audio
        ST_AVS3_VIDEO       = 0xD4, //!< AVS3 video
        ST_AVS3_AUDIO       = 0xD5, //!< AVS3 audio
    };

    //!
    //! Check if a stream type value indicates a stream carrying sections.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates a stream carrying sections.
    //!
    TSDUCKDLL bool StreamTypeIsSection(uint8_t st);

    //!
    //! Check if a stream type value indicates a PES stream.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates a PES stream.
    //!
    TSDUCKDLL bool StreamTypeIsPES(uint8_t st);

    //!
    //! Check if a stream type value indicates a video stream.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates a video stream.
    //!
    TSDUCKDLL bool StreamTypeIsVideo(uint8_t st);

    //!
    //! Check if a stream type value indicates a video stream using AVC / H.264 encoding.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates an AVC / H.264 video stream.
    //!
    TSDUCKDLL bool StreamTypeIsAVC(uint8_t st);

    //!
    //! Check if a stream type value indicates a video stream using HEVC / H.265 encoding.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates an HEVC / H.265 video stream.
    //!
    TSDUCKDLL bool StreamTypeIsHEVC(uint8_t st);

    //!
    //! Check if a stream type value indicates a video stream using VVC / H.266 encoding.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates a VVC / H.266 video stream.
    //!
    TSDUCKDLL bool StreamTypeIsVVC(uint8_t st);

    //!
    //! Check if a stream type value indicates an audio stream.
    //! @param [in] st Stream type as used in the PMT.
    //! @return True if @a st indicates an audio stream.
    //!
    TSDUCKDLL bool StreamTypeIsAudio(uint8_t st);

    //!
    //! Check if a stream type value indicates an audio stream.
    //! @param [in] st Stream type as used in the PMT.
    //! @param [in] regids Set of previous registration id from registration descriptors.
    //! @return True if @a st indicates an audio stream.
    //!
    TSDUCKDLL bool StreamTypeIsAudio(uint8_t st, const std::set<REGID>& regids);

    //!
    //! Check if a stream type value indicates an audio stream.
    //! @param [in] st Stream type as used in the PMT.
    //! @param [in] dlist A descriptor list where registration ids are searched.
    //! @return True if @a st indicates an audio stream.
    //!
    TSDUCKDLL bool StreamTypeIsAudio(uint8_t st, const DescriptorList& dlist);

    //!
    //! Name of a Stream type value.
    //! @param [in] st Stream type (in PMT).
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString StreamTypeName(uint8_t st, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of a Stream type value.
    //! @param [in] st Stream type (in PMT).
    //! @param [in] regids Set of previous registration id from registration descriptors.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString StreamTypeName(uint8_t st, const REGIDVector& regids, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of a Stream type value.
    //! @param [in] st Stream type (in PMT).
    //! @param [in] duck TSDuck execution context.
    //! @param [in] dlist A descriptor list where registration ids are searched.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString StreamTypeName(uint8_t st, const DuckContext& duck, const DescriptorList& dlist, NamesFlags flags = NamesFlags::NAME);
}
