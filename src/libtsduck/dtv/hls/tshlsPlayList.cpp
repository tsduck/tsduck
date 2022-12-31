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

#include "tshlsPlayList.h"
#include "tshlsTagAttributes.h"
#include "tsWebRequest.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::PlayList::PlayList() :
    _valid(false),
    _version(1),
    _type(PlayListType::UNKNOWN),
    _original(),
    _fileBase(),
    _isURL(false),
    _url(),
    _targetDuration(0),
    _mediaSequence(0),
    _endList(false),
    _utcDownload(),
    _utcTermination(),
    _segments(),
    _playlists(),
    _altPlaylists(),
    _loadedContent(),
    _autoSaveDir(),
    _extraTags()
{
}


//----------------------------------------------------------------------------
// Clear the content of the playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::clear()
{
    _valid = false;
    _version = 1;
    _type = PlayListType::UNKNOWN;
    _original.clear();
    _fileBase.clear();
    _isURL = false;
    _url.clear();
    _targetDuration = 0;
    _mediaSequence = 0;
    _endList = false;
    _utcDownload = Time::Epoch;
    _utcTermination = Time::Epoch;
    _segments.clear();
    _playlists.clear();
    _altPlaylists.clear();
    _loadedContent.clear();
    _extraTags.clear();
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
    _extraTags.clear();
}


//----------------------------------------------------------------------------
// Build an URL for a media segment or sub playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::buildURL(MediaElement& media, const UString& uri) const
{
    media.relativeURI = uri;
    media.url.clear();

    if (_isURL) {
        // Build a full URL, based on original URL.
        media.url.setURL(uri, _url);
        media.filePath = media.url.getPath();
    }
    else if (uri.startWith(u"/")) {
        // The original URI was a file and the segment is an absolute file name.
        media.filePath = uri;
    }
    else {
        // The original URI was a file and the segment is a relative file name.
        media.filePath = _fileBase + uri;
    }
}


//----------------------------------------------------------------------------
// Set the playlist type.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setType(PlayListType type, Report& report, bool forced)
{
    if (forced ||                           // forced => no check
        _type == type ||                    // same type was already known => no change
        _type == PlayListType::UNKNOWN ||   // type was unknown => now we know it.
        (_type == PlayListType::LIVE && (type == PlayListType::VOD || type == PlayListType::EVENT)))
                                            // media playlist without EXT-X-PLAYLIST-TYPE tag ("live") => now we get a tag
    {
        _type = type;
        return true;
    }
    else {
        report.error(u"incompatible tags or URI in HLS playlist, cannot be both master, VoD and event playlist");
        _valid = false;
        return false;
    }
}


//----------------------------------------------------------------------------
// Set the playlist type as media playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setTypeMedia(Report& report)
{
    switch (_type) {
        case PlayListType::UNKNOWN:
            // Force the playlist to be a media playlist without EXT-X-PLAYLIST-TYPE tag so far.
            _type = PlayListType::LIVE;
            return true;
        case PlayListType::VOD:
        case PlayListType::EVENT:
        case PlayListType::LIVE:
            // Already a media playlist.
            return true;
        case PlayListType::MASTER:
        default:
            report.error(u"incompatible tags or URI in HLS playlist, cannot be both master and media playlist");
            _valid = false;
            return false;
    }
}


//----------------------------------------------------------------------------
// Set various properties in the playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setTargetDuration(ts::Second duration, Report& report)
{
    if (setTypeMedia(report)) {
        _targetDuration = duration;
        return true;
    }
    else {
        return false;
    }
}

bool ts::hls::PlayList::setMediaSequence(size_t seq, Report& report)
{
    if (setTypeMedia(report)) {
        _mediaSequence = seq;
        return true;
    }
    else {
        return false;
    }
}

