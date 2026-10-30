//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsStringifyInterface.h"
#include "tshls.h"
#include "tshlsMediaPlayList.h"
#include "tshlsAltPlayList.h"
#include "tshlsMediaSegment.h"
#include "tsURL.h"
#include "tsTime.h"
#include "tsWebRequestArgs.h"

namespace ts::hls {
    //!
    //! Playlist for HTTP Live Streaming (HLS).
    //! @ingroup libtsduck hls
    //!
    class TSDUCKDLL PlayList: public ReporterBase, public StringifyInterface
    {
        TS_DEFAULT_COPY_MOVE(PlayList);
    private:
        PlayList() = delete;
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit PlayList(Report* report) : ReporterBase(report) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit PlayList(ReporterBase* delegate) : ReporterBase(delegate) {}

        //!
        //! Destructor.
        //!
        virtual ~PlayList() override;

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
        //! @return True on success, false on error.
        //!
        bool loadURL(const UString& url, bool strict = false, const WebRequestArgs& args = WebRequestArgs(), PlayListType type = PlayListType::UNKNOWN);

        //!
        //! Load the playlist from a URL.
        //! @param [in] url URL from which to load the playlist.
        //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
        //! @param [in] args Web request arguments from command line.
        //! @param [in] type Expected playlist type, unknown by default.
        //! @return True on success, false on error.
        //!
        bool loadURL(const URL& url, bool strict = false, const WebRequestArgs& args = WebRequestArgs(), PlayListType type = PlayListType::UNKNOWN);

        //!
        //! Load the playlist from a text file.
        //! @param [in] filename File from which to load the playlist.
        //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
        //! @param [in] type Expected playlist type, unknown by default.
        //! @return True on success, false on error.
        //!
        bool loadFile(const UString& filename, bool strict = false, PlayListType type = PlayListType::UNKNOWN);

        //!
        //! Load the playlist from its text content.
        //! @param [in] text Text of the playlist (multi-lines).
        //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
        //! @param [in] type Expected playlist type, unknown by default.
        //! @return True on success, false on error.
        //!
        bool loadText(const UString& text, bool strict = false, PlayListType type = PlayListType::UNKNOWN);

        //!
        //! Update the URL of the playlist.
        //! @param [in] url URL from which to load the playlist.
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True on success, false on invalid URL string or incompatible embedded URLs.
        //!
        bool setURL(const UString& url, bool silent = false);

        //!
        //! Update the URL of the playlist.
        //! @param [in] url URL from which to load the playlist.
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True on success, false on invalid URL string or incompatible embedded URLs.
        //!
        bool setURL(const URL& url, bool silent = false);

        //!
        //! Update the file name of the playlist.
        //! @param [in] filename File from which to load the playlist.
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True on success, false on invalid URL string or incompatible embedded URLs.
        //!
        bool setFile(const UString& filename, bool silent = false);

        //!
        //! Reload a media playlist with updated content.
        //! Master playlists or media playlists for which endList() is true are never reloaded.
        //! Live playlists (media playlists for which endList() is false) are reloaded from the same URL.
        //! New segments are added. If a segment hole is found, all previous content is replaced.
        //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
        //! @param [in] args Web request arguments from command line.
        //! @return True on success, false on error.
        //!
        bool reload(bool strict = false, const WebRequestArgs& args = WebRequestArgs());

        //!
        //! Reload a media playlist with updated text content.
        //! Master playlists or media playlists for which endList() is true are never reloaded.
        //! New segments are added. If a segment hole is found, all previous content is replaced.
        //! @param [in] text Text of the playlist (multi-lines).
        //! @param [in] strict If true, perform strict conformance checking. By default, relaxed as long as we can understand the content.
        //! @return True on success, false on error.
        //!
        bool reloadText(const UString& text, bool strict = false);

        //!
        //! Set a directory name where all loaded files or URL are automatically saved.
        //! This property is preserved when the playlist is cleared or reloaded.
        //! @param [in] dir A directory name.
        //!
        void setAutoSaveDirectory(const UString& dir) { _auto_save_dir = dir; }

        //!
        //! Check if the playlist has been successfully loaded.
        //! @return True if the playlist has been successfully loaded.
        //!
        bool isValid() const { return _valid; }

        //!
        //! Save the playlist to a text file.
        //! @param [in] filename File where to save the playlist. By default, use the same file from loadFile() or reset().
        //! @return True on success, false on error.
        //!
        bool saveFile(const UString& filename = UString()) const;

        //!
        //! Build the text content of the playlist.
        //! @return The text content on success, an empty string on error.
        //!
        UString textContent() const;

        //!
        //! Get the original loaded text content of the playlist.
        //! This can be different from the current content of the playlist
        //! if the object has been modified.
        //! @return A constant reference to the original loaded text lines.
        //!
        const UStringList& originalLoadedContent() const { return _loaded_content; }

