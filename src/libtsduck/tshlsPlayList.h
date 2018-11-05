//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshls.h"
#include "tshlsMediaPlayList.h"
#include "tshlsMediaSegment.h"
#include "tsCerrReport.h"
#include "tsWebRequestArgs.h"

namespace ts {
    namespace hls {
        //!
        //! Playlist for HTTP Live Streaming (HLS).
        //! @ingroup hls
        //!
        class TSDUCKDLL PlayList
        {
        public:
            //!
            //! Constructor.
            //!
            PlayList();

            //!
            //! Clear the content of the playlist.
            //!
            void clear();

            //!
            //! Load the playlist from a URL.
            //! @param [in] url URL from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] args Web request arguments from command line.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadURL(const UString& url, bool strict = false, const WebRequestArgs args = WebRequestArgs(), PlayListType type = UNKNOWN_PLAYLIST, Report& report = CERR);

            //!
            //! Load the playlist from a text file.
            //! @param [in] filename File from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadFile(const UString& filename, bool strict = false, PlayListType type = UNKNOWN_PLAYLIST, Report& report = CERR);

            //!
            //! Load the playlist from its text content.
            //! @param [in] text Text of the playlist (multi-lines).
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadText(const UString& text, bool strict = false, PlayListType type = UNKNOWN_PLAYLIST, Report& report = CERR);

            //!
            //! Reload a media playlist with updated content.
            //! Master playlists or media playlists for which endList() is true are never reloaded.
            //! Live playlists (media playlists for which endList() is false) are reloaded from the same URL.
            //! New segments are added. If a segment hole is found, all previous content is replaced.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] args Web request arguments from command line.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool reload(bool strict = false, const WebRequestArgs args = WebRequestArgs(), Report& report = CERR);

            //!
            //! Check if the playlist has been successfully loaded.
            //! @return True if the playlist has been successfully loaded.
            //!
            bool isValid() const { return _valid; }

            //!
            //! Get the original URL.
            //! @return The original URL.
            //!
            UString url() const { return _url; }

            //!
            //! Build an URL for a media segment or sub playlist.
            //! @param [in] uri An URI, typically extracted from the playlist.
            //! @return The complete URL.
            //!
            UString buildURL(const UString& uri) const;

            //!
            //! Get the playlist type.
            //! @return The playlist type.
            //!
            PlayListType type() const { return _type; }

            //!
            //! Get the playlist version (EXT-X-VERSION).
            //! @return The playlist version.
            //!
            int version() const { return _version; }

            //!
            //! Get the segment target duration (informative, in media playlist).
            //! @return The segment target duration in seconds.
            //!
            Second targetDuration() const { return _targetDuration; }

            //!
            //! Get the sequence number of first segment (in media playlist).
            //! @return The sequence number of first segment.
            //!
            size_t mediaSequence() const { return _mediaSequence; }

            //!
            //! Get the end of list indicator (in media playlist).
            //! @return The end of list indicator. If true, there is no need to reload
            //! the playlist, there will not be any new segment.
            //!
            bool endList() const { return _endList; }

            //!
            //! Get the media playlist type ("EVENT" or "VOD", in media playlist).
            //! @return The media playlist type.
            //!
            UString playlistType() const { return _playlistType; }

            //!
            //! Get the number of media segments (in media playlist).
            //! @return The number of media segments.
            //!
            size_t segmentCount() const { return _segments.size(); }

            //!
            //! Get the number of media playlists (in master playlist).
            //! @return The number of media playlists.
            //!
            size_t playListCount() const { return _playlists.size(); }

            //!
            //! Get a constant reference to a media segment (in media playlist).
            //! @param [in] index Index of the segment, from 0 to segmentCount().
            //! @return A constant reference to the media segment at @a index.
            //!
            const MediaSegment& segment(size_t index) const;

            //!
            //! Remove the first media segment (in media playlist).
            //! @param [out] seg Characteristics of the first segment.
            //! @return True of a segment was successfully removed, false otherwise.
            //!
            bool popFirstSegment(MediaSegment& seg);

            //!
            //! Get a constant reference to a media playlist description (in master playlist).
            //! @param [in] index Index of the playlist, from 0 to playListCount().
            //! @return A constant reference to the media playlist description at @a index.
            //!
            const MediaPlayList& playList(size_t index) const;

            //!
            //! Select the first media playlist with specific constraints.
            //! @param [in] minBitrate Minimum bitrate. Zero means no minimum.
            //! @param [in] maxBitrate Maximum bitrate. Zero means no maximum.
            //! @param [in] minWidth Minimum width. Zero means no minimum.
            //! @param [in] maxWidth Maximum width. Zero means no maximum.
            //! @param [in] minHeight Minimum height. Zero means no minimum.
            //! @param [in] maxHeight Maximum height. Zero means no maximum.
            //! @return Index of the selected media play list or NPOS if there is none.
            //!
            size_t selectPlayList(BitRate minBitrate,
                                  BitRate maxBitrate,
                                  size_t  minWidth,
                                  size_t  maxWidth,
                                  size_t  minHeight,
                                  size_t  maxHeight) const;

            //!
            //! Select the media playlist with the lowest bitrate.
            //! @return Index of the selected media play list or NPOS if there is none.
            //!
            size_t selectPlayListLowestBitRate() const;

            //!
            //! Select the media playlist with the highest bitrate.
            //! @return Index of the selected media play list or NPOS if there is none.
            //!
            size_t selectPlayListHighestBitRate() const;

            //!
            //! Select the media playlist with the lowest resolution.
            //! @return Index of the selected media play list or NPOS if there is none.
            //!
            size_t selectPlayListLowestResolution() const;

            //!
            //! Select the media playlist with the highest resolution.
            //! @return Index of the selected media play list or NPOS if there is none.
            //!
            size_t selectPlayListHighestResolution() const;

        private:
            // We need to access lists of media, with index access and fast insert at beginning and end.
            typedef std::deque<MediaSegment> MediaSegmentQueue;
            typedef std::deque<MediaPlayList> MediaPlayListQueue;

            bool               _valid;           // Content loaded and valid.
            int                _version;         // Playlist format version.
            PlayListType       _type;            // Playlist type.
            UString            _url;             // Original URL (or file name).
            UString            _urlBase;         // Base URL (to resolve relative URI's).
            bool               _isURL;           // The base is an URL, not a directory name.
            Second             _targetDuration;  // Segment target duration (media playlist).
            size_t             _mediaSequence;   // Sequence number of first segment (media playlist).
            bool               _endList;         // End of list indicator (media playlist).
            UString            _playlistType;    // Media playlist type ("EVENT" or "VOD", media playlist).
            MediaSegmentQueue  _segments;        // List of media segments (media playlist).
            MediaPlayListQueue _playlists;       // List of media playlists (master playlist).

            // Empty data to return.
            static const MediaSegment emptySegment;
            static const MediaPlayList emptyPlayList;

            // Load from the text content.
            bool parse(const UString& text, bool strict, Report& report);
            bool parse(const UStringList& lines, bool strict, Report& report);

            // Check if the line contains a valid tag or URI.
            bool getTag(const UString& line, Tag& tag, UString& params, bool strict, Report& report);
            bool isURI(const UString& line, bool strict, Report& report);

            // Set the playlist type, return true on success, false on error.
            bool setType(PlayListType type, Report& report);
        };
    }
}
