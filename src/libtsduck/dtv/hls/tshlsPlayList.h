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
//!
//!  @file
//!  HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshls.h"
#include "tshlsMediaPlayList.h"
#include "tshlsAltPlayList.h"
#include "tshlsMediaSegment.h"
#include "tsURL.h"
#include "tsTime.h"
#include "tsCerrReport.h"
#include "tsWebRequestArgs.h"
#include "tsStringifyInterface.h"

namespace ts {
    namespace hls {
        //!
        //! Playlist for HTTP Live Streaming (HLS).
        //! @ingroup hls
        //!
        class TSDUCKDLL PlayList: public StringifyInterface
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
            //! Reset the content of a playlist.
            //! Should be used before rebuilding a new playlist.
            //! @param [in] type Playlist type.
            //! @param [in] filename File path where the playlist will be saved.
            //! This required to build relative paths for sub-playlists or media segments.
            //! @param [in] version Playlist format version. The default is 3, the minimum
            //! level which is required for playlists which are supported here.
            //!
            void reset(PlayListType type, const UString& filename, int version = 3);

            //!
            //! Load the playlist from a URL.
            //! @param [in] url URL from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] args Web request arguments from command line.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadURL(const UString& url, bool strict = false, const WebRequestArgs args = WebRequestArgs(), PlayListType type = PlayListType::UNKNOWN, Report& report = CERR);

            //!
            //! Load the playlist from a URL.
            //! @param [in] url URL from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] args Web request arguments from command line.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadURL(const URL& url, bool strict = false, const WebRequestArgs args = WebRequestArgs(), PlayListType type = PlayListType::UNKNOWN, Report& report = CERR);

            //!
            //! Load the playlist from a text file.
            //! @param [in] filename File from which to load the playlist.
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadFile(const UString& filename, bool strict = false, PlayListType type = PlayListType::UNKNOWN, Report& report = CERR);

            //!
            //! Load the playlist from its text content.
            //! @param [in] text Text of the playlist (multi-lines).
            //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
            //! @param [in] type Expected playlist type, unknown by default.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool loadText(const UString& text, bool strict = false, PlayListType type = PlayListType::UNKNOWN, Report& report = CERR);

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
            //! Set a directory name where all loaded files or URL are automatically saved.
            //! @param [in] dir A directory name.
            //!
            void setAutoSaveDirectory(const UString dir) { _autoSaveDir = dir; }

            //!
            //! Check if the playlist has been successfully loaded.
            //! @return True if the playlist has been successfully loaded.
            //!
            bool isValid() const { return _valid; }

            //!
            //! Save the playlist to a text file.
            //! @param [in] filename File where to save the playlist. By default, use the same file from loadFile() or reset().
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool saveFile(const UString& filename = UString(), Report& report = CERR) const;

            //!
            //! Build the text content of the playlist.
            //! @param [in,out] report Where to report errors.
            //! @return The text content on success, an empty string on error.
            //!
            UString textContent(Report& report = CERR) const;

            //!
            //! Get the orginal loaded text content of the playlist.
            //! This can be different from the current content of the playlist
            //! if the object has been modified.
            //! @return A constant reference to the original loaded text lines.
            //!
            const UStringList& originalLoadedContent() const { return _loadedContent; }

            //!
            //! Get the original URL.
            //! @return The original URL.
            //!
            UString url() const { return _original; }

            //!
            //! Build an URL for a media segment or sub-playlist.
            //! @param [out] media A media element with all fields filled in.
            //! @param [in] uri An URI, typically extracted from the playlist.
            //!
            void buildURL(MediaElement& media, const UString& uri) const;

            //!
            //! Get the playlist type.
            //! @return The playlist type.
            //!
            PlayListType type() const { return _type; }

            //!
            //! Set the playlist type.
            //! @param [in] type The playlist type.
            //! @param [in,out] report Where to report errors.
            //! @param [in] forced When true, unconditionally set the playlist type.
            //! When false (the default), check that the playlist type does not change in an inconsistent way.
            //! Allowed changes are UNKNOWN to anything and LIVE to VOD or EVENT. The latter case is a playlist
            //! which is known to be a media playlist but for which no EXT-X-PLAYLIST-TYPE tag was found so far.
            //! @return True on success, false on error.
            //!
            bool setType(PlayListType type, Report& report = CERR, bool forced = false);

