//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshls.h"


//----------------------------------------------------------------------------
// Enumeration description of ts::hls::PlayListType.
//----------------------------------------------------------------------------

const ts::Names& ts::hls::PlayListTypeNames()
{
    static const Names data {
        {u"Unknown", PlayListType::UNKNOWN},
        {u"Master",  PlayListType::MASTER},
        {u"VoD",     PlayListType::VOD},
        {u"Event",   PlayListType::EVENT},
        {u"Live",    PlayListType::LIVE},
    };
    return data;
}


//----------------------------------------------------------------------------
// Enumeration description of ts::hls::Tag.
//----------------------------------------------------------------------------

const ts::Names& ts::hls::TagNames()
{
    static const Names data {
        {u"EXTM3U",                       Tag::EXTM3U},
        {u"EXT-X-VERSION",                Tag::VERSION},
        {u"EXTINF",                       Tag::EXTINF},
        {u"EXT-X-BYTERANGE",              Tag::BYTERANGE},
        {u"EXT-X-DISCONTINUITY",          Tag::DISCONTINUITY},
        {u"EXT-X-KEY",                    Tag::KEY},
        {u"EXT-X-MAP",                    Tag::MAP},
        {u"EXT-X-PROGRAM-DATE-TIME",      Tag::PROGRAM_DATE_TIME},
        {u"EXT-X-DATERANGE",              Tag::DATERANGE},
        {u"EXT-X-SKIP",                   Tag::SKIP},
        {u"EXT-X-PRELOAD-HINT",           Tag::PRELOAD_HINT},
        {u"EXT-X-RENDITION-REPORT",       Tag::RENDITION_REPORT},
        {u"EXT-X-GAP",                    Tag::GAP},
        {u"EXT-X-BITRATE",                Tag::BITRATE},
        {u"EXT-X-PART",                   Tag::PART},
        {u"EXT-X-TARGETDURATION",         Tag::TARGETDURATION},
        {u"EXT-X-MEDIA-SEQUENCE",         Tag::MEDIA_SEQUENCE},
        {u"EXT-X-DISCONTINUITY-SEQUENCE", Tag::DISCONTINUITY_SEQUENCE},
        {u"EXT-X-ENDLIST",                Tag::ENDLIST},
        {u"EXT-X-PLAYLIST-TYPE",          Tag::PLAYLIST_TYPE},
        {u"EXT-X-I-FRAMES-ONLY",          Tag::I_FRAMES_ONLY},
        {u"EXT-X-PART-INF",               Tag::PART_INF},
        {u"EXT-X-SERVER-CONTROL",         Tag::SERVER_CONTROL},
        {u"EXT-X-MEDIA",                  Tag::MEDIA},
        {u"EXT-X-STREAM-INF",             Tag::STREAM_INF},
        {u"EXT-X-I-FRAME-STREAM-INF",     Tag::I_FRAME_STREAM_INF},
        {u"EXT-X-SESSION-DATA",           Tag::SESSION_DATA},
        {u"EXT-X-SESSION-KEY",            Tag::SESSION_KEY},
        {u"EXT-X-CONTENT-STEERING",       Tag::CONTENT_STEERING},
        {u"EXT-X-INDEPENDENT-SEGMENTS",   Tag::INDEPENDENT_SEGMENTS},
        {u"EXT-X-START",                  Tag::START},
        {u"EXT-X-DEFINE",                 Tag::DEFINE},
    };
    return data;
}


//----------------------------------------------------------------------------
// Get the properties of a Tag.
//----------------------------------------------------------------------------

ts::hls::TagFlags ts::hls::TagProperties(Tag tag)
{
    static const std::map<Tag, TagFlags> properties {
        {Tag::EXTM3U,                 TagFlags::MASTER | TagFlags::MEDIA},
        {Tag::VERSION,                TagFlags::MASTER | TagFlags::MEDIA},
        {Tag::EXTINF,                 TagFlags::MEDIA},
        {Tag::BYTERANGE,              TagFlags::MEDIA},
        {Tag::DISCONTINUITY,          TagFlags::MEDIA},
        {Tag::KEY,                    TagFlags::MEDIA},
        {Tag::MAP,                    TagFlags::MEDIA},
        {Tag::PROGRAM_DATE_TIME,      TagFlags::MEDIA},
        {Tag::DATERANGE,              TagFlags::MEDIA},
        {Tag::SKIP,                   TagFlags::MEDIA},
        {Tag::PRELOAD_HINT,           TagFlags::MEDIA},
        {Tag::RENDITION_REPORT,       TagFlags::MEDIA},
        {Tag::GAP,                    TagFlags::MEDIA},
        {Tag::BITRATE,                TagFlags::MEDIA},
        {Tag::PART,                   TagFlags::MEDIA},
        {Tag::TARGETDURATION,         TagFlags::MEDIA},
        {Tag::MEDIA_SEQUENCE,         TagFlags::MEDIA},
        {Tag::DISCONTINUITY_SEQUENCE, TagFlags::MEDIA},
        {Tag::ENDLIST,                TagFlags::MEDIA},
        {Tag::PLAYLIST_TYPE,          TagFlags::MEDIA},
        {Tag::I_FRAMES_ONLY,          TagFlags::MEDIA},
        {Tag::PART_INF,               TagFlags::MEDIA},
        {Tag::SERVER_CONTROL,         TagFlags::MEDIA},
        {Tag::MEDIA,                  TagFlags::MASTER},
        {Tag::STREAM_INF,             TagFlags::MASTER},
        {Tag::I_FRAME_STREAM_INF,     TagFlags::MASTER},
        {Tag::SESSION_DATA,           TagFlags::MASTER},
        {Tag::CONTENT_STEERING,       TagFlags::MASTER},
        {Tag::SESSION_KEY,            TagFlags::MASTER},
        {Tag::INDEPENDENT_SEGMENTS,   TagFlags::MASTER | TagFlags::MEDIA},
        {Tag::START,                  TagFlags::MASTER | TagFlags::MEDIA},
        {Tag::DEFINE,                 TagFlags::MASTER | TagFlags::MEDIA},
    };

    auto it = properties.find(tag);
    return it == properties.end() ? TagFlags::NONE : it->second;
}
