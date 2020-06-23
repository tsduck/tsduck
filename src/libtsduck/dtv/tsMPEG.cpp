//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsMPEG.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// These PID sets respectively contains no PID and all PID's.
// The default constructor for PIDSet (std::bitset) sets all bits to 0.
//----------------------------------------------------------------------------

const ts::PIDSet ts::NoPID;
const ts::PIDSet ts::AllPIDs (~NoPID);


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
});


//----------------------------------------------------------------------------
// Check if an ST value indicates a PES stream
//----------------------------------------------------------------------------

bool ts::IsPES (uint8_t st)
{
    return
        st == ST_MPEG1_VIDEO      ||
        st == ST_MPEG2_VIDEO      ||
        st == ST_MPEG1_AUDIO      ||
        st == ST_MPEG2_AUDIO      ||
        st == ST_PES_PRIV         ||
        st == ST_MPEG2_ATM        ||
        st == ST_MPEG4_VIDEO      ||
        st == ST_MPEG4_AUDIO      ||
        st == ST_MPEG4_PES        ||
        st == ST_MDATA_PES        ||
        st == ST_AVC_VIDEO        ||
        st == ST_AAC_AUDIO        ||
        st == ST_AC3_AUDIO        ||
        st == ST_EAC3_AUDIO       ||
        st == ST_HEVC_VIDEO       ||
        st == ST_HEVC_SUBVIDEO    ||
        st == ST_MPEG4_AUDIO_RAW  ||
        st == ST_MPH3D_MAIN       ||
        st == ST_MPH3D_AUX        ||
        st == ST_AUX_VIDEO        ||
        st == ST_AVC_SUBVIDEO_G   ||
        st == ST_AVC_SUBVIDEO_H   ||
        st == ST_J2K_VIDEO        ||
        st == ST_MPEG2_3D_VIEW    ||
        st == ST_AVC_3D_VIEW      ||
        st == ST_AVC_SUBVIDEO_I   ||
        st == ST_HEVC_SUBVIDEO_G  ||
        st == ST_HEVC_SUBVIDEO_TG ||
        st == ST_HEVC_SUBVIDEO_H  ||
        st == ST_HEVC_SUBVIDEO_TH ||
        st == ST_MPEG4_TEXT       ||
        st == ST_EXT_MEDIA;
}


//----------------------------------------------------------------------------
// Check if an ST value indicates a video stream
//----------------------------------------------------------------------------

bool ts::IsVideoST (uint8_t st)
{
    return
        st == ST_MPEG1_VIDEO      ||
        st == ST_MPEG2_VIDEO      ||
        st == ST_MPEG4_VIDEO      ||
        st == ST_AVC_VIDEO        ||
        st == ST_HEVC_VIDEO       ||
        st == ST_HEVC_SUBVIDEO    ||
        st == ST_AUX_VIDEO        ||
        st == ST_AVC_SUBVIDEO_G   ||
        st == ST_AVC_SUBVIDEO_H   ||
        st == ST_J2K_VIDEO        ||
        st == ST_MPEG2_3D_VIEW    ||
        st == ST_AVC_3D_VIEW      ||
        st == ST_AVC_SUBVIDEO_I   ||
        st == ST_HEVC_SUBVIDEO_G  ||
        st == ST_HEVC_SUBVIDEO_TG ||
        st == ST_HEVC_SUBVIDEO_H  ||
        st == ST_HEVC_SUBVIDEO_TH;
}


//----------------------------------------------------------------------------
// Check if an ST value indicates an audio stream
//----------------------------------------------------------------------------

bool ts::IsAudioST (uint8_t st)
{
    return
        st == ST_MPEG1_AUDIO     ||
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

bool ts::IsSectionST (uint8_t st)
{
    return
        st == ST_PRIV_SECT     ||
        st == ST_DSMCC_UN      ||
        st == ST_DSMCC_SECT    ||
        st == ST_MPEG4_SECT    ||
        st == ST_MDATA_SECT    ||
        st == ST_SCTE35_SPLICE ||
        st == ST_GREEN         ||
        st == ST_QUALITY;
}


//----------------------------------------------------------------------------
// Check if a SID value indicates a PES packet with long header
//----------------------------------------------------------------------------

bool ts::IsLongHeaderSID (uint8_t sid)
{
    return
        sid != SID_PSMAP &&    // Program stream map
        sid != SID_PAD &&      // Padding stream
        sid != SID_PRIV2 &&    // Private stream 2
        sid != SID_ECM &&      // ECM stream
        sid != SID_EMM &&      // EMM stream
        sid != SID_PSDIR &&    // Program stream directory
        sid != SID_DSMCC &&    // DSM-CC data
        sid != SID_H222_1_E;   // H.222.1 type E
}


//----------------------------------------------------------------------------
// Compute the PCR of a packet, based on the PCR of a previous packet.
//----------------------------------------------------------------------------

uint64_t ts::NextPCR(uint64_t last_pcr, PacketCounter distance, BitRate bitrate)
{
    if (last_pcr == INVALID_PCR || bitrate == 0) {
        return INVALID_PCR;
    }

    uint64_t next_pcr = last_pcr + (distance * 8 * PKT_SIZE * SYSTEM_CLOCK_FREQ) / uint64_t(bitrate);
    if (next_pcr >= PCR_SCALE) {
        next_pcr -= PCR_SCALE;
    }

    return next_pcr;
}


//----------------------------------------------------------------------------
// Compute the difference between PCR2 and PCR1.
//----------------------------------------------------------------------------

uint64_t ts::DiffPCR(uint64_t pcr1, uint64_t pcr2)
{
    if (pcr1 > MAX_PCR || pcr2 > MAX_PCR) {
        return INVALID_PCR;
    }
    else {
        return pcr2 >= pcr1 ? pcr2 - pcr1 : PCR_SCALE + pcr2 - pcr1;
    }
}

uint64_t ts::DiffPTS(uint64_t pts1, uint64_t pts2)
{
    if (pts1 > MAX_PTS_DTS || pts2 > MAX_PTS_DTS) {
        return INVALID_PTS;
    }
    else {
        return pts2 >= pts1 ? pts2 - pts1 : PTS_DTS_SCALE + pts2 - pts1;
    }
}
