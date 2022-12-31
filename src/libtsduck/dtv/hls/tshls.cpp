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

#include "tshls.h"

const ts::Enumeration ts::hls::TagNames({
    {u"EXTM3U",                       ts::hls::EXTM3U},
    {u"EXT-X-VERSION",                ts::hls::VERSION},
    {u"EXTINF",                       ts::hls::EXTINF},
    {u"EXT-X-BYTERANGE",              ts::hls::BYTERANGE},
    {u"EXT-X-DISCONTINUITY",          ts::hls::DISCONTINUITY},
    {u"EXT-X-KEY",                    ts::hls::KEY},
    {u"EXT-X-MAP",                    ts::hls::MAP},
    {u"EXT-X-PROGRAM-DATE-TIME",      ts::hls::PROGRAM_DATE_TIME},
    {u"EXT-X-DATERANGE",              ts::hls::DATERANGE},
    {u"EXT-X-SKIP",                   ts::hls::SKIP},
    {u"EXT-X-PRELOAD-HINT",           ts::hls::PRELOAD_HINT},
    {u"EXT-X-RENDITION-REPORT",       ts::hls::RENDITION_REPORT},
    {u"EXT-X-GAP",                    ts::hls::GAP},
    {u"EXT-X-BITRATE",                ts::hls::BITRATE},
    {u"EXT-X-PART",                   ts::hls::PART},
    {u"EXT-X-TARGETDURATION",         ts::hls::TARGETDURATION},
    {u"EXT-X-MEDIA-SEQUENCE",         ts::hls::MEDIA_SEQUENCE},
    {u"EXT-X-DISCONTINUITY-SEQUENCE", ts::hls::DISCONTINUITY_SEQUENCE},
    {u"EXT-X-ENDLIST",                ts::hls::ENDLIST},
    {u"EXT-X-PLAYLIST-TYPE",          ts::hls::PLAYLIST_TYPE},
    {u"EXT-X-I-FRAMES-ONLY",          ts::hls::I_FRAMES_ONLY},
    {u"EXT-X-PART-INF",               ts::hls::PART_INF},
    {u"EXT-X-SERVER-CONTROL",         ts::hls::SERVER_CONTROL},
    {u"EXT-X-MEDIA",                  ts::hls::MEDIA},
    {u"EXT-X-STREAM-INF",             ts::hls::STREAM_INF},
    {u"EXT-X-I-FRAME-STREAM-INF",     ts::hls::I_FRAME_STREAM_INF},
    {u"EXT-X-SESSION-DATA",           ts::hls::SESSION_DATA},
    {u"EXT-X-SESSION-KEY",            ts::hls::SESSION_KEY},
    {u"EXT-X-CONTENT-STEERING",       ts::hls::CONTENT_STEERING},
    {u"EXT-X-INDEPENDENT-SEGMENTS",   ts::hls::INDEPENDENT_SEGMENTS},
    {u"EXT-X-START",                  ts::hls::START},
    {u"EXT-X-DEFINE",                 ts::hls::DEFINE},
});


//----------------------------------------------------------------------------
// Get the properties of a Tag.
//----------------------------------------------------------------------------

namespace {
    const std::map<ts::hls::Tag,int> TagPropertyMap({
        {ts::hls::EXTM3U,                 ts::hls::TAG_MASTER | ts::hls::TAG_MEDIA},
        {ts::hls::VERSION,                ts::hls::TAG_MASTER | ts::hls::TAG_MEDIA},
        {ts::hls::EXTINF,                 ts::hls::TAG_MEDIA},
        {ts::hls::BYTERANGE,              ts::hls::TAG_MEDIA},
        {ts::hls::DISCONTINUITY,          ts::hls::TAG_MEDIA},
        {ts::hls::KEY,                    ts::hls::TAG_MEDIA},
        {ts::hls::MAP,                    ts::hls::TAG_MEDIA},
        {ts::hls::PROGRAM_DATE_TIME,      ts::hls::TAG_MEDIA},
        {ts::hls::DATERANGE,              ts::hls::TAG_MEDIA},
        {ts::hls::SKIP,                   ts::hls::TAG_MEDIA},
        {ts::hls::PRELOAD_HINT,           ts::hls::TAG_MEDIA},
        {ts::hls::RENDITION_REPORT,       ts::hls::TAG_MEDIA},
        {ts::hls::GAP,                    ts::hls::TAG_MEDIA},
        {ts::hls::BITRATE,                ts::hls::TAG_MEDIA},
        {ts::hls::PART,                   ts::hls::TAG_MEDIA},
        {ts::hls::TARGETDURATION,         ts::hls::TAG_MEDIA},
        {ts::hls::MEDIA_SEQUENCE,         ts::hls::TAG_MEDIA},
        {ts::hls::DISCONTINUITY_SEQUENCE, ts::hls::TAG_MEDIA},
        {ts::hls::ENDLIST,                ts::hls::TAG_MEDIA},
        {ts::hls::PLAYLIST_TYPE,          ts::hls::TAG_MEDIA},
        {ts::hls::I_FRAMES_ONLY,          ts::hls::TAG_MEDIA},
        {ts::hls::PART_INF,               ts::hls::TAG_MEDIA},
        {ts::hls::SERVER_CONTROL,         ts::hls::TAG_MEDIA},
        {ts::hls::MEDIA,                  ts::hls::TAG_MASTER},
        {ts::hls::STREAM_INF,             ts::hls::TAG_MASTER},
        {ts::hls::I_FRAME_STREAM_INF,     ts::hls::TAG_MASTER},
        {ts::hls::SESSION_DATA,           ts::hls::TAG_MASTER},
        {ts::hls::CONTENT_STEERING,       ts::hls::TAG_MASTER},
        {ts::hls::SESSION_KEY,            ts::hls::TAG_MASTER},
        {ts::hls::INDEPENDENT_SEGMENTS,   ts::hls::TAG_MASTER | ts::hls::TAG_MEDIA},
        {ts::hls::START,                  ts::hls::TAG_MASTER | ts::hls::TAG_MEDIA},
        {ts::hls::DEFINE,                 ts::hls::TAG_MASTER | ts::hls::TAG_MEDIA},
    });
}

int ts::hls::TagProperties(Tag tag)
{
    auto it = TagPropertyMap.find(tag);
    return it == TagPropertyMap.end() ? 0 : it->second;
}
