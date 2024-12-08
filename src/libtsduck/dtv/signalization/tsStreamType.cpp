//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStreamType.h"
#include "tsDescriptorList.h"


//----------------------------------------------------------------------------
// Check stream types.
//----------------------------------------------------------------------------

// Check if an ST value indicates a stream carrying sections
bool ts::StreamTypeIsSection(uint8_t st)
{
    return st == ST_PRIV_SECT     ||
           st == ST_DSMCC_UN      ||
           st == ST_DSMCC_SECT    ||
           st == ST_MPEG4_SECT    ||
           st == ST_MDATA_SECT    ||
           st == ST_SCTE35_SPLICE ||
           st == ST_GREEN         ||
           st == ST_QUALITY;
}

// Check if a stream type value indicates a PES stream
bool ts::StreamTypeIsPES(uint8_t st)
{
    return StreamTypeIsVideo(st) ||
           StreamTypeIsAudio(st) ||
           st == ST_PES_PRIV     ||
           st == ST_MPEG2_ATM    ||
           st == ST_MPEG4_PES    ||
           st == ST_MDATA_PES    ||
           st == ST_MPEG4_TEXT   ||
           st == ST_EXT_MEDIA;
}

// Check if a stream type value indicates a video stream
bool ts::StreamTypeIsVideo(uint8_t st)
{
    return st == ST_MPEG1_VIDEO   ||
           st == ST_MPEG2_VIDEO   ||
           st == ST_MPEG4_VIDEO   ||
           st == ST_AUX_VIDEO     ||
           st == ST_J2K_VIDEO     ||
           st == ST_MPEG2_3D_VIEW ||
           StreamTypeIsAVC(st)    ||
           StreamTypeIsHEVC(st)   ||
           StreamTypeIsVVC(st)    ||
           st == ST_JPEG_XS_VIDEO ||
           st == ST_EVC_VIDEO     ||
           st == ST_LCEVC_VIDEO   ||
           st == ST_AVS3_VIDEO;
}

// Check if a stream type value indicates a video stream using AVC encoding.
bool ts::StreamTypeIsAVC(uint8_t st)
{
    return st == ST_AVC_VIDEO      ||
           st == ST_AVC_SUBVIDEO_G ||
           st == ST_AVC_SUBVIDEO_H ||
           st == ST_AVC_SUBVIDEO_I ||
           st == ST_AVC_3D_VIEW;
}

// Check if a stream type value indicates a video stream using HEVC encoding.
bool ts::StreamTypeIsHEVC(uint8_t st)
{
    return st == ST_HEVC_VIDEO       ||
           st == ST_HEVC_SUBVIDEO    ||
           st == ST_HEVC_SUBVIDEO_G  ||
           st == ST_HEVC_SUBVIDEO_TG ||
           st == ST_HEVC_SUBVIDEO_H  ||
           st == ST_HEVC_SUBVIDEO_TH ||
           st == ST_HEVC_TILESET;
}

// Check if a stream type value indicates a video stream using VVC encoding.
bool ts::StreamTypeIsVVC(uint8_t st)
{
    return st == ST_VVC_VIDEO ||
           st == ST_VVC_VIDEO_SUBSET;
}

// Check if an ST value indicates an audio stream
bool ts::StreamTypeIsAudio(uint8_t st, REGID regid)
{
    bool audio = false;
    if (regid == REGID_HDMV) {
        audio = st == ST_LPCM_AUDIO       ||
                st == ST_HDMV_AC3         ||
                st == ST_DTS_AUDIO        ||
                st == ST_HDMV_AC3_TRUEHD  ||
                st == ST_HDMV_AC3_PLUS    ||
                st == ST_DTS_HS_AUDIO     ||
                st == ST_DTS_HD_MA_AUDIO  ||
                st == ST_HDMV_EAC3        ||
                st == ST_DTS_AUDIO_8A     ||
                st == ST_SDDS_AUDIO       ||
                st == ST_HDMV_AC3_PLS_SEC ||
                st == ST_DTS_HD_SEC;
    }
    return audio                     ||
           st == ST_MPEG1_AUDIO      ||
           st == ST_MPEG2_AUDIO      ||
           st == ST_MPEG4_AUDIO      ||
           st == ST_AAC_AUDIO        ||
           st == ST_AC3_AUDIO        ||
           st == ST_AC3_PLUS_AUDIO   ||
           st == ST_AC3_TRUEHD_AUDIO ||
           st == ST_A52B_AC3_AUDIO   ||
           st == ST_EAC3_AUDIO       ||
           st == ST_MPEG4_AUDIO_RAW  ||
           st == ST_MPH3D_MAIN       ||
           st == ST_MPH3D_AUX        ||
           st == ST_AVS2_AUDIO       ||
           st == ST_AVS3_AUDIO;
}

// Check if an ST value indicates an audio stream (with descriptor list).
bool ts::StreamTypeIsAudio(uint8_t st, const DescriptorList& dlist)
{
    return StreamTypeIsAudio(st, dlist.containsRegistration(REGID_HDMV) ? REGID_HDMV : REGID_NULL);
}


//----------------------------------------------------------------------------
// Name of a Stream type value.
//----------------------------------------------------------------------------

ts::UString ts::StreamTypeName(uint8_t st, NamesFlags flags, REGID regid)
{
    const NamesFile::NamesFilePtr repo = NamesFile::Instance(NamesFile::Predefined::DTV);
    NamesFile::Value value = (NamesFile::Value(regid) << 8) | NamesFile::Value(st);
    if (regid == REGID_NULL || !repo->nameExists(u"StreamType", value)) {
        // No value found with registration id, use the stream type alone.
        value = NamesFile::Value(st);
    }
    return repo->nameFromSection(u"StreamType", value, flags);
}

ts::UString ts::StreamTypeName(uint8_t st, NamesFlags flags, const DescriptorList& dlist)
{
    const NamesFile::NamesFilePtr repo = NamesFile::Instance(NamesFile::Predefined::DTV);

    // Get all registration ids from the descriptor list.
    std::vector<REGID> regids;
    dlist.getAllRegistrations(regids);

    // The default value is the stream type alone.
    NamesFile::Value value = NamesFile::Value(st);

    // Check all registration ids to see if there is one stream type with that id.
    for (auto id : regids) {
        const NamesFile::Value full = (NamesFile::Value(id) << 8) | NamesFile::Value(st);
        if (repo->nameExists(u"StreamType", full)) {
            value = full;
            break;
        }
    }

    return repo->nameFromSection(u"StreamType", value, flags);
}
