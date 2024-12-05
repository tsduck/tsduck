//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCodecType.h"
#include "tsAlgorithm.h"

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::CodecTypeEnum, ({
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
    {u"AVS3 Video",    ts::CodecType::AVS3_VIDEO},
    {u"AVS2 Audio",    ts::CodecType::AVS2_AUDIO},
    {u"AVS3 Audio",    ts::CodecType::AVS3_AUDIO},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::CodecTypeArgEnum, ({
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
    {u"AVS3Video",     ts::CodecType::AVS3_VIDEO},
    {u"AVS2Audio",     ts::CodecType::AVS2_AUDIO},
    {u"AVS3Audio",     ts::CodecType::AVS3_AUDIO},
}));

namespace {
    const std::set<ts::CodecType> AudioCodecs {
        ts::CodecType::MPEG1_AUDIO,
        ts::CodecType::MPEG2_AUDIO,
        ts::CodecType::MP3,
        ts::CodecType::AAC,
        ts::CodecType::AC3,
        ts::CodecType::EAC3,
        ts::CodecType::AC4,
        ts::CodecType::HEAAC,
        ts::CodecType::DTS,
        ts::CodecType::DTSHD,
        ts::CodecType::AVS2_AUDIO,
        ts::CodecType::AVS3_AUDIO,
    };

    const std::set<ts::CodecType> VideoCodecs {
        ts::CodecType::MPEG1_VIDEO,
        ts::CodecType::MPEG2_VIDEO,
        ts::CodecType::MPEG4_VIDEO,
        ts::CodecType::J2K,
        ts::CodecType::AVC,
        ts::CodecType::HEVC,
        ts::CodecType::VVC,
        ts::CodecType::EVC,
        ts::CodecType::LCEVC,
        ts::CodecType::VP9,
        ts::CodecType::AV1,
        ts::CodecType::AVS3_VIDEO,
    };

    const std::set<ts::CodecType> SubtitlingTypes {
        ts::CodecType::TELETEXT,
        ts::CodecType::DVB_SUBTITLES,
    };
}


//----------------------------------------------------------------------------
// Check if a codec type value indicates a stream of a given type.
//----------------------------------------------------------------------------

bool ts::CodecTypeIsAudio(CodecType ct)
{
    return Contains(AudioCodecs, ct);
}

bool ts::CodecTypeIsVideo(CodecType ct)
{
    return Contains(VideoCodecs, ct);
}

bool ts::CodecTypeIsSubtitles(CodecType ct)
{
    return Contains(SubtitlingTypes, ct);
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
        return NameFromDTV(table, NamesFile::Value(type), flags);
    }
    else {
        return NamesFile::Formatted(NamesFile::Value(type), u"unknown", flags, 8);
    }
}