            //!
            //! Set the playlist type as media playlist.
            //! If the type is already known and already a media playlist, do nothing.
            //! If the type is unknown, set it as LIVE which is a media playlist type without EXT-X-PLAYLIST-TYPE
            //! tag and which can be later turned into a VOD or EVENT playlist.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool setTypeMedia(Report& report = CERR);

            //!
            //! Add a custom tag in the playlist.
            //! @param [in] tag A custom tag line to add in the playlist. If @a does not start
            //! with a @c '#', it will be automatically added.
            //!
            void addCustomTag(const UString& tag) { _extraTags.push_back(tag); }

            //!
            //! Clear all application-specific custom tags.
            //! @see addCustomTag()
            //!
            void clearCustomTags() { _extraTags.clear(); }

            //!
            //! Check if the playlist can be updated (and must be reloaded later).
            //! @return True if the playlist can be updated (and must be reloaded later).
            //!
            bool isUpdatable() const { return (_type == PlayListType::EVENT || _type == PlayListType::LIVE) && !_endList; }

            //!
            //! Check if the playlist is a media playlist (contains references to media segments).
            //! @return True if the playlist is a media playlist.
            //!
            bool isMedia() const { return _type == PlayListType::EVENT || _type == PlayListType::LIVE || _type == PlayListType::VOD; }

            //!
            //! Check if the playlist is a master playlist (contains references to media playlists).
            //! @return True if the playlist is a master playlist.
            //!
            bool isMaster() const { return _type == PlayListType::MASTER; }

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
            //! Set the segment target duration in a media playlist.
            //! @param [in] duration The segment target duration in seconds.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool setTargetDuration(Second duration, Report& report = CERR);

            //!
            //! Get the sequence number of first segment (in media playlist).
            //! @return The sequence number of first segment.
            //!
            size_t mediaSequence() const { return _mediaSequence; }

            //!
            //! Set the sequence number of first segment in a media playlist.
            //! @param [in] seq The sequence number of first segment.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool setMediaSequence(size_t seq, Report& report = CERR);

            //!
            //! Get the end of list indicator (in media playlist).
            //! @return The end of list indicator. If true, there is no need to reload
            //! the playlist, there will not be any new segment.
            //!
            bool endList() const { return _endList; }

            //!
            //! Set the end of list indicator in a media playlist.
            //! @param [in] end The end of list indicator.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool setEndList(bool end, Report& report = CERR);

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
            //! Get the number of altenative rendition playlists (in master playlist).
            //! @return The number of altenative rendition playlists.
            //!
            size_t altPlayListCount() const { return _altPlaylists.size(); }

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
            //! Remove the first media segment and do not even return it (in media playlist).
            //! @return True of a segment was successfully removed, false otherwise.
            //!
            bool popFirstSegment();

            //!
            //! Add a segment in a media playlist.
            //! @param [in] seg The new media segment to append. If the playlist's URI is a file
            //! name, the URI of the segment is transformed into a relative URI from the playlist's path.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool addSegment(const MediaSegment& seg, Report& report = CERR);

            //!
            //! Get the download UTC time of the playlist.
            //! @return The download UTC time of the playlist.
            //!
            Time downloadUTC() const { return _utcDownload; }

            //!
            //! Get the playout estimated termination UTC time of the playlist (in media playlist).
            //! @return The estimated playout UTC time of the playlist. This is the download time
            //! of the playlist plus the sum of all segment durations.
            //!
            Time terminationUTC() const { return _utcTermination; }

            //!
            //! Get a constant reference to a media playlist description (in master playlist).
            //! @param [in] index Index of the playlist, from 0 to playListCount()-1.
            //! @return A constant reference to the media playlist description at @a index.
            //!
            const MediaPlayList& playList(size_t index) const;

            //!
            //! Delete a media playlist description from a master playlist.
            //! @param [in] index Index of the media playlist to delete, from 0 to playListCount()-1.
            //!
            void deletePlayList(size_t index);

            //!
            //! Add a media playlist in a master playlist.
            //! @param [in] pl The new media playlist to append. If the master playlist's URI is a file
            //! name, the URI of the media playlist is transformed into a relative URI from the master
            //! playlist's path.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool addPlayList(const MediaPlayList& pl, Report& report = CERR);

