//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of an alternative rendition playlist inside an HLS master playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshlsMediaElement.h"

namespace ts {
    namespace hls {
        //!
        //! Description of an alternative rendition media playlist inside an HLS master playlist.
        //! Alternative rendition media playlists are introduced by the tag @c \#EXT-X-MEDIA while
        //! regular media playlists (class MediaPlayList) are introduced by the tag @c \#EXT-X-STREAM-INF.
        //! @ingroup hls
        //!
        class TSDUCKDLL AltPlayList: public MediaElement
        {
        public:
            //!
            //! Constructor.
            //!
            AltPlayList() = default;

            // Implementation of StringifyInterface
            virtual UString toString() const override;

            // Public fields.
            bool    isDefault = false;     //!< The client should play this Rendition in the absence of information from the user indicating a different choice.
            bool    autoselect = false;    //!< The client may choose to play this Rendition in the absence of explicit user preference.
            bool    forced = false;        //!< The Rendition contains content that is considered essential to play.
            UString name {};               //!< Human-readable description of the Rendition. Required.
            UString type {};               //!< Playlist type, required, one of "AUDIO", "VIDEO", "SUBTITLES", "CLOSED-CAPTIONS".
            UString groupId {};            //!< Group to which the Rendition belongs. Required.
            UString stableRenditionId {};  //!< Stable identifier for the URI within the Multivariant Playlist. Optional.
            UString language {};           //!< Primary language used in the Rendition. Optional.
            UString assocLanguage {};      //!< Associated language. Optional.
            UString inStreamId {};         //!< Rendition within the segments in the Media Playlist.
            UString characteristics {};    //!< Media Characteristic Tags (MCTs) separated by comma (,) characters.
            UString channels {};           //!< Ordered, slash-separated ("/") list of channel parameters.
        };
    }
}
