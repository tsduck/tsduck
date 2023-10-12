//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCodecType.h"

const ts::Enumeration ts::CodecTypeEnum({
    {u"undefined",     ts::CodecType::UNDEFINED},
    {u"MPEG-1 Video",  ts::CodecType::MPEG1_VIDEO},
    {u"MPEG-1 Audio",  ts::CodecType::MPEG1_AUDIO},
    {u"MPEG-2 Video",  ts::CodecType::MPEG2_VIDEO},
    {u"MPEG-2 Audio",  ts::CodecType::MPEG2_AUDIO},
    {u"MP3",           ts::CodecType::MP3},
    {u"AAC",           ts::CodecType::AAC},
    {u"AC3",           ts::CodecType::AC3},
    {u"E-AC3",         ts::CodecType::EAC3},
    {u"AC4",           ts::CodecType::AC4},
    {u"MPEG-4 Video",  ts::CodecType::MPEG4_VIDEO},
    {u"HE-AAC",        ts::CodecType::HEAAC},
    {u"JPEG-2000",     ts::CodecType::J2K},
    {u"AVC H.264",     ts::CodecType::AVC},
    {u"HEVC H.265",    ts::CodecType::HEVC},
    {u"VVC H.266",     ts::CodecType::VVC},
    {u"EVC",           ts::CodecType::EVC},
    {u"LC-EVC",        ts::CodecType::LCEVC},
    {u"VP9",           ts::CodecType::VP9},
    {u"AV1",           ts::CodecType::AV1},
    {u"DTS",           ts::CodecType::DTS},
    {u"DTS-HD",        ts::CodecType::DTSHD},
    {u"Teletext",      ts::CodecType::TELETEXT},
    {u"DVB Subtitles", ts::CodecType::DVB_SUBTITLES},
    {u"AVS3",          ts::CodecType::AVS3},
});

const ts::Enumeration ts::CodecTypeArgEnum({
    {u"undefined",     ts::CodecType::UNDEFINED},
    {u"MPEG-1-Video",  ts::CodecType::MPEG1_VIDEO},
    {u"MP1Video",      ts::CodecType::MPEG1_VIDEO},
    {u"MPEG-1-Audio",  ts::CodecType::MPEG1_AUDIO},
    {u"MP1Audio",      ts::CodecType::MPEG1_AUDIO},
    {u"MPEG-2-Video",  ts::CodecType::MPEG2_VIDEO},
    {u"MP2Video",      ts::CodecType::MPEG2_VIDEO},
    {u"MPEG-2-Audio",  ts::CodecType::MPEG2_AUDIO},
    {u"MP2Audio",      ts::CodecType::MPEG2_AUDIO},
    {u"MP3",           ts::CodecType::MP3},
    {u"AAC",           ts::CodecType::AAC},
    {u"AC3",           ts::CodecType::AC3},
    {u"EAC3",          ts::CodecType::EAC3},
    {u"AC4",           ts::CodecType::AC4},
    {u"MPEG-4-Video",  ts::CodecType::MPEG4_VIDEO},
    {u"MP4Video",      ts::CodecType::MPEG4_VIDEO},
    {u"HEAAC",         ts::CodecType::HEAAC},
    {u"JPEG2000",      ts::CodecType::J2K},
    {u"AVC",           ts::CodecType::AVC},
    {u"H264",          ts::CodecType::AVC},
    {u"HEVC",          ts::CodecType::HEVC},
    {u"H265",          ts::CodecType::HEVC},
    {u"VVC",           ts::CodecType::VVC},
    {u"H266",          ts::CodecType::VVC},
    {u"EVC",           ts::CodecType::EVC},
    {u"LCEVC",         ts::CodecType::LCEVC},
    {u"VP9",           ts::CodecType::VP9},
    {u"AV1",           ts::CodecType::AV1},
    {u"DTS",           ts::CodecType::DTS},
    {u"DTSHD",         ts::CodecType::DTSHD},
    {u"Teletext",      ts::CodecType::TELETEXT},
    {u"DVBSubtitles",  ts::CodecType::DVB_SUBTITLES},
    {u"AVS3",          ts::CodecType::AVS3},
});


//----------------------------------------------------------------------------
// Check if a codec type value indicates an audio stream.
//----------------------------------------------------------------------------

