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

#include "tshlsPlayList.h"
#include "tshlsTagAttributes.h"
#include "tsWebRequest.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::PlayList::PlayList() :
    _valid(false),
    _version(1),
    _type(UNKNOWN_PLAYLIST),
    _original(),
    _fileBase(),
    _isURL(false),
    _url(),
    _targetDuration(0),
    _mediaSequence(0),
    _endList(false),
    _playlistType(),
    _utcDownload(),
    _utcTermination(),
    _segments(),
    _playlists(),
    _loadedContent(),
    _autoSaveDir()
{
}


//----------------------------------------------------------------------------
// Clear the content of the playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::clear()
{
    _valid = false;
    _version = 1;
    _type = UNKNOWN_PLAYLIST;
    _original.clear();
    _fileBase.clear();
    _isURL = false;
    _url.clear();
    _targetDuration = 0;
    _mediaSequence = 0;
    _endList = false;
    _playlistType.clear();
    _utcDownload = Time::Epoch;
    _utcTermination = Time::Epoch;
    _segments.clear();
    _playlists.clear();
    _loadedContent.clear();
    // Preserve _autoSaveDir
}


//----------------------------------------------------------------------------
// Reset the content of a playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::reset(ts::hls::PlayListType type, const ts::UString &filename, int version)
{
    clear();
    _valid = true;
    _version = version;
    _type = type;
    _original = AbsoluteFilePath(filename);
    _fileBase = DirectoryName(_original) + PathSeparator;
    _isURL = false;
    _url.clear();
}


//----------------------------------------------------------------------------
// Build an URL for a media segment or sub playlist.
//----------------------------------------------------------------------------

ts::UString ts::hls::PlayList::buildURL(const ts::UString& uri) const
{
    if (_isURL) {
        // Build a full URL, based on original URL.
        return URL(uri, _url).toString();
    }
    else if (uri.startWith(u"/")) {
        // The original URI was a file and the segment is an absolute file name.
        return uri;
    }
    else {
        // The original URI was a file and the segment is a relative file name.
        return _fileBase + uri;
    }
}


//----------------------------------------------------------------------------
// Set various properties in the playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setTargetDuration(ts::Second duration, Report& report)
{
    return setMember(MEDIA_PLAYLIST, &PlayList::_targetDuration, duration, report);
}

bool ts::hls::PlayList::setMediaSequence(size_t seq, Report& report)
{
    return setMember(MEDIA_PLAYLIST, &PlayList::_mediaSequence, seq, report);
}

bool ts::hls::PlayList::setEndList(bool end, Report& report)
{
    return setMember(MEDIA_PLAYLIST, &PlayList::_endList, end, report);
}

bool ts::hls::PlayList::setPlaylistType(const ts::UString& mt, Report& report)
{
    return setMember(MEDIA_PLAYLIST, &PlayList::_playlistType, mt, report);
}


//----------------------------------------------------------------------------
// Check if the playlist can be updated (and must be reloaded later).
//----------------------------------------------------------------------------

bool ts::hls::PlayList::updatable() const
{
    // See RFC 8216, sections 4.3.3.5 and 6.2.1.
    return _type == MEDIA_PLAYLIST && _playlistType != u"VOD" && !_endList;
}


//----------------------------------------------------------------------------
// Get a constant reference to a component.
//----------------------------------------------------------------------------

const ts::hls::MediaSegment ts::hls::PlayList::EmptySegment;
const ts::hls::MediaPlayList ts::hls::PlayList::EmptyPlayList;

const ts::hls::MediaSegment& ts::hls::PlayList::segment(size_t index) const
{
    return index < _segments.size() ? _segments[index] : EmptySegment;
}

bool ts::hls::PlayList::popFirstSegment()
{
    if (_segments.empty()) {
        return false;
    }
    else {
        _segments.pop_front();
        _mediaSequence++;
        return true;
    }
}

bool ts::hls::PlayList::popFirstSegment(MediaSegment& seg)
{
    if (_segments.empty()) {
        seg = EmptySegment;
        return false;
    }
    else {
        seg = _segments.front();
        _segments.pop_front();
        _mediaSequence++;
        return true;
    }
}