        //!
        //! Get the original URL.
        //! @return The original URL.
        //!
        UString url() const { return _original; }

        //!
        //! Build an URL for a media segment or sub-playlist.
        //! @param [out] media A media element with all fields filled in.
        //! @param [in] uri An URI, typically extracted from the playlist.
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True on success, false on invalid URL string or incompatible embedded URLs.
        //!
        bool buildURL(MediaElement& media, const UString& uri, bool silent = false) const;

        //!
        //! Get the playlist type.
        //! @return The playlist type.
        //!
        PlayListType type() const { return _type; }

        //!
        //! Set the playlist type.
        //! @param [in] type The playlist type.
        //! @param [in] forced When true, unconditionally set the playlist type.
        //! When false (the default), check that the playlist type does not change in an inconsistent way.
        //! Allowed changes are UNKNOWN to anything and LIVE to VOD or EVENT. The latter case is a playlist
        //! which is known to be a media playlist but for which no EXT-X-PLAYLIST-TYPE tag was found so far.
        //! @return True on success, false on error.
        //!
        bool setType(PlayListType type, bool forced = false);

        //!
        //! Set the playlist type as media playlist.
        //! If the type is already known and already a media playlist, do nothing.
        //! If the type is unknown, set it as LIVE which is a media playlist type without EXT-X-PLAYLIST-TYPE
        //! tag and which can be later turned into a VOD or EVENT playlist.
        //! @return True on success, false on error.
        //!
        bool setTypeMedia();

        //!
        //! Allow permissive URL resolution.
        //! By default, for security reasons, http: and https: playlist can only contain segments
        //! or media playlists with http: or https: URLs. If an http: or https: contains a segment
        //! file a file: URL, for instance, an error is generated. Use this method to relax this rule.
        //! This property is preserved when the playlist is cleared or reloaded.
        //! @param [in] permissive When true, the playlist allows permissive URL resolution.
        //!
        void setPermissiveURL(bool permissive) { _permissive_url = permissive; }

        //!
        //! Add a custom tag in the playlist.
        //! @param [in] tag A custom tag line to add in the playlist. If @a does not start
        //! with a @c '#', it will be automatically added.
        //!
        void addCustomTag(const UString& tag) { _extra_tags.push_back(tag); }

        //!
        //! Clear all application-specific custom tags.
        //! @see addCustomTag()
        //!
        void clearCustomTags() { _extra_tags.clear(); }

        //!
        //! Check if the playlist can be updated (and must be reloaded later).
        //! @return True if the playlist can be updated (and must be reloaded later).
        //!
        bool isUpdatable() const { return (_type == PlayListType::EVENT || _type == PlayListType::LIVE) && !_end_list; }

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
        cn::seconds targetDuration() const { return _target_duration; }

        //!
        //! Set the segment target duration in a media playlist.
        //! @param [in] duration The segment target duration in seconds.
        //! @return True on success, false on error.
        //!
        bool setTargetDuration(cn::seconds duration);

        //!
        //! Get the sequence number of first segment (in media playlist).
        //! @return The sequence number of first segment.
        //!
        size_t mediaSequence() const { return _media_sequence; }

        //!
        //! Set the sequence number of first segment in a media playlist.
        //! @param [in] seq The sequence number of first segment.
        //! @return True on success, false on error.
        //!
        bool setMediaSequence(size_t seq);

        //!
        //! Get the end of list indicator (in media playlist).
        //! @return The end of list indicator. If true, there is no need to reload
        //! the playlist, there will not be any new segment.
        //!
        bool endList() const { return _end_list; }

        //!
        //! Set the end of list indicator in a media playlist.
        //! @param [in] end The end of list indicator.
        //! @return True on success, false on error.
        //!
        bool setEndList(bool end);

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
        size_t altPlayListCount() const { return _alt_playlists.size(); }

        //!
        //! Get a constant reference to a media segment (in media playlist).
        //! @param [in] index Index of the segment, from 0 to segmentCount().
        //! @return A constant reference to the media segment at @a index.
        //!
        const MediaSegment& segment(size_t index) const;

        //!
        //! Remove the first media segment (in media playlist).
        //! @param [out] seg Characteristics of the first segment.
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True if a segment was successfully removed, false otherwise.
        //!
        bool popFirstSegment(MediaSegment& seg, bool silent = false);

        //!
        //! Remove the first media segment and do not even return it (in media playlist).
        //! @param [in] silent When true, do not report an error message in case of error.
        //! @return True of a segment was successfully removed, false otherwise.
        //!
        bool popFirstSegment(bool silent = false);

        //!
        //! Add a segment in a media playlist.
        //! @param [in] seg The new media segment to append. If the playlist's URI is a file
        //! name, the URI of the segment is transformed into a relative URI from the playlist's path.
        //! @return True on success, false on error.
        //!
        bool addSegment(const MediaSegment& seg);

