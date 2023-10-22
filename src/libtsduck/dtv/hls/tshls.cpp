//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshls.h"

const ts::Enumeration ts::hls::TagNames({
    {u"EXTM3U",                       ts::hls::Tag::EXTM3U},
    {u"EXT-X-VERSION",                ts::hls::Tag::VERSION},
    {u"EXTINF",                       ts::hls::Tag::EXTINF},
    {u"EXT-X-BYTERANGE",              ts::hls::Tag::BYTERANGE},
    {u"EXT-X-DISCONTINUITY",          ts::hls::Tag::DISCONTINUITY},
    {u"EXT-X-KEY",                    ts::hls::Tag::KEY},
    {u"EXT-X-MAP",                    ts::hls::Tag::MAP},
    {u"EXT-X-PROGRAM-DATE-TIME",      ts::hls::Tag::PROGRAM_DATE_TIME},
    {u"EXT-X-DATERANGE",              ts::hls::Tag::DATERANGE},
    {u"EXT-X-SKIP",                   ts::hls::Tag::SKIP},
    {u"EXT-X-PRELOAD-HINT",           ts::hls::Tag::PRELOAD_HINT},
    {u"EXT-X-RENDITION-REPORT",       ts::hls::Tag::RENDITION_REPORT},
    {u"EXT-X-GAP",                    ts::hls::Tag::GAP},
    {u"EXT-X-BITRATE",                ts::hls::Tag::BITRATE},
    {u"EXT-X-PART",                   ts::hls::Tag::PART},
    {u"EXT-X-TARGETDURATION",         ts::hls::Tag::TARGETDURATION},
    {u"EXT-X-MEDIA-SEQUENCE",         ts::hls::Tag::MEDIA_SEQUENCE},
    {u"EXT-X-DISCONTINUITY-SEQUENCE", ts::hls::Tag::DISCONTINUITY_SEQUENCE},
    {u"EXT-X-ENDLIST",                ts::hls::Tag::ENDLIST},
    {u"EXT-X-PLAYLIST-TYPE",          ts::hls::Tag::PLAYLIST_TYPE},
    {u"EXT-X-I-FRAMES-ONLY",          ts::hls::Tag::I_FRAMES_ONLY},
    {u"EXT-X-PART-INF",               ts::hls::Tag::PART_INF},
    {u"EXT-X-SERVER-CONTROL",         ts::hls::Tag::SERVER_CONTROL},
    {u"EXT-X-MEDIA",                  ts::hls::Tag::MEDIA},
    {u"EXT-X-STREAM-INF",             ts::hls::Tag::STREAM_INF},
    {u"EXT-X-I-FRAME-STREAM-INF",     ts::hls::Tag::I_FRAME_STREAM_INF},
    {u"EXT-X-SESSION-DATA",           ts::hls::Tag::SESSION_DATA},
    {u"EXT-X-SESSION-KEY",            ts::hls::Tag::SESSION_KEY},
    {u"EXT-X-CONTENT-STEERING",       ts::hls::Tag::CONTENT_STEERING},
    {u"EXT-X-INDEPENDENT-SEGMENTS",   ts::hls::Tag::INDEPENDENT_SEGMENTS},
    {u"EXT-X-START",                  ts::hls::Tag::START},
    {u"EXT-X-DEFINE",                 ts::hls::Tag::DEFINE},
});


//----------------------------------------------------------------------------
// Get the properties of a Tag.
//----------------------------------------------------------------------------

namespace {
    const std::map<ts::hls::Tag,ts::hls::TagFlags> TagPropertyMap({
        {ts::hls::Tag::EXTM3U,                 ts::hls::TagFlags::MASTER | ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::VERSION,                ts::hls::TagFlags::MASTER | ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::EXTINF,                 ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::BYTERANGE,              ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::DISCONTINUITY,          ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::KEY,                    ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::MAP,                    ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::PROGRAM_DATE_TIME,      ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::DATERANGE,              ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::SKIP,                   ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::PRELOAD_HINT,           ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::RENDITION_REPORT,       ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::GAP,                    ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::BITRATE,                ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::PART,                   ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::TARGETDURATION,         ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::MEDIA_SEQUENCE,         ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::DISCONTINUITY_SEQUENCE, ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::ENDLIST,                ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::PLAYLIST_TYPE,          ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::I_FRAMES_ONLY,          ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::PART_INF,               ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::SERVER_CONTROL,         ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::MEDIA,                  ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::STREAM_INF,             ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::I_FRAME_STREAM_INF,     ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::SESSION_DATA,           ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::CONTENT_STEERING,       ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::SESSION_KEY,            ts::hls::TagFlags::MASTER},
        {ts::hls::Tag::INDEPENDENT_SEGMENTS,   ts::hls::TagFlags::MASTER | ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::START,                  ts::hls::TagFlags::MASTER | ts::hls::TagFlags::MEDIA},
        {ts::hls::Tag::DEFINE,                 ts::hls::TagFlags::MASTER | ts::hls::TagFlags::MEDIA},
    });
}

ts::hls::TagFlags ts::hls::TagProperties(Tag tag)
{
    auto it = TagPropertyMap.find(tag);
    return it == TagPropertyMap.end() ? TagFlags::NONE : it->second;
}
