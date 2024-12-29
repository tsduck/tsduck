//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCodecType.h"


//----------------------------------------------------------------------------
// Enumeration description of ts::CodecType.
//----------------------------------------------------------------------------

// This version is suitable to display codec names.
const ts::Enumeration& ts::CodecTypeEnum()
{
    // Thread-safe init-safe static data pattern:
    static const Enumeration data({
        {u"undefined",     CodecType::UNDEFINED},
        {u"MPEG-1 Video",  CodecType::MPEG1_VIDEO},
        {u"MPEG-1 Audio",  CodecType::MPEG1_AUDIO},
        {u"MPEG-2 Video",  CodecType::MPEG2_VIDEO},
        {u"MPEG-2 Audio",  CodecType::MPEG2_AUDIO},
        {u"MP3",           CodecType::MP3},
        {u"AAC",           CodecType::AAC},
        {u"AC3",           CodecType::AC3},
        {u"E-AC3",         CodecType::EAC3},
        {u"AC4",           CodecType::AC4},
        {u"MPEG-4 Video",  CodecType::MPEG4_VIDEO},
        {u"HE-AAC",        CodecType::HEAAC},
        {u"JPEG-2000",     CodecType::J2K},
        {u"AVC H.264",     CodecType::AVC},
        {u"HEVC H.265",    CodecType::HEVC},
        {u"VVC H.266",     CodecType::VVC},
        {u"EVC",           CodecType::EVC},
        {u"LC-EVC",        CodecType::LCEVC},
        {u"VP9",           CodecType::VP9},
        {u"AV1",           CodecType::AV1},
        {u"DTS",           CodecType::DTS},
        {u"DTS-HD",        CodecType::DTSHD},
        {u"Teletext",      CodecType::TELETEXT},
        {u"DVB Subtitles", CodecType::DVB_SUBTITLES},
        {u"AVS3 Video",    CodecType::AVS3_VIDEO},
        {u"AVS2 Audio",    CodecType::AVS2_AUDIO},
        {u"AVS3 Audio",    CodecType::AVS3_AUDIO},
    });
    return data;
}

// This version is suitable to define command line arguments taking codec names as parameter.
const ts::Enumeration& ts::CodecTypeArgEnum()
{
    // Thread-safe init-safe static data pattern:
    static const Enumeration data({
        {u"undefined",     CodecType::UNDEFINED},
        {u"MPEG-1-Video",  CodecType::MPEG1_VIDEO},
        {u"MP1Video",      CodecType::MPEG1_VIDEO},
        {u"MPEG-1-Audio",  CodecType::MPEG1_AUDIO},
        {u"MP1Audio",      CodecType::MPEG1_AUDIO},
        {u"MPEG-2-Video",  CodecType::MPEG2_VIDEO},
        {u"MP2Video",      CodecType::MPEG2_VIDEO},
        {u"MPEG-2-Audio",  CodecType::MPEG2_AUDIO},
        {u"MP2Audio",      CodecType::MPEG2_AUDIO},
        {u"MP3",           CodecType::MP3},
        {u"AAC",           CodecType::AAC},
        {u"AC3",           CodecType::AC3},
        {u"EAC3",          CodecType::EAC3},
        {u"AC4",           CodecType::AC4},
        {u"MPEG-4-Video",  CodecType::MPEG4_VIDEO},
        {u"MP4Video",      CodecType::MPEG4_VIDEO},
        {u"HEAAC",         CodecType::HEAAC},
        {u"JPEG2000",      CodecType::J2K},
        {u"AVC",           CodecType::AVC},
        {u"H264",          CodecType::AVC},
        {u"HEVC",          CodecType::HEVC},
        {u"H265",          CodecType::HEVC},
        {u"VVC",           CodecType::VVC},
        {u"H266",          CodecType::VVC},
        {u"EVC",           CodecType::EVC},
        {u"LCEVC",         CodecType::LCEVC},
        {u"VP9",           CodecType::VP9},
        {u"AV1",           CodecType::AV1},
        {u"DTS",           CodecType::DTS},
        {u"DTSHD",         CodecType::DTSHD},
        {u"Teletext",      CodecType::TELETEXT},
        {u"DVBSubtitles",  CodecType::DVB_SUBTITLES},
        {u"AVS3Video",     CodecType::AVS3_VIDEO},
        {u"AVS2Audio",     CodecType::AVS2_AUDIO},
        {u"AVS3Audio",     CodecType::AVS3_AUDIO},
    });
    return data;
}


//----------------------------------------------------------------------------
// Check if a codec type value indicates a stream of a given type.
//----------------------------------------------------------------------------

bool ts::CodecTypeIsAudio(CodecType ct)
{
    // Thread-safe init-safe static data pattern:
    static const std::set<CodecType> audio_codecs {
        CodecType::MPEG1_AUDIO,
        CodecType::MPEG2_AUDIO,
        CodecType::MP3,
        CodecType::AAC,
        CodecType::AC3,
        CodecType::EAC3,
        CodecType::AC4,
        CodecType::HEAAC,
        CodecType::DTS,
        CodecType::DTSHD,
        CodecType::AVS2_AUDIO,
        CodecType::AVS3_AUDIO,
    };

    return audio_codecs.contains(ct);
}

bool ts::CodecTypeIsVideo(CodecType ct)
{
    // Thread-safe init-safe static data pattern:
    static const std::set<CodecType> video_codecs {
        CodecType::MPEG1_VIDEO,
        CodecType::MPEG2_VIDEO,
        CodecType::MPEG4_VIDEO,
        CodecType::J2K,
        CodecType::AVC,
        CodecType::HEVC,
        CodecType::VVC,
        CodecType::EVC,
        CodecType::LCEVC,
        CodecType::VP9,
        CodecType::AV1,
        CodecType::AVS3_VIDEO,
    };

    return video_codecs.contains(ct);
}

bool ts::CodecTypeIsSubtitles(CodecType ct)
{
    // Thread-safe init-safe static data pattern:
    static const std::set<CodecType> subtitling_types {
        CodecType::TELETEXT,
        CodecType::DVB_SUBTITLES,
    };

    return subtitling_types.contains(ct);
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