            //!
            //! Select the first media playlist with specific constraints.
            //! @param [in] minBitrate Minimum bitrate. Zero means no minimum.
            //! @param [in] maxBitrate Maximum bitrate. Zero means no maximum.
            //! @param [in] minWidth Minimum width. Zero means no minimum.
            //! @param [in] maxWidth Maximum width. Zero means no maximum.
            //! @param [in] minHeight Minimum height. Zero means no minimum.
            //! @param [in] maxHeight Maximum height. Zero means no maximum.
            //! @return Index of the selected media play list or NPOS if there is none.
            //! If all criteria are zero, select the first playlist.
            //!
            size_t selectPlayList(const BitRate& minBitrate,
                                  const BitRate& maxBitrate,
                                  size_t minWidth,
                                  size_t maxWidth,
                                  size_t minHeight,
                                  size_t maxHeight) const;

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

            //!
            //! Get a constant reference to an alternative rendition playlist description (in master playlist).
            //! @param [in] index Index of the playlist, from 0 to altPlayListCount()-1.
            //! @return A constant reference to the alternative rendition playlist description at @a index.
            //!
            const AltPlayList& altPlayList(size_t index) const;

            //!
            //! Delete an alternative rendition playlist description from a master playlist.
            //! @param [in] index Index of the alternative rendition playlist to delete, from 0 to altPlayListCount()-1.
            //!
            void deleteAltPlayList(size_t index);

            //!
            //! Add an alternative rendition media playlist in a master playlist.
            //! @param [in] pl The new alternative rendition playlist to append. If the master playlist's URI is a file
            //! name, the URI of the media playlist is transformed into a relative URI from the master
            //! playlist's path.
            //! @param [in,out] report Where to report errors.
            //! @return True on success, false on error.
            //!
            bool addAltPlayList(const AltPlayList& pl, Report& report = CERR);

            //!
            //! Select the first alternative rendition playlist with specific criteria.
            //! @param [in] type The type to match. Ignored if empty.
            //! @param [in] name The name to match. Ignored if empty.
            //! @param [in] groupId The group-id to match. Ignored if empty.
            //! @param [in] language The language to match. Ignored if empty.
            //! @return Index of the selected alternative rendition playlist which matches all non-empty criteria
            //! or NPOS if there is none. If all criteria are empty, select the first playlist.
            //!
            size_t selectAltPlayList(const UString& type = UString(),
                                     const UString& name = UString(),
                                     const UString& groupId = UString(),
                                     const UString& language = UString()) const;

            // Implementation of StringifyInterface
            virtual UString toString() const override;

        private:
            // We need to access lists of media, with index access and fast insert at beginning and end.
            typedef std::deque<MediaSegment> MediaSegmentQueue;
            typedef std::deque<MediaPlayList> MediaPlayListQueue;
            typedef std::deque<AltPlayList> AltPlayListQueue;

            bool               _valid;           // Content loaded and valid.
            int                _version;         // Playlist format version.
            PlayListType       _type;            // Playlist type.
            UString            _original;        // Original URL or file name.
            UString            _fileBase;        // Base file path to resolve relative URI's (when original is a file name).
            bool               _isURL;           // The base is an URL, not a directory name.
            URL                _url;             // Original URL.
            Second             _targetDuration;  // Segment target duration (media playlist).
            size_t             _mediaSequence;   // Sequence number of first segment (media playlist).
            bool               _endList;         // End of list indicator (media playlist).
            Time               _utcDownload;     // UTC time of download.
            Time               _utcTermination;  // UTC time of termination (download + all segment durations).
            MediaSegmentQueue  _segments;        // List of media segments (media playlist).
            MediaPlayListQueue _playlists;       // List of media playlists (master playlist).
            AltPlayListQueue   _altPlaylists;    // List of alternative rendition media playlists (master playlist).
            UStringList        _loadedContent;   // Loaded text content (can be different from current content).
            UString            _autoSaveDir;     // If not empty, automatically save loaded playlist to this directory.
            UStringList        _extraTags;       // Additional tags which were manually added by the application.

            // Empty data to return.
            static const MediaSegment EmptySegment;
            static const MediaPlayList EmptyPlayList;
            static const AltPlayList EmptyAltPlayList;

            // Load from the text content.
            bool parse(const UString& text, bool strict, Report& report);
            bool parse(bool strict, Report& report);

            // Check if the line contains a valid tag or URI.
            bool getTag(const UString& line, Tag& tag, UString& params, bool strict, Report& report);
            bool isURI(const UString& line, bool strict, Report& report);

            // Perform automatic save of the loaded playlist.
            bool autoSave(Report& report);
        };
    }
}