        //!
        //! Get the download UTC time of the playlist.
        //! @return The download UTC time of the playlist.
        //!
        Time downloadUTC() const { return _utc_download; }

        //!
        //! Get the playout estimated termination UTC time of the playlist (in media playlist).
        //! @return The estimated playout UTC time of the playlist. This is the download time
        //! of the playlist plus the sum of all segment durations.
        //!
        Time terminationUTC() const { return _utc_termination; }

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
        //! @param [in] pl The new media playlist to append. If the master playlist's URI is a file name,
        //! the URI of the media playlist is transformed into a relative URI from the master playlist's path.
        //! @return True on success, false on error.
        //!
        bool addPlayList(const MediaPlayList& pl);

        //!
        //! Select the first media playlist with specific constraints.
        //! @param [in] min_bitrate Minimum bitrate. Zero means no minimum.
        //! @param [in] max_bitrate Maximum bitrate. Zero means no maximum.
        //! @param [in] min_width Minimum width. Zero means no minimum.
        //! @param [in] max_width Maximum width. Zero means no maximum.
        //! @param [in] min_height Minimum height. Zero means no minimum.
        //! @param [in] max_height Maximum height. Zero means no maximum.
        //! @return Index of the selected media play list or NPOS if there is none.
        //! If all criteria are zero, select the first playlist.
        //!
        size_t selectPlayList(const BitRate& min_bitrate,
                              const BitRate& max_bitrate,
                              size_t min_width,
                              size_t max_width,
                              size_t min_height,
                              size_t max_height) const;

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
        //! @return True on success, false on error.
        //!
        bool addAltPlayList(const AltPlayList& pl);

        //!
        //! Select the first alternative rendition playlist with specific criteria.
        //! @param [in] type The type to match. Ignored if empty.
        //! @param [in] name The name to match. Ignored if empty.
        //! @param [in] group_id The group-id to match. Ignored if empty.
        //! @param [in] language The language to match. Ignored if empty.
        //! @return Index of the selected alternative rendition playlist which matches all non-empty criteria
        //! or NPOS if there is none. If all criteria are empty, select the first playlist.
        //!
        size_t selectAltPlayList(const UString& type = UString(),
                                 const UString& name = UString(),
                                 const UString& group_id = UString(),
                                 const UString& language = UString()) const;

        // Implementation of StringifyInterface
        virtual UString toString() const override;

    private:
        // We need to access lists of media, with index access and fast insert at beginning and end.
        using MediaSegmentQueue = std::deque<MediaSegment>;
        using MediaPlayListQueue = std::deque<MediaPlayList>;
        using AltPlayListQueue = std::deque<AltPlayList>;

        bool               _valid = false;          // Content loaded and valid.
        bool               _permissive_url = false; // Allow incompatible URLs.
        int                _version = 1;            // Playlist format version.
        PlayListType       _type = PlayListType::UNKNOWN; // Playlist type.
        UString            _original {};            // Original URL or file name.
        UString            _file_base {};           // Base file path to resolve relative URI's (when original is a file name).
        bool               _is_url = false;         // The base is an URL, not a directory name.
        URL                _url {};                 // Original URL.
        cn::seconds        _target_duration {};     // Segment target duration (media playlist).
        size_t             _media_sequence = 0;     // Sequence number of first segment (media playlist).
        bool               _end_list = false;       // End of list indicator (media playlist).
        Time               _utc_download {};        // UTC time of download.
        Time               _utc_termination {};     // UTC time of termination (download + all segment durations).
        MediaSegmentQueue  _segments {};            // List of media segments (media playlist).
        MediaPlayListQueue _playlists {};           // List of media playlists (master playlist).
        AltPlayListQueue   _alt_playlists {};       // List of alternative rendition media playlists (master playlist).
        UStringList        _loaded_content {};      // Loaded text content (can be different from current content).
        UString            _auto_save_dir {};       // If not empty, automatically save loaded playlist to this directory.
        UStringList        _extra_tags {};          // Additional tags which were manually added by the application.

        // Empty data to return.
        static const MediaSegment& EmptySegment();
        static const MediaPlayList& EmptyPlayList();
        static const AltPlayList& EmptyAltPlayList();

        // Load from the text content.
        bool parse(const UString& text, bool strict);
        bool parse(bool strict);

        // Reload common code.
        void reload(PlayList& new_pl);

        // Update the URL or file paths of all media segments or playlists.
        bool updateReferences(bool silent);

        // Check if the line contains a valid tag or URI.
        bool getTag(const UString& line, Tag& tag, UString& params, bool strict);
        bool isURI(const UString& line, bool strict);

        // Perform automatic save of the loaded playlist.
        bool autoSave();
    };
}
