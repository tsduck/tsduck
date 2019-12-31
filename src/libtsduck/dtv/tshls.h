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
//!
//!  @file
//!  Global declarations for HTTP Live Streaming (HLS) classes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Namespace for HTTP Live Streaming (HLS) classes.
    //!
    namespace hls {
        //!
        //! Types of .M3U8 playlist.
        //!
        enum PlayListType {
            UNKNOWN_PLAYLIST,  //!< Type is unknown.
            MASTER_PLAYLIST,   //!< Master playlist, contains references to media playlists.
            MEDIA_PLAYLIST,    //!< Media playlist, contains references to media segments.
        };

        //!
        //! Tags to be used in the .M3U8 playlists.
        //! @ingroup hls
        //! @see RFC 8216, chapter 4.
        //! @see draft-pantos-hls-rfc8216bis-03
        //!
        enum Tag {
            //
            // 4.3.1 Basic Tags
            //
            EXTM3U,                  //!< \#EXTM3U - first line, all playlists.
            VERSION,                 //!< \#EXT-X-VERSION:n - global, version number.
            //
            // 4.3.2 Media Segment Tags, apply to one or more media segments, media playlists only.
            //
            EXTINF,                  //!< \#EXTINF:duration,[title] - next media segment only, required.
            BYTERANGE,               //!< \#EXT-X-BYTERANGE:n[\@o] - next media segment only.
            DISCONTINUITY,           //!< \#EXT-X-DISCONTINUITY - next media segment only.
            KEY,                     //!< \#EXT-X-KEY:attribute-list - all media segments until next KEY.
            MAP,                     //!< \#EXT-X-MAP:attribute-list - all media segments until next MAP.
            PROGRAM_DATE_TIME,       //!< \#EXT-X-PROGRAM-DATE-TIME:date-time-msec - next media segment only.
            DATERANGE,               //!< \#EXT-X-DATERANGE:attribute-list
            GAP,                     //!< \#EXT-X-GAP
            BITRATE,                 //!< \#EXT-X-BITRATE:rate
            //
            // 4.3.3 Media Playlist Tags, global parameters of a Media Playlist.
            //
            TARGETDURATION,          //!< \#EXT-X-TARGETDURATION:s
            MEDIA_SEQUENCE,          //!< \#EXT-X-MEDIA-SEQUENCE:number
            DISCONTINUITY_SEQUENCE,  //!< \#EXT-X-DISCONTINUITY-SEQUENCE:number
            ENDLIST,                 //!< \#EXT-X-ENDLIST
            PLAYLIST_TYPE,           //!< \#EXT-X-PLAYLIST-TYPE:type (EVENT or VOD).
            I_FRAMES_ONLY,           //!< \#EXT-X-I-FRAMES-ONLY
            //
            // 4.3.4 Master Playlist Tags
            //
            MEDIA,                   //!< \#EXT-X-MEDIA:attribute-list
            STREAM_INF,              //!< \#EXT-X-STREAM-INF:attribute-list - immediately followed by an URI line.
            I_FRAME_STREAM_INF,      //!< \#EXT-X-I-FRAME-STREAM-INF:attribute-list - global to playlist.
            SESSION_DATA,            //!< \#EXT-X-SESSION-DATA:attribute-list
            SESSION_KEY,             //!< \#EXT-X-SESSION-KEY:attribute-list
            //
            // 4.3.5 Media or Master Playlist Tags
            //
            INDEPENDENT_SEGMENTS,    //!< \#EXT-X-INDEPENDENT-SEGMENTS
            START,                   //!< \#EXT-X-START:attribute-list
            DEFINE,                  //!< \#EXT-X-DEFINE:attribute-list
        };

        //!
        //! Properties of playlist tags.
        //! Can be used as bitmask.
        //!
        enum TagFlags {
            TAG_MASTER = 0x0001,     //!< The tag is allowed in master playlists.
            TAG_MEDIA  = 0x0002,     //!< The tag is allowed in media playlists.
        };

        //!
        //! Enumeration description of ts::hls::Tag.
        //! The names are the actual tag names from a .M3U8 playlist file.
        //!
        TSDUCKDLL extern const Enumeration TagNames;

        //!
        //! Get the properties of a Tag.
        //! @param [in] tag The tag to get the properties of.
        //! @return A bitmask of TagFlags.
        //!
        TSDUCKDLL int TagProperties(Tag tag);
    }
}