const ts::hls::MediaPlayList& ts::hls::PlayList::playList(size_t index) const
{
    return index < _playlists.size() ? _playlists[index] : EmptyPlayList;
}


//----------------------------------------------------------------------------
// Delete a media playlist description from a master playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::deletePlayList(size_t index)
{
    if (index < _playlists.size()) {
        _playlists.erase(_playlists.begin() + index);
    }
}


//----------------------------------------------------------------------------
// Add a segment or sub-playlist in a playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::addSegment(const ts::hls::MediaSegment& seg, ts::Report& report)
{
    if (seg.uri.empty()) {
        report.error(u"empty media segment URI");
        return false;
    }
    else if (setType(MEDIA_PLAYLIST, report)) {
        // Add the segment.
        _segments.push_back(seg);
        // Build a relative URI.
        if (!_isURL && !_original.empty()) {
            // The playlist's URI is a file name, update the segment's URI.
            _segments.back().uri = RelativeFilePath(seg.uri, _fileBase, FileSystemCaseSensitivity, true);
        }
        return true;
    }
    else {
        return false;
    }
}


bool ts::hls::PlayList::addPlayList(const ts::hls::MediaPlayList& pl, ts::Report& report)
{
    if (pl.uri.empty()) {
        report.error(u"empty media playlist URI");
        return false;
    }
    else if (setType(MASTER_PLAYLIST, report)) {
        // Add the media playlist.
        _playlists.push_back(pl);
        // Build a relative URI.
        if (!_isURL && !_original.empty()) {
            // The master playlist's URI is a file name, update the media playlist's URI.
            _playlists.back().uri = RelativeFilePath(pl.uri, _fileBase, FileSystemCaseSensitivity, true);
        }
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Select a media playlist with specific constraints.
//----------------------------------------------------------------------------

size_t ts::hls::PlayList::selectPlayList(BitRate minBitrate, BitRate maxBitrate, size_t minWidth, size_t maxWidth, size_t minHeight, size_t maxHeight) const
{
    for (size_t i = 0; i < _playlists.size(); ++i) {
        const MediaPlayList& pl(_playlists[i]);
        if ((minBitrate == 0 || pl.bandwidth >= minBitrate) &&
            (maxBitrate == 0 || (pl.bandwidth > 0 && pl.bandwidth <= maxBitrate)) &&
            (minWidth == 0 || pl.width >= minWidth) &&
            (maxWidth == 0 || (pl.width > 0 && pl.width <= maxWidth)) &&
            (minHeight == 0 || pl.height >= minHeight) &&
            (maxHeight == 0 || (pl.height > 0 && pl.height <= maxHeight)))
        {
            // Match all criteria.
            return i;
        }
    }

    // None found.
    return NPOS;
}

size_t ts::hls::PlayList::selectPlayListLowestBitRate() const
{
    size_t result = NPOS;
    BitRate ref = std::numeric_limits<BitRate>::max();
    BitRate val = 0;
    for (size_t i = 0; i < _playlists.size(); ++i) {
        if ((val = _playlists[i].bandwidth) < ref) {
            result = i;
            ref = val;
        }
    }
    return result;
}

size_t ts::hls::PlayList::selectPlayListHighestBitRate() const
{
    size_t result = NPOS;
    BitRate ref = 0;
    BitRate val = 0;
    for (size_t i = 0; i < _playlists.size(); ++i) {
        if ((val = _playlists[i].bandwidth) > ref) {
            result = i;
            ref = val;
        }
    }
    return result;
}

size_t ts::hls::PlayList::selectPlayListLowestResolution() const
{
    size_t result = NPOS;
    size_t ref = std::numeric_limits<size_t>::max();
    size_t val = 0;
    for (size_t i = 0; i < _playlists.size(); ++i) {
        if ((val = _playlists[i].width * _playlists[i].height) < ref) {
            result = i;
            ref = val;
        }
    }
    return result;
}

size_t ts::hls::PlayList::selectPlayListHighestResolution() const
{
    size_t result = NPOS;
    size_t ref = 0;
    size_t val = 0;
    for (size_t i = 0; i < _playlists.size(); ++i) {
        if ((val = _playlists[i].width * _playlists[i].height) > ref) {
            result = i;
            ref = val;
        }
    }
    return result;
}


//----------------------------------------------------------------------------
// Load the playlist from a URL.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadURL(const UString& url_string, bool strict, const WebRequestArgs args, PlayListType type, Report& report)
{
    const URL url(url_string);
    if (url.isValid()) {
        return loadURL(url, strict, args, type, report);
    }
    else {
        report.error(u"invalid URL");
        return false;
    }
}

bool ts::hls::PlayList::loadURL(const URL& url, bool strict, const WebRequestArgs args, PlayListType type, Report& report)
{
    clear();
    _type = type;

    // Keep the URL.
    _url = url;
    _original = url.toString();
    _isURL = true;

    // Build a web request to download the playlist.
    WebRequest web(report);
    web.setURL(_original);
    web.setArgs(args);
    if (args.useCookies) {
        web.enableCookies(args.cookiesFile);
    }
    else {
        web.disableCookies();
    }

    // Download the content.
    UString text;
    report.debug(u"downloading %s", {_original});
    if (!web.downloadTextContent(text)) {
        return false;
    }

    // Save the final URL in case of redirections.
    _original = web.finalURL();
    _url.setURL(_original);

    // Check MIME type of the downloaded content.
    const UString mime(web.mimeType());
    report.debug(u"MIME type: %s", {mime});

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u - OR -
    // HTTP Content-Type is application/vnd.apple.mpegurl or audio/mpegurl.
    if (strict &&
        !_original.endWith(u".m3u8", CASE_INSENSITIVE) &&
        !_original.endWith(u".m3u", CASE_INSENSITIVE) &&
        mime != u"application/vnd.apple.mpegurl" &&
        mime != u"application/mpegurl" &&
        mime != u"audio/mpegurl")
    {
        report.error(u"Invalid MIME type \"%s\" for HLS playlist at %s", {mime, _original});
        return false;
    }

    // Split content lines.
    text.remove(CARRIAGE_RETURN);
    text.split(_loadedContent, LINE_FEED, false, false);

    // Autosave if necessary, ignore errors.
    autoSave(report);

    // Load from the text.
    return parse(strict, report);
}


//----------------------------------------------------------------------------
// Load the playlist from a text file.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadFile(const UString& filename, bool strict, PlayListType type, Report& report)
{
    clear();
    _type = type;

    // Keep file name.
    _original = filename;
    _fileBase = DirectoryName(filename) + PathSeparator;
    _isURL = false;

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u.
    if (strict && !filename.endWith(u".m3u8", CASE_INSENSITIVE) && !filename.endWith(u".m3u", CASE_INSENSITIVE)) {
        report.error(u"Invalid file name extension for HLS playlist in %s", {filename});
        return false;
    }

    // Load the file.
    if (UString::Load(_loadedContent, filename)) {
        // Autosave if necessary, ignore errors.
        autoSave(report);
        // Load from the text.
        return parse(strict, report);
    }
    else {
        report.error(u"error loading %s", {filename});
        return false;
    }
}


//----------------------------------------------------------------------------
// Load the playlist from its text content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadText(const UString& text, bool strict, PlayListType type, Report& report)
{
    clear();
    _type = type;
    return parse(text, strict, report);
}


//----------------------------------------------------------------------------
// Reload a media playlist with updated content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::reload(bool strict, const WebRequestArgs args, ts::Report& report)
{
    // Playlists which cannot be reloaded are ignored (no error).
    if (_type != MEDIA_PLAYLIST || _endList || _original.empty()) {
        report.debug(u"non-reloadable playlist: %s", {_original});
        return true;
    }

    // Reload the new content in another object.
    PlayList plNew;
    if ((_isURL && !plNew.loadURL(_original, strict, args, UNKNOWN_PLAYLIST, report)) ||
        (!_isURL && !plNew.loadFile(_original, strict, UNKNOWN_PLAYLIST, report)))
    {
        return false;
    }
    assert(plNew._valid);
    report.debug(u"playlist media sequence: old: %d/%s, new: %d/%d", {_mediaSequence, _segments.size(), plNew._mediaSequence, plNew._segments.size()});

    // If no new segment is present, nothing to do.
    if (plNew._mediaSequence + plNew._segments.size() <= _mediaSequence + _segments.size()) {
        report.debug(u"no new segment in playlist");
        return true;
    }

    // Copy global characteristics.
    _version = plNew._version;
    _targetDuration = plNew._targetDuration;
    _endList = plNew._endList;
    _playlistType = plNew._playlistType;
    _utcTermination = plNew._utcTermination;
    _loadedContent.swap(plNew._loadedContent);

    // Copy missing segments.
    if (_mediaSequence + _segments.size() < plNew._mediaSequence) {
        // There are missing segments, we reloaded too late.
        report.warning(u"missed %d HLS segments, dropping %d outdated segments", {plNew._mediaSequence - _mediaSequence - _segments.size(), _segments.size()});
        // Dropping current segments, reloading fresh contiguous set of segments.
        _mediaSequence = plNew._mediaSequence;
        _segments.swap(plNew._segments);
    }
    else {
        // Start at first new segment, copy all new segments.
        for (size_t i = _mediaSequence + _segments.size() - plNew._mediaSequence; i < plNew._segments.size(); ++i) {
            _segments.push_back(plNew._segments[i]);
        }
    }

    // Autosave if necessary, ignore errors.
    autoSave(report);

    return true;
}


//----------------------------------------------------------------------------
// Load from the text content with embedded line separators.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::parse(const UString& text, bool strict, Report& report)
{
    text.toRemoved(CARRIAGE_RETURN).split(_loadedContent, LINE_FEED, false, false);
    return parse(strict, report);
}


//----------------------------------------------------------------------------
// Load from the text content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::parse(bool strict, Report& report)
{
    // Global media segment or playlist information.
    // Contains properties which are valid until next occurence of same property.
    MediaPlayList plGlobal;
    MediaSegment segGlobal;

    // Next media segment or playlist information.
    // Contains properties which are valid for next URI only.
    MediaPlayList plNext;
    MediaSegment segNext;

    // Current tag and parameters.
    Tag tag = EXTM3U;
    UString tagParams;

    // The playlist must always start with #EXTM3U.
    if (_loadedContent.empty() || !getTag(_loadedContent.front(), tag, tagParams, strict, report) || tag != EXTM3U) {
        report.error(u"invalid HLS playlist, does not start with #EXTM3U");
        return false;
    }

    // Assume valid playlist, invalidate when necessary.
    _valid = true;

    // Initial download time.
    _utcDownload = _utcTermination = Time::CurrentUTC();

    // Loop on all lines in file.
    for (auto it = _loadedContent.begin(); it != _loadedContent.end(); ++it) {

        // In non-strict mode, ignore leading and trailing spaces.
        UString line(*it);
        if (!strict) {
            line.trim();
        }
        report.log(2, u"playlist: %s", {line});

        // A line is one of blank, comment, tag, URI.
        if (isURI(line, strict, report)) {
            // URI line, add media segment or media playlist description, depending on current playlist type.
            switch (_type) {
                case MASTER_PLAYLIST:
                    // Enqueue a new playlist description.
                    plNext.uri = line;
                    _playlists.push_back(plNext);
                    // Reset description of next playlist.
                    plNext = plGlobal;
                    break;
                case MEDIA_PLAYLIST:
                    // Enqueue a new media segment.
                    segNext.uri = line;
                    _utcTermination += segNext.duration;
                    _segments.push_back(segNext);
                    // Reset description of next segment.
                    segNext = segGlobal;
                    break;
                case UNKNOWN_PLAYLIST:
                    report.debug(u"unknown URI: %s", {line});
                    _valid = false;
                    break;
                default:
                    // Should not get there.
                    assert(false);
                    break;
            }
        }
        else if (getTag(line, tag, tagParams, strict, report)) {
            // The line contains a tag.
            switch (tag) {
                case EXTM3U: {
                    if (strict && it != _loadedContent.begin()) {
                        report.error(u"misplaced: %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case VERSION: {
                    if (!tagParams.toInteger(_version) && strict) {
                        report.error(u"invalid HLS playlist version: %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case EXTINF: {
                    // #EXTINF:duration,[title]
                    const size_t comma = tagParams.find(u",");  // can be NPOS
                    if (!TagAttributes::ToMilliValue(segNext.duration, tagParams.substr(0, comma))) {
                        report.error(u"invalid segment duration in %s", {line});
                        _valid = false;
                    }
                    if (comma != NPOS) {
                        segNext.title.assign(tagParams, comma + 1);
                        segNext.title.trim();
                    }
                    break;
                }
                case BITRATE: {
                    // #EXT-X-BITRATE:<rate>
                    BitRate kilobits = 0;
                    if (tagParams.toInteger(kilobits)) {
                        segNext.bitrate = 1024 * kilobits;
                    }
                    else if (strict) {
                        report.error(u"invalid segment bitrate in %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case GAP: {
                    segNext.gap = true;
                    break;
                }
                case TARGETDURATION: {
                    // #EXT-X-TARGETDURATION:s
                    if (!tagParams.toInteger(_targetDuration) && strict) {
                        report.error(u"invalid target duration in %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case MEDIA_SEQUENCE: {
                    // #EXT-X-MEDIA-SEQUENCE:number
                    if (!tagParams.toInteger(_mediaSequence) && strict) {
                        report.error(u"invalid media sequence in %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case ENDLIST: {
                    _endList = true;
                    break;
                }
                case PLAYLIST_TYPE: {
                    _playlistType = tagParams;
                    break;
                }
                case STREAM_INF: {
                    const TagAttributes attr(tagParams);
                    attr.getIntValue(plNext.bandwidth, u"BANDWIDTH");
                    attr.getIntValue(plNext.averageBandwidth, u"AVERAGE-BANDWIDTH");
                    attr.value(u"RESOLUTION").scan(u"%dx%d", {&plNext.width, &plNext.height});
                    attr.getMilliValue(plNext.frameRate, u"FRAME-RATE");
                    plNext.codecs = attr.value(u"CODECS");
                    plNext.hdcp = attr.value(u"HDCP-LEVEL");
                    plNext.videoRange = attr.value(u"VIDEO-RANGE");
                    plNext.video = attr.value(u"VIDEO");
                    plNext.audio = attr.value(u"AUDIO");
                    plNext.subtitles = attr.value(u"SUBTITLES");
                    plNext.closedCaptions = attr.value(u"CLOSED-CAPTIONS");
                    break;
                }
                case MEDIA:
                case BYTERANGE:
                case DISCONTINUITY:
                case KEY:
                case MAP:
                case PROGRAM_DATE_TIME:
                case DATERANGE:
                case DISCONTINUITY_SEQUENCE:
                case I_FRAMES_ONLY:
                case I_FRAME_STREAM_INF:
                case SESSION_DATA:
                case SESSION_KEY:
                case INDEPENDENT_SEGMENTS:
                case START:
                case DEFINE:
                    // Currently ignored tags.
                    break;
                default:
                    // Should not get there.
                    assert(false);
                    break;
            }
        }
    }

    return _valid;
}


//----------------------------------------------------------------------------
// Check if the line contains a valid tag.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::getTag(const UString& line, Tag& tag, UString& params, bool strict, Report& report)
{
    // Check if this is a tag line.
    if (!line.startWith(u"#EXT", strict ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
        return false;
    }

    // This is a tag line. Locate the tag name (letters, digits and dash).
    size_t pos = 1;
    while (pos < line.size() && (IsAlpha(line[pos]) || IsDigit(line[pos]) || line[pos] == u'-')) {
        ++pos;
    }

    // Identify the tag. Report unknown tag but do not invalidate the playlist.
    if (!TagNames.getValue(tag, line.substr(1, pos - 1), strict)) {
        report.log(strict ? Severity::Error : Severity::Debug, u"unsupported HLS tag: %s", {line.substr(1, pos - 1)});
        return false;
    }

    // Set playlist type based on tags which are unique to a playlist type.
    const int flags = TagProperties(tag);
    if ((flags & (TAG_MASTER | TAG_MEDIA)) == TAG_MASTER) {
        // This is a master-only tag.
        setType(MASTER_PLAYLIST, report);
    }
    else if ((flags & (TAG_MASTER | TAG_MEDIA)) == TAG_MEDIA) {
        // This is a media-only tag.
        setType(MEDIA_PLAYLIST, report);
    }

    // The tag must be alone of followed by ':'.
    while (!strict && pos < line.size() && IsSpace(line[pos])) {
        ++pos;
    }
    if (pos < line.size()) {
        if (line[pos] == u':') {
            ++pos; // skip ':'
        }
        else {
            report.error(u"invalid HLS playlist line: %s", {line});
            _valid = false;
            return false;
        }
    }
    while (!strict && pos < line.size() && IsSpace(line[pos])) {
        ++pos;
    }

    // Rest of the line is the tag parameters.
    params.assign(line, pos);
    return true;
}


//----------------------------------------------------------------------------
// Check if the line contains a valid URI.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::isURI(const UString& line, bool strict, Report& report)
{
    if (line.empty() || line.startWith(u"#")) {
        // Not an URI line.
        return false;
    }

    // If the URI extension is known, set playlist type.
    if (line.endWith(u".m3u8", CASE_INSENSITIVE) || line.endWith(u".m3u", CASE_INSENSITIVE)) {
        // Reference to another playlist, this is a master playlist.
        setType(MASTER_PLAYLIST, report);
    }
    else if (line.endWith(u".ts", CASE_INSENSITIVE)) {
        // Reference to a TS file, this is a media playlist.
        setType(MEDIA_PLAYLIST, report);
    }
    else {
        report.debug(u"unexpected file extension in reference URI: %s", {line});
    }

    return true;
}


//----------------------------------------------------------------------------
// Set the playlist type, return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setType(PlayListType type, Report& report)
{
    if (_type == UNKNOWN_PLAYLIST) {
        // Type was unknown, now we know it.
        _type = type;
        return true;
    }
    else if (_type == type) {
        // Type was already known, confirmed.
        return true;
    }
    else {
        report.error(u"incompatible tags or URI in HLS playlist, cannot be both master and media playlist");
        _valid = false;
        return false;
    }
}


//----------------------------------------------------------------------------
// Perform automatic save of the loaded playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::autoSave(Report& report)
{
    if (_autoSaveDir.empty() || _original.empty()) {
        // No need to save
        return true;
    }
    else {
        const UString name(_autoSaveDir + PathSeparator + BaseName(_original));
        report.verbose(u"saving playlist to %s", {name});
        const bool ok = UString::Save(_loadedContent, name);
        if (!ok) {
            report.warning(u"error saving playlist to %s", {name});
        }
        return ok;
    }
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::hls::PlayList::toString() const
{
    UString str;

    if (_isURL) {
        const size_t slash = _original.rfind(u'/');
        str = slash == NPOS ? _original : _original.substr(slash + 1);
    }
    else {
        str = BaseName(_original);
    }
    if (!str.empty()) {
        str.append(u", ");
    }
    if (!_valid) {
        str.append(u"invalid playlist");
    }
    else if (_type == MEDIA_PLAYLIST) {
        str.append(u"media playlist");
    }
    else if (_type == MASTER_PLAYLIST) {
        str.append(u"master playlist");
    }
    else {
        str.append(u"unknown playlist");
    }
    str.append(updatable() ? u", updatable (live)" : u", static");
    if (_type == MEDIA_PLAYLIST) {
        str += UString::Format(u", %d segments", {_segments.size()});
    }
    else if (_type == MASTER_PLAYLIST) {
        str += UString::Format(u", %d media playlists", {_playlists.size()});
    }
    if (_targetDuration > 0) {
        str += UString::Format(u", %d seconds/segment", {_targetDuration});
    }
    return str;
}


//----------------------------------------------------------------------------
// Save the playlist to a text file.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::saveFile(const ts::UString &filename, ts::Report &report) const
{
    // Check that we have a valid file name to store the file.
    if (filename.empty() && (_isURL || _original.empty())) {
        report.error(u"no file name specified to store the HLS playlist");
        return false;
    }

    // Generate the text content.
    const UString text(textContent(report));
    if (text.empty()) {
        return false;
    }

    // Save the file.
    const UString& name(filename.empty() ? _original : filename);
    if (!text.save(name, false, true)) {
        report.error(u"error saving HLS playlist in %s", {name});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Build the text content of the playlist.
//----------------------------------------------------------------------------

ts::UString ts::hls::PlayList::textContent(ts::Report &report) const
{
    // Filter out invalid content.
    if (!_valid) {
        report.error(u"invalid HLS playlist content");
        return UString();
    }

    // Start building the content.
    UString text;
    text.format(u"#%s\n#%s:%d\n", {TagNames.name(EXTM3U), TagNames.name(VERSION), _version});

    switch (_type) {
        case MASTER_PLAYLIST: {
            // Loop on all media playlists.
            for (auto it = _playlists.begin(); it != _playlists.end(); ++it) {
                if (!it->uri.empty()) {
                    // The #EXT-X-STREAM-INF line must exactly preceed the URI line.
                    // Take care about string parameters: some are documented as quoted-string and
                    // some as enumerated-string. The former shall be quoted, the latter shall not.
                    text.append(UString::Format(u"#%s:BANDWIDTH=%d", {TagNames.name(STREAM_INF), it->bandwidth}));
                    if (it->averageBandwidth > 0) {
                        text.append(UString::Format(u",AVERAGE-BANDWIDTH=%d", {it->averageBandwidth}));
                    }
                    if (it->frameRate > 0) {
                        text.append(UString::Format(u",FRAME-RATE=%d.%03d", {it->frameRate / 1000, it->frameRate % 1000}));
                    }
                    if (it->width > 0 && it->height > 0) {
                        text.append(UString::Format(u",RESOLUTION=%dx%d", {it->width, it->height}));
                    }
                    if (!it->codecs.empty()) {
                        text.append(UString::Format(u",CODECS=\"%s\"", {it->codecs}));
                    }
                    if (!it->hdcp.empty()) {
                        text.append(UString::Format(u",HDCP-LEVEL=%s", {it->hdcp}));
                    }
                    if (!it->videoRange.empty()) {
                        text.append(UString::Format(u",VIDEO-RANGE=%s", {it->videoRange}));
                    }
                    if (!it->video.empty()) {
                        text.append(UString::Format(u",VIDEO=\"%s\"", {it->video}));
                    }
                    if (!it->audio.empty()) {
                        text.append(UString::Format(u",AUDIO=\"%s\"", {it->audio}));
                    }
                    if (!it->subtitles.empty()) {
                        text.append(UString::Format(u",SUBTITLES=\"%s\"", {it->subtitles}));
                    }
                    if (!it->closedCaptions.empty()) {
                        if (it->closedCaptions.similar(u"NONE")) {
                            // enumerated-string
                            text.append(u",CLOSED-CAPTIONS=NONE");
                        }
                        else {
                            // quoted-string
                            text.append(UString::Format(u",CLOSED-CAPTIONS=\"%s\"", {it->closedCaptions}));
                        }
                    }
                    // Close the #EXT-X-STREAM-INF line.
                    text.append(u'\n');
                    // The URI line must come right after #EXT-X-STREAM-INF.
                    text.append(UString::Format(u"%s\n", {it->uri}));
                }
            }
            break;
        }
        case MEDIA_PLAYLIST: {
            // Global tags.
            text.append(UString::Format(u"#%s:%d\n", {TagNames.name(TARGETDURATION), _targetDuration}));
            text.append(UString::Format(u"#%s:%d\n", {TagNames.name(MEDIA_SEQUENCE), _mediaSequence}));
            if (!_playlistType.empty()) {
                text.append(UString::Format(u"#%s:%s\n", {TagNames.name(PLAYLIST_TYPE), _playlistType}));
            }

            // Loop on all media segments.
            for (auto it = _segments.begin(); it != _segments.end(); ++it) {
                if (!it->uri.empty()) {
                    text.append(UString::Format(u"#%s:%d.%03d,%s\n", {TagNames.name(EXTINF), it->duration / MilliSecPerSec, it->duration % MilliSecPerSec, it->title}));
                    if (it->bitrate > 1024) {
                        text.append(UString::Format(u"#%s:%d\n", {TagNames.name(BITRATE), it->bitrate / 1024}));
                    }
                    if (it->gap) {
                        text.append(UString::Format(u"#%s\n", {TagNames.name(GAP)}));
                    }
                    text.append(UString::Format(u"%s\n", {it->uri}));
                }
            }

            // Mark end of list when necessary.
            if (_endList) {
                text.append(UString::Format(u"#%s\n", {TagNames.name(ENDLIST)}));
            }
            break;
        }
        case UNKNOWN_PLAYLIST:
        default: {
            report.error(u"unknown HLS playlist type (master or media playlist)");
            text.clear();
        }
    }

    return text;
}
