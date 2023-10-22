//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Global declarations for HTTP Live Streaming (HLS) classes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Namespace for HTTP Live Streaming (HLS) classes.
    //!
    namespace hls {
        //!
        //! Types of .M3U8 playlist.
        //!
        //! - Master playlist. It contains references to media playlists (typically same content with various bitrates).
        //!   Also called "multivariant playlist" in recent versions of the HLS standard.
        //! - All other types of playlists are media playlists, they contain references to media segments.
        //!   - VoD playlist. A static media playlist for a fully recorded content. The list of media segments cannot change.
        //!   - Event playlist. A growing media playlist for a running event. It is possible to move backward in the event,
        //!     up to the beginning. New media segments can be added at the end of the playlist. No segment can be removed.
        //!   - Live playlist. A sliding media playlist for a live channel, without backward browsing. The initial segments
        //!     are regularly removed. New segments are regularly added at the end of the list.
        //!
        enum class PlayListType {
            UNKNOWN,  //!< Type is unknown.
            MASTER,   //!< Master playlist, contains references to media playlists.
            VOD,      //!< VoD media playlist, reference media segments, static, cannot change.
            EVENT,    //!< Event media playlist, reference media segments, can grow.
            LIVE,     //!< Live media playlist, reference media segments, sliding window.
        };

        //!
        //! Tags to be used in the .M3U8 playlists.
        //! @ingroup hls
        //! @see RFC 8216, chapter 4.
        //! @see draft-pantos-hls-rfc8216bis-10
        //!
        enum class Tag {
            //
            // 4.4.1 Basic Tags
            //
            EXTM3U,                  //!< \#EXTM3U - first line, all playlists.
            VERSION,                 //!< \#EXT-X-VERSION:n - global, version number.
            //
            // 4.4.4 Media Segment Tags, apply to one or more media segments, media playlists only.
            //
            EXTINF,                  //!< \#EXTINF:duration,[title] - next media segment only, required.
            BYTERANGE,               //!< \#EXT-X-BYTERANGE:n[\@o] - next media segment only.
            DISCONTINUITY,           //!< \#EXT-X-DISCONTINUITY - next media segment only.
            KEY,                     //!< \#EXT-X-KEY:attribute-list - all media segments until next KEY.
            MAP,                     //!< \#EXT-X-MAP:attribute-list - all media segments until next MAP.
            PROGRAM_DATE_TIME,       //!< \#EXT-X-PROGRAM-DATE-TIME:date-time-msec - next media segment only.
            GAP,                     //!< \#EXT-X-GAP
            BITRATE,                 //!< \#EXT-X-BITRATE:rate
            PART,                    //!< \#EXT-X-PART:attribute-list
            //
            // 4.4.5 Media Metadata Tags, apply to one or more media segments, media playlists only.
            //
            DATERANGE,               //!< \#EXT-X-DATERANGE:attribute-list
            SKIP,                    //!< \#EXT-X-SKIP:attribute-list
            PRELOAD_HINT,            //!< \#EXT-X-PRELOAD-HINT:attribute-list
            RENDITION_REPORT,        //!< \#EXT-X-RENDITION-REPORT:attribute-list
            //
            // 4.4.3 Media Playlist Tags, global parameters of a Media Playlist.
            //
            TARGETDURATION,          //!< \#EXT-X-TARGETDURATION:s
            MEDIA_SEQUENCE,          //!< \#EXT-X-MEDIA-SEQUENCE:number
            DISCONTINUITY_SEQUENCE,  //!< \#EXT-X-DISCONTINUITY-SEQUENCE:number
            ENDLIST,                 //!< \#EXT-X-ENDLIST
            PLAYLIST_TYPE,           //!< \#EXT-X-PLAYLIST-TYPE:type (EVENT or VOD).
            I_FRAMES_ONLY,           //!< \#EXT-X-I-FRAMES-ONLY
            PART_INF,                //!< \#EXT-X-PART-INF
            SERVER_CONTROL,          //!< \#EXT-X-SERVER-CONTROL
            //
            // 4.4.6 Master / Multivariant Playlist Tags
            //
            MEDIA,                   //!< \#EXT-X-MEDIA:attribute-list
            STREAM_INF,              //!< \#EXT-X-STREAM-INF:attribute-list - immediately followed by an URI line.
            I_FRAME_STREAM_INF,      //!< \#EXT-X-I-FRAME-STREAM-INF:attribute-list - global to playlist.
            SESSION_DATA,            //!< \#EXT-X-SESSION-DATA:attribute-list
            SESSION_KEY,             //!< \#EXT-X-SESSION-KEY:attribute-list
            CONTENT_STEERING,        //!< \#EXT-X-CONTENT-STEERING:attribute-list
            //
            // 4.4.2 Media or Master Playlist Tags
            //
            INDEPENDENT_SEGMENTS,    //!< \#EXT-X-INDEPENDENT-SEGMENTS
            START,                   //!< \#EXT-X-START:attribute-list
            DEFINE,                  //!< \#EXT-X-DEFINE:attribute-list
        };

        //!
        //! Properties of playlist tags.
        //! Can be used as bitmask.
        //!
        enum class TagFlags {
            NONE   = 0x0000,   //! Tag is not allowed anywhere.
            MASTER = 0x0001,   //!< The tag is allowed in master playlists.
            MEDIA  = 0x0002,   //!< The tag is allowed in media playlists.
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
        TSDUCKDLL TagFlags TagProperties(Tag tag);
    }
}
TS_ENABLE_BITMASK_OPERATORS(ts::hls::TagFlags);
