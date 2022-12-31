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

#include "tsPSI.h"


//----------------------------------------------------------------------------
// Enumeration description of TableScope values.
//----------------------------------------------------------------------------

const ts::Enumeration ts::TableScopeEnum({
    {u"none",   int(ts::TableScope::NONE)},
    {u"actual", int(ts::TableScope::ACTUAL)},
    {u"all",    int(ts::TableScope::ALL)},
});


//----------------------------------------------------------------------------
// Enumeration description of PDS values.
//----------------------------------------------------------------------------

const ts::Enumeration ts::PrivateDataSpecifierEnum({
    {u"BskyB",     ts::PDS_BSKYB},
    {u"Nagra",     ts::PDS_NAGRA},
    {u"TPS",       ts::PDS_TPS},
    {u"EACEM",     ts::PDS_EACEM},
    {u"EICTA",     ts::PDS_EICTA},  // same value as EACEM
    {u"NorDig",    ts::PDS_NORDIG},
    {u"Logiways",  ts::PDS_LOGIWAYS},
    {u"CanalPlus", ts::PDS_CANALPLUS},
    {u"Eutelsat",  ts::PDS_EUTELSAT},
    {u"OFCOM",     ts::PDS_OFCOM},
    {u"AVS",       ts::PDS_AVS},
    {u"AOM",       ts::PDS_AOM},
});


//----------------------------------------------------------------------------
// Check if a stream type value indicates a PES stream
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Check if a stream type value indicates a video stream
//----------------------------------------------------------------------------

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
           st == ST_LCEVC_VIDEO;
}


//----------------------------------------------------------------------------
// Check if a stream type value indicates a video stream using AVC encoding.
//----------------------------------------------------------------------------

bool ts::StreamTypeIsAVC(uint8_t st)
{
    return st == ST_AVC_VIDEO      ||
           st == ST_AVC_SUBVIDEO_G ||
           st == ST_AVC_SUBVIDEO_H ||
           st == ST_AVC_SUBVIDEO_I ||
           st == ST_AVC_3D_VIEW;
}


//----------------------------------------------------------------------------
// Check if a stream type value indicates a video stream using HEVC encoding.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Check if a stream type value indicates a video stream using VVC encoding.
//----------------------------------------------------------------------------

bool ts::StreamTypeIsVVC(uint8_t st)
{
    // Warning: at this time, the stream types for VVC / H.266 are still unclear.
    // Be sure to verity this on further versions of the ISO 13818-1 standard.
    return st == ST_VVC_VIDEO        ||
           st == ST_VVC_VIDEO_SUBSET;
}


//----------------------------------------------------------------------------
// Check if an ST value indicates an audio stream
//----------------------------------------------------------------------------

bool ts::StreamTypeIsAudio(uint8_t st)
{
    return st == ST_MPEG1_AUDIO     ||
           st == ST_MPEG2_AUDIO     ||
           st == ST_MPEG4_AUDIO     ||
           st == ST_AAC_AUDIO       ||
           st == ST_AC3_AUDIO       ||
           st == ST_EAC3_AUDIO      ||
           st == ST_MPEG4_AUDIO_RAW ||
           st == ST_MPH3D_MAIN      ||
           st == ST_MPH3D_AUX;
}


//----------------------------------------------------------------------------
// Check if an ST value indicates a stream carrying sections
//----------------------------------------------------------------------------

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