bool ts::hls::PlayList::setEndList(bool end, Report& report)
{
    if (setTypeMedia(report)) {
        _endList = end;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Get a constant reference to a component.
//----------------------------------------------------------------------------

const ts::hls::MediaSegment ts::hls::PlayList::EmptySegment;
const ts::hls::MediaPlayList ts::hls::PlayList::EmptyPlayList;
const ts::hls::AltPlayList ts::hls::PlayList::EmptyAltPlayList;

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

const ts::hls::AltPlayList& ts::hls::PlayList::altPlayList(size_t index) const
{
    return index < _altPlaylists.size() ? _altPlaylists[index] : EmptyAltPlayList;
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

void ts::hls::PlayList::deleteAltPlayList(size_t index)
{
    if (index < _altPlaylists.size()) {
        _altPlaylists.erase(_altPlaylists.begin() + index);
    }
}


//----------------------------------------------------------------------------
// Add a segment or sub-playlist in a playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::addSegment(const MediaSegment& seg, Report& report)
{
    if (seg.relativeURI.empty()) {
        report.error(u"empty media segment URI");
        return false;
    }
    else if (setTypeMedia(report)) {
        // Add the segment.
        _segments.push_back(seg);
        // Build a relative URI.
        if (!_isURL && !_original.empty()) {
            // The playlist's URI is a file name, update the segment's URI.
            _segments.back().relativeURI = RelativeFilePath(seg.relativeURI, _fileBase, FileSystemCaseSensitivity, true);
        }
        return true;
    }
    else {
        return false;
    }
}


bool ts::hls::PlayList::addPlayList(const MediaPlayList& pl, Report& report)
{
    if (pl.relativeURI.empty()) {
        report.error(u"empty media playlist URI");
        return false;
    }
    else if (setType(PlayListType::MASTER, report)) {
        // Add the media playlist.
        _playlists.push_back(pl);
        // Build a relative URI.
        if (!_isURL && !_original.empty()) {
            // The master playlist's URI is a file name, update the media playlist's URI.
            _playlists.back().relativeURI = RelativeFilePath(pl.relativeURI, _fileBase, FileSystemCaseSensitivity, true);
        }
        return true;
    }
    else {
        return false;
    }
}


bool ts::hls::PlayList::addAltPlayList(const AltPlayList& pl, Report& report)
{
    if (setType(PlayListType::MASTER, report)) {
        // Add the media playlist.
        _altPlaylists.push_back(pl);
        // Build a relative URI if there is one (the URI field is optional in an alternative rendition playlist).
        if (!pl.relativeURI.empty() && !_isURL && !_original.empty()) {
            // The master playlist's URI is a file name, update the media playlist's URI.
            _altPlaylists.back().relativeURI = RelativeFilePath(pl.relativeURI, _fileBase, FileSystemCaseSensitivity, true);
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

size_t ts::hls::PlayList::selectPlayList(const BitRate& minBitrate, const BitRate& maxBitrate, size_t minWidth, size_t maxWidth, size_t minHeight, size_t maxHeight) const
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
    BitRate ref = BitRate::MAX;
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
// Select the first alternative rendition playlist with specific criteria.
//----------------------------------------------------------------------------

size_t ts::hls::PlayList::selectAltPlayList(const UString& type, const UString& name, const UString& groupId, const UString& language) const
{
    for (size_t i = 0; i < _altPlaylists.size(); ++i) {
        const AltPlayList& pl(_altPlaylists[i]);
        if ((type.empty() || pl.type.similar(type)) &&
            (name.empty() || pl.name.similar(name)) &&
            (groupId.empty() || pl.groupId.similar(groupId)) &&
            (language.empty() || pl.language.similar(language)))
        {
            // Match all criteria.
            return i;
        }
    }

    // None found.
    return NPOS;
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
    if (!web.downloadTextContent(_original, text)) {
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
    if (!isUpdatable() || _original.empty()) {
        report.debug(u"non-reloadable playlist: %s", {_original});
        return true;
    }

    // Reload the new content in another object.
    PlayList plNew;
    if ((_isURL && !plNew.loadURL(_original, strict, args, PlayListType::UNKNOWN, report)) ||
        (!_isURL && !plNew.loadFile(_original, strict, PlayListType::UNKNOWN, report)))
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
    _type = plNew._type;
    _version = plNew._version;
    _targetDuration = plNew._targetDuration;
    _endList = plNew._endList;
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
    uint32_t lineNumber = 0;
    for (const auto& it : _loadedContent) {

        // In non-strict mode, ignore leading and trailing spaces.
        UString line(it);
        if (!strict) {
            line.trim();
        }
        lineNumber++;
        report.log(2, u"playlist: %s", {line});

        // A line is one of blank, comment, tag, URI.
        if (isURI(line, strict, report)) {
            // URI line, add media segment or media playlist description, depending on current playlist type.
            if (isMaster()) {
                // Enqueue a new playlist description.
                buildURL(plNext, line);
                _playlists.push_back(plNext);
                if (!plNext.filePath.endWith(u".m3u8", CASE_INSENSITIVE)) {
                    report.debug(u"unexpected playlist file extension in reference URI: %s", {line});
                }
                // Reset description of next playlist.
                plNext = plGlobal;
            }
            else if (isMedia()) {
                // Enqueue a new media segment.
                buildURL(segNext, line);
                _utcTermination += segNext.duration;
                _segments.push_back(segNext);
                if (!segNext.filePath.endWith(u".ts", CASE_INSENSITIVE)) {
                    report.debug(u"unexpected segment file extension in reference URI: %s", {line});
                }
                // Reset description of next segment.
                segNext = segGlobal;
            }
            else {
                report.debug(u"unknown URI: %s", {line});
                _valid = false;
            }
        }
        else if (getTag(line, tag, tagParams, strict, report)) {
            // The line contains a tag.
            switch (tag) {
                case EXTM3U: {
                    if (strict && lineNumber > 1) {
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
                    // Apply to next segment only.
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
                    if (kilobits.fromString(tagParams)) {
                        // Apply to one or more segments.
                        segGlobal.bitrate = segNext.bitrate = 1024 * kilobits;
                    }
                    else if (strict) {
                        report.error(u"invalid segment bitrate in %s", {line});
                        _valid = false;
                    }
                    break;
                }
                case GAP: {
                    // #EXT-X-GAP
                    // Apply to next segment only.
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
                    if (tagParams.similar(u"VOD")) {
                        setType(PlayListType::VOD, report);
                    }
                    else if (tagParams.similar(u"EVENT")) {
                        setType(PlayListType::EVENT, report);
                    }
                    else {
                        report.error(u"invalid playlist type '%s' in %s", {tagParams, line});
                        _valid = false;
                    }
                    break;
                }
                case STREAM_INF: {
                    // #EXT-X-STREAM-INF:<attribute-list>
                    // Apply to next playlist only.
                    const TagAttributes attr(tagParams);
                    attr.getValue(plNext.bandwidth, u"BANDWIDTH");
                    attr.getValue(plNext.averageBandwidth, u"AVERAGE-BANDWIDTH");
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
                case MEDIA: {
                    // #EXT-X-MEDIA:<attribute-list>
                    const TagAttributes attr(tagParams);
                    AltPlayList pl;
                    pl.name = attr.value(u"NAME");
                    pl.type = attr.value(u"TYPE");
                    pl.groupId = attr.value(u"GROUP-ID");
                    pl.stableRenditionId = attr.value(u"STABLE-RENDITION-ID");
                    pl.language = attr.value(u"LANGUAGE");
                    pl.assocLanguage = attr.value(u"ASSOC-LANGUAGE");
                    pl.inStreamId = attr.value(u"INSTREAM-ID");
                    pl.characteristics = attr.value(u"CHARACTERISTICS");
                    pl.channels = attr.value(u"CHANNELS");
                    pl.isDefault = attr.value(u"DEFAULT").similar(u"YES");
                    pl.autoselect = attr.value(u"AUTOSELECT").similar(u"YES");
                    pl.forced = attr.value(u"FORCED").similar(u"YES");
                    const UString uri(attr.value(u"URI"));
                    if (!uri.empty()) {
                        buildURL(pl, uri);
                        if (!pl.filePath.endWith(u".m3u8", CASE_INSENSITIVE)) {
                            report.debug(u"unexpected playlist file extension in reference URI: %s", {uri});
                        }
                    }
                    _altPlaylists.push_back(pl);
                    break;
                }
                case BYTERANGE:
                case DISCONTINUITY:
                case KEY:
                case MAP:
                case PROGRAM_DATE_TIME:
                case DATERANGE:
                case SKIP:
                case PRELOAD_HINT:
                case RENDITION_REPORT:
                case DISCONTINUITY_SEQUENCE:
                case I_FRAMES_ONLY:
                case PART_INF:
                case SERVER_CONTROL:
                case I_FRAME_STREAM_INF:
                case SESSION_DATA:
                case SESSION_KEY:
                case CONTENT_STEERING:
                case INDEPENDENT_SEGMENTS:
                case START:
                case DEFINE:
                case PART:
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
        setType(PlayListType::MASTER, report);
    }
    else if ((flags & (TAG_MASTER | TAG_MEDIA)) == TAG_MEDIA) {
        // This is a media-only tag.
        setTypeMedia(report);
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

    // Build a full path of the URI and extract the path name (without trailing query or fragment).
    MediaElement me;
    buildURL(me, line);
    const UString name(me.url.isValid() ? me.url.getPath() : me.filePath);

    // If the URI extension is known, set playlist type.
    if (name.endWith(u".m3u8", CASE_INSENSITIVE) || name.endWith(u".m3u", CASE_INSENSITIVE)) {
        // Reference to another playlist, this is a master playlist.
        setType(PlayListType::MASTER, report);
    }
    else if (name.endWith(u".ts", CASE_INSENSITIVE)) {
        // Reference to a TS file, this is a media playlist.
        setTypeMedia(report);
    }

    return true;
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
    else if (isMedia()) {
        str.append(u"media playlist");
    }
    else if (isMaster()) {
        str.append(u"master playlist");
    }
    else {
        str.append(u"unknown playlist");
    }
    str.append(isUpdatable() ? u", updatable (live)" : u", static");
    if (isMedia()) {
        str.format(u", %d segments", {_segments.size()});
    }
    else if (_type == PlayListType::MASTER) {
        str.format(u", %d media playlists", {_playlists.size()});
        if (!_altPlaylists.empty()) {
            str.format(u", %d alternative rendition playlists", {_altPlaylists.size()});
        }
    }
    if (_targetDuration > 0) {
        str.format(u", %d seconds/segment", {_targetDuration});
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

    // Insert application-specific tags before standard tags.
    for (const auto& tag : _extraTags) {
        text.format(u"%s%s\n", {tag.startWith(u"#") ? u"" : u"#", tag});
    }

    if (isMaster()) {
        // Loop on all alternative rendition playlists.
        for (const auto& pl : _altPlaylists) {
            // The initial fields are required.
            text.format(u"#%s:TYPE=%s,GROUP-ID=\"%s\",NAME=\"%s\"", {TagNames.name(MEDIA), pl.type, pl.groupId, pl.name});
            if (pl.isDefault) {
                text.append(u",DEFAULT=YES");
            }
            if (pl.autoselect) {
                text.append(u",AUTOSELECT=YES");
            }
            if (pl.forced) {
                text.append(u",FORCED=YES");
            }
            if (!pl.language.empty()) {
                text.format(u",LANGUAGE=\"%s\"", {pl.language});
            }
            if (!pl.assocLanguage.empty()) {
                text.format(u",ASSOC-LANGUAGE=\"%s\"", {pl.assocLanguage});
            }
            if (!pl.stableRenditionId.empty()) {
                text.format(u",STABLE-RENDITION-ID=\"%s\"", {pl.stableRenditionId});
            }
            if (!pl.inStreamId.empty()) {
                text.format(u",INSTREAM-ID=\"%s\"", {pl.inStreamId});
            }
            if (!pl.characteristics.empty()) {
                text.format(u",CHARACTERISTICS=\"%s\"", {pl.characteristics});
            }
            if (!pl.channels.empty()) {
                text.format(u",CHANNELS=\"%s\"", {pl.channels});
            }
            if (!pl.relativeURI.empty()) {
                text.format(u",URI=\"%s\"", {pl.relativeURI});
            }
            // Close the #EXT-X-MEDIA line.
            text.append(u'\n');
        }
        // Loop on all media playlists.
        for (const auto& pl : _playlists) {
            if (!pl.relativeURI.empty()) {
                // The #EXT-X-STREAM-INF line must exactly preceed the URI line.
                // Take care about string parameters: some are documented as quoted-string and
                // some as enumerated-string. The former shall be quoted, the latter shall not.
                text.format(u"#%s:BANDWIDTH=%d", {TagNames.name(STREAM_INF), pl.bandwidth.toInt()});
                if (pl.averageBandwidth > 0) {
                    text.format(u",AVERAGE-BANDWIDTH=%d", {pl.averageBandwidth.toInt()});
                }
                if (pl.frameRate > 0) {
                    text.format(u",FRAME-RATE=%d.%03d", {pl.frameRate / 1000, pl.frameRate % 1000});
                }
                if (pl.width > 0 && pl.height > 0) {
                    text.format(u",RESOLUTION=%dx%d", {pl.width, pl.height});
                }
                if (!pl.codecs.empty()) {
                    text.format(u",CODECS=\"%s\"", {pl.codecs});
                }
                if (!pl.hdcp.empty()) {
                    text.format(u",HDCP-LEVEL=%s", {pl.hdcp});
                }
                if (!pl.videoRange.empty()) {
                    text.format(u",VIDEO-RANGE=%s", {pl.videoRange});
                }
                if (!pl.video.empty()) {
                    text.format(u",VIDEO=\"%s\"", {pl.video});
                }
                if (!pl.audio.empty()) {
                    text.format(u",AUDIO=\"%s\"", {pl.audio});
                }
                if (!pl.subtitles.empty()) {
                    text.format(u",SUBTITLES=\"%s\"", {pl.subtitles});
                }
                if (!pl.closedCaptions.empty()) {
                    if (pl.closedCaptions.similar(u"NONE")) {
                        // enumerated-string
                        text.append(u",CLOSED-CAPTIONS=NONE");
                    }
                    else {
                        // quoted-string
                        text.format(u",CLOSED-CAPTIONS=\"%s\"", {pl.closedCaptions});
                    }
                }
                // Close the #EXT-X-STREAM-INF line.
                text.append(u'\n');
                // The URI line must come right after #EXT-X-STREAM-INF.
                text.format(u"%s\n", {pl.relativeURI});
            }
        }
    }
    else if (isMedia()) {
        // Global tags.
        text.format(u"#%s:%d\n", {TagNames.name(TARGETDURATION), _targetDuration});
        text.format(u"#%s:%d\n", {TagNames.name(MEDIA_SEQUENCE), _mediaSequence});
        if (_type == PlayListType::VOD) {
            text.format(u"#%s:VOD\n", {TagNames.name(PLAYLIST_TYPE)});
        }
        else if (_type == PlayListType::EVENT) {
            text.format(u"#%s:EVENT\n", {TagNames.name(PLAYLIST_TYPE)});
        }

        // Loop on all media segments.
        for (const auto& seg : _segments) {
            if (!seg.relativeURI.empty()) {
                text.format(u"#%s:%d.%03d,%s\n", {TagNames.name(EXTINF), seg.duration / MilliSecPerSec, seg.duration % MilliSecPerSec, seg.title});
                if (seg.bitrate > 1024) {
                    text.format(u"#%s:%d\n", {TagNames.name(BITRATE), (seg.bitrate / 1024).toInt()});
                }
                if (seg.gap) {
                    text.format(u"#%s\n", {TagNames.name(GAP)});
                }
                text.format(u"%s\n", {seg.relativeURI});
            }
        }

        // Mark end of list when necessary.
        if (_endList) {
            text.format(u"#%s\n", {TagNames.name(ENDLIST)});
        }
    }
    else {
        report.error(u"unknown HLS playlist type (master or media playlist)");
        text.clear();
    }

    return text;
}