bool ts::CodecTypeIsAudio(CodecType ct)
{
    switch (ct) {
        case CodecType::MPEG1_AUDIO:
        case CodecType::MPEG2_AUDIO:
        case CodecType::MP3:
        case CodecType::AAC:
        case CodecType::AC3:
        case CodecType::EAC3:
        case CodecType::AC4:
        case CodecType::HEAAC:
        case CodecType::DTS:
        case CodecType::DTSHD:
            return true;

        case CodecType::UNDEFINED:
        case CodecType::MPEG1_VIDEO:
        case CodecType::MPEG2_VIDEO:
        case CodecType::MPEG4_VIDEO:
        case CodecType::J2K:
        case CodecType::AVC:
        case CodecType::HEVC:
        case CodecType::VVC:
        case CodecType::EVC:
        case CodecType::LCEVC:
        case CodecType::VP9:
        case CodecType::AV1:
        case CodecType::TELETEXT:
        case CodecType::DVB_SUBTITLES:
        case CodecType::AVS3:
        default:
            return false;
    }
}


//----------------------------------------------------------------------------
// Check if a codec type value indicates a video stream.
//----------------------------------------------------------------------------

bool ts::CodecTypeIsVideo(CodecType ct)
{
    switch (ct) {
        case CodecType::MPEG1_VIDEO:
        case CodecType::MPEG2_VIDEO:
        case CodecType::MPEG4_VIDEO:
        case CodecType::J2K:
        case CodecType::AVC:
        case CodecType::HEVC:
        case CodecType::VVC:
        case CodecType::EVC:
        case CodecType::LCEVC:
        case CodecType::VP9:
        case CodecType::AV1:
        case CodecType::AVS3:
            return true;

        case CodecType::UNDEFINED:
        case CodecType::MPEG1_AUDIO:
        case CodecType::MPEG2_AUDIO:
        case CodecType::MP3:
        case CodecType::AAC:
        case CodecType::AC3:
        case CodecType::EAC3:
        case CodecType::AC4:
        case CodecType::HEAAC:
        case CodecType::DTS:
        case CodecType::DTSHD:
        case CodecType::TELETEXT:
        case CodecType::DVB_SUBTITLES:
        default:
            return false;
    }
}


//----------------------------------------------------------------------------
// Check if a codec type value indicates a subtitle stream.
//----------------------------------------------------------------------------

bool ts::CodecTypeIsSubtitles(CodecType ct)
{
    switch (ct) {
        case CodecType::TELETEXT:
        case CodecType::DVB_SUBTITLES:
            return true;

        case CodecType::UNDEFINED:
        case CodecType::MPEG1_VIDEO:
        case CodecType::MPEG1_AUDIO:
        case CodecType::MPEG2_VIDEO:
        case CodecType::MPEG2_AUDIO:
        case CodecType::MP3:
        case CodecType::AAC:
        case CodecType::AC3:
        case CodecType::EAC3:
        case CodecType::AC4:
        case CodecType::MPEG4_VIDEO:
        case CodecType::HEAAC:
        case CodecType::J2K:
        case CodecType::AVC:
        case CodecType::HEVC:
        case CodecType::VVC:
        case CodecType::EVC:
        case CodecType::LCEVC:
        case CodecType::VP9:
        case CodecType::AV1:
        case CodecType::DTS:
        case CodecType::DTSHD:
        case CodecType::AVS3:
        default:
            return false;
    }
}


//----------------------------------------------------------------------------
// Name of AVC/HEVC/VVC access unit (aka "NALunit") type.
//----------------------------------------------------------------------------

ts::UString ts::AccessUnitTypeName(CodecType codec, uint8_t type, NamesFlags flags)
{
    const UChar* table = nullptr;
    if (codec == CodecType::AVC) {
        table = u"avc.unit_type";
    }
    else if (codec == CodecType::HEVC) {
        table = u"hevc.unit_type";
    }
    else if (codec == CodecType::VVC) {
        table = u"vvc.unit_type";
    }
    if (table != nullptr) {
        return NameFromDTV(table, NamesFile::Value(type), flags, 8);
    }
    else {
        return NamesFile::Formatted(NamesFile::Value(type), u"unknown", flags, 8);
    }
}
