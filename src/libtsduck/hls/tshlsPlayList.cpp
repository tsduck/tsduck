//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsPlayList.h"
#include "tshlsTagAttributes.h"
#include "tsWebRequest.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Clear the content of the playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::clear()
{
    _valid = false;
    _version = 1;
    _type = PlayListType::UNKNOWN;
    _original.clear();
    _file_base.clear();
    _is_url = false;
    _url.clear();
    _target_duration = cn::seconds::zero();
    _media_sequence = 0;
    _end_list = false;
    _utc_download = Time::Epoch;
    _utc_termination = Time::Epoch;
    _segments.clear();
    _playlists.clear();
    _alt_playlists.clear();
    _loaded_content.clear();
    _extra_tags.clear();
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
    _file_base = DirectoryName(_original) + fs::path::preferred_separator;
    _is_url = false;
    _url.clear();
    _extra_tags.clear();
}


//----------------------------------------------------------------------------
// Build an URL for a media segment or sub playlist.
//----------------------------------------------------------------------------

void ts::hls::PlayList::buildURL(MediaElement& media, const UString& uri) const
{
    media.relative_uri = uri;
    media.url.clear();

    if (_is_url) {
        // Build a full URL, based on original URL.
        media.url.setURL(uri, _url);
        media.file_path = media.url.getPath();
    }
    else if (uri.starts_with(u"/")) {
        // The original URI was a file and the segment is an absolute file name.
        media.file_path = uri;
    }
    else {
        // The original URI was a file and the segment is a relative file name.
        media.file_path = _file_base + uri;
    }
}

//----------------------------------------------------------------------------
// Update the URL or file paths of all media segments or playlists.
//----------------------------------------------------------------------------

void ts::hls::PlayList::updateReferences()
{
    for (auto& me : _segments) {
        buildURL(me, me.relative_uri);
    }
    for (auto& me : _playlists) {
        buildURL(me, me.relative_uri);
    }
    for (auto& me : _alt_playlists) {
        buildURL(me, me.relative_uri);
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

bool ts::hls::PlayList::setTargetDuration(cn::seconds duration, Report& report)
{
    if (setTypeMedia(report)) {
        _target_duration = duration;
        return true;
    }
    else {
        return false;
    }
}

bool ts::hls::PlayList::setMediaSequence(size_t seq, Report& report)
{
    if (setTypeMedia(report)) {
        _media_sequence = seq;
        return true;
    }
    else {
        return false;
    }
}

bool ts::hls::PlayList::setEndList(bool end, Report& report)
{
    if (setTypeMedia(report)) {
        _end_list = end;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Get a constant reference to a component.
//----------------------------------------------------------------------------

const ts::hls::MediaSegment& ts::hls::PlayList::EmptySegment()
{
    static const MediaSegment data;
    return data;
}

const ts::hls::MediaPlayList& ts::hls::PlayList::EmptyPlayList()
{
    static const MediaPlayList data;
    return data;
}

const ts::hls::AltPlayList& ts::hls::PlayList::EmptyAltPlayList()
{
    static const AltPlayList data;
    return data;
}

const ts::hls::MediaSegment& ts::hls::PlayList::segment(size_t index) const
{
    return index < _segments.size() ? _segments[index] : EmptySegment();
}

bool ts::hls::PlayList::popFirstSegment()
{
    if (_segments.empty()) {
        return false;
    }
    else {
        _segments.pop_front();
        _media_sequence++;
        return true;
    }
}

bool ts::hls::PlayList::popFirstSegment(MediaSegment& seg)
{
    if (_segments.empty()) {
        seg = EmptySegment();
        return false;
    }
    else {
        seg = _segments.front();
        _segments.pop_front();
        _media_sequence++;
        return true;
    }
}

const ts::hls::MediaPlayList& ts::hls::PlayList::playList(size_t index) const
{
    return index < _playlists.size() ? _playlists[index] : EmptyPlayList();
}

const ts::hls::AltPlayList& ts::hls::PlayList::altPlayList(size_t index) const
{
    return index < _alt_playlists.size() ? _alt_playlists[index] : EmptyAltPlayList();
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
    if (index < _alt_playlists.size()) {
        _alt_playlists.erase(_alt_playlists.begin() + index);
    }
}


//----------------------------------------------------------------------------
// Add a segment or sub-playlist in a playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::addSegment(const MediaSegment& seg, Report& report)
{
    if (seg.relative_uri.empty()) {
        report.error(u"empty media segment URI");
        return false;
    }
    else if (setTypeMedia(report)) {
        // Add the segment.
        _segments.push_back(seg);
        // Build a relative URI.
        if (!_is_url && !_original.empty()) {
            // The playlist's URI is a file name, update the segment's URI.
            _segments.back().relative_uri = RelativeFilePath(seg.relative_uri, _file_base, FILE_SYSTEM_CASE_SENSITVITY, true);
        }
        return true;
    }
    else {
        return false;
    }
}


bool ts::hls::PlayList::addPlayList(const MediaPlayList& pl, Report& report)
{
    if (pl.relative_uri.empty()) {
        report.error(u"empty media playlist URI");
        return false;
    }
    else if (setType(PlayListType::MASTER, report)) {
        // Add the media playlist.
        _playlists.push_back(pl);
        // Build a relative URI.
        if (!_is_url && !_original.empty()) {
            // The master playlist's URI is a file name, update the media playlist's URI.
            _playlists.back().relative_uri = RelativeFilePath(pl.relative_uri, _file_base, FILE_SYSTEM_CASE_SENSITVITY, true);
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
        _alt_playlists.push_back(pl);
        // Build a relative URI if there is one (the URI field is optional in an alternative rendition playlist).
        if (!pl.relative_uri.empty() && !_is_url && !_original.empty()) {
            // The master playlist's URI is a file name, update the media playlist's URI.
            _alt_playlists.back().relative_uri = RelativeFilePath(pl.relative_uri, _file_base, FILE_SYSTEM_CASE_SENSITVITY, true);
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

size_t ts::hls::PlayList::selectPlayList(const BitRate& min_bitrate, const BitRate& max_bitrate, size_t min_width, size_t max_width, size_t min_height, size_t max_height) const
{
    for (size_t i = 0; i < _playlists.size(); ++i) {
        const MediaPlayList& pl(_playlists[i]);
        if ((min_bitrate == 0 || pl.bandwidth >= min_bitrate) &&
            (max_bitrate == 0 || (pl.bandwidth > 0 && pl.bandwidth <= max_bitrate)) &&
            (min_width == 0 || pl.width >= min_width) &&
            (max_width == 0 || (pl.width > 0 && pl.width <= max_width)) &&
            (min_height == 0 || pl.height >= min_height) &&
            (max_height == 0 || (pl.height > 0 && pl.height <= max_height)))
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

size_t ts::hls::PlayList::selectAltPlayList(const UString& type, const UString& name, const UString& group_id, const UString& language) const
{
    for (size_t i = 0; i < _alt_playlists.size(); ++i) {
        const AltPlayList& pl(_alt_playlists[i]);
        if ((type.empty() || pl.type.similar(type)) &&
            (name.empty() || pl.name.similar(name)) &&
            (group_id.empty() || pl.group_id.similar(group_id)) &&
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
// Update the URL or filename of the playlist.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::setURL(const UString& url_string, Report& report)
{
    const URL url(url_string);
    if (url.isValid()) {
        setURL(url);
        return true;
    }
    else {
        report.error(u"invalid URL: %s", url_string);
        return false;
    }
}

void ts::hls::PlayList::setURL(const URL& url)
{
    _url = url;
    _original = url.toString();
    _is_url = true;
    updateReferences();
}

void ts::hls::PlayList::setFile(const UString& filename)
{
    _original = filename;
    _file_base = DirectoryName(filename) + fs::path::preferred_separator;
    _is_url = false;
    updateReferences();
}


//----------------------------------------------------------------------------
// Load the playlist from a URL.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadURL(const UString& url_string, bool strict, const WebRequestArgs& args, PlayListType type, Report& report)
{
    const URL url(url_string);
    if (url.isValid()) {
        return loadURL(url, strict, args, type, report);
    }
    else {
        report.error(u"invalid URL: %s", url_string);
        return false;
    }
}

bool ts::hls::PlayList::loadURL(const URL& url, bool strict, const WebRequestArgs& args, PlayListType type, Report& report)
{
    clear();
    setURL(url);
    _type = type;

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
    report.debug(u"downloading %s", _original);
    if (!web.downloadTextContent(_original, text)) {
        return false;
    }

    // Save the final URL in case of redirections.
    _original = web.finalURL();
    _url.setURL(_original);

    // Check MIME type of the downloaded content.
    const UString mime(web.mimeType());
    report.debug(u"MIME type: %s", mime);

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u - OR -
    // HTTP Content-Type is application/vnd.apple.mpegurl or audio/mpegurl.
    if (strict &&
        !_original.ends_with(u".m3u8", CASE_INSENSITIVE) &&
        !_original.ends_with(u".m3u", CASE_INSENSITIVE) &&
        mime != u"application/vnd.apple.mpegurl" &&
        mime != u"application/mpegurl" &&
        mime != u"audio/mpegurl")
    {
        report.error(u"Invalid MIME type \"%s\" for HLS playlist at %s", mime, _original);
        return false;
    }

    // Split content lines.
    text.remove(CARRIAGE_RETURN);
    text.split(_loaded_content, LINE_FEED, false, false);

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
    setFile(filename);
    _type = type;

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u.
    if (strict && !filename.ends_with(u".m3u8", CASE_INSENSITIVE) && !filename.ends_with(u".m3u", CASE_INSENSITIVE)) {
        report.error(u"Invalid file name extension for HLS playlist in %s", filename);
        return false;
    }

    // Load the file.
    if (UString::Load(_loaded_content, filename)) {
        // Autosave if necessary, ignore errors.
        autoSave(report);
        // Load from the text.
        return parse(strict, report);
    }
    else {
        report.error(u"error loading %s", filename);
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

bool ts::hls::PlayList::reload(bool strict, const WebRequestArgs& args, ts::Report& report)
{
    // Playlists which cannot be reloaded are ignored (no error).
    if (!isUpdatable() || _original.empty()) {
        report.debug(u"non-reloadable playlist: %s", _original);
        return true;
    }

    // Reload the new content in another object.
    PlayList new_pl;
    if ((_is_url && !new_pl.loadURL(_original, strict, args, PlayListType::UNKNOWN, report)) ||
        (!_is_url && !new_pl.loadFile(_original, strict, PlayListType::UNKNOWN, report)))
    {
        return false;
    }

    // Then update the content.
    reload(new_pl, report);

    // Autosave if necessary, ignore errors.
    autoSave(report);
    return true;
}


//----------------------------------------------------------------------------
// Reload a media playlist with updated text content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::reloadText(const UString& text, bool strict, Report& report)
{
    // Load the new content in another object.
    PlayList new_pl;
    if (!new_pl.loadText(text, strict, PlayListType::UNKNOWN, report)) {
        return false;
    }

    // Then update the content.
    reload(new_pl, report);
    return true;
}


//----------------------------------------------------------------------------
// Reload common code.
//----------------------------------------------------------------------------

void ts::hls::PlayList::reload(PlayList& new_pl, Report& report)
{
    assert(new_pl._valid);
    report.debug(u"playlist media sequence: old: %d/%s, new: %d/%d", _media_sequence, _segments.size(), new_pl._media_sequence, new_pl._segments.size());

    // If no new segment is present, nothing to do.
    if (new_pl._media_sequence + new_pl._segments.size() <= _media_sequence + _segments.size()) {
        report.debug(u"no new segment in playlist");
        return;
    }

    // Copy global characteristics.
    _type = new_pl._type;
    _version = new_pl._version;
    _target_duration = new_pl._target_duration;
    _end_list = new_pl._end_list;
    _utc_termination = new_pl._utc_termination;
    _loaded_content.swap(new_pl._loaded_content);

    // Copy missing segments.
    if (_media_sequence + _segments.size() < new_pl._media_sequence) {
        // There are missing segments, we reloaded too late.
        report.warning(u"missed %d HLS segments, dropping %d outdated segments", new_pl._media_sequence - _media_sequence - _segments.size(), _segments.size());
        // Dropping current segments, reloading fresh contiguous set of segments.
        _media_sequence = new_pl._media_sequence;
        _segments.swap(new_pl._segments);
    }
    else {
        // Start at first new segment, copy all new segments.
        for (size_t i = _media_sequence + _segments.size() - new_pl._media_sequence; i < new_pl._segments.size(); ++i) {
            _segments.push_back(new_pl._segments[i]);
        }
    }
}


//----------------------------------------------------------------------------
// Load from the text content with embedded line separators.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::parse(const UString& text, bool strict, Report& report)
{
    text.toRemoved(CARRIAGE_RETURN).split(_loaded_content, LINE_FEED, false, false);
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
    Tag tag = Tag::EXTM3U;
    UString tagParams;

    // The playlist must always start with #EXTM3U.
    if (_loaded_content.empty() || !getTag(_loaded_content.front(), tag, tagParams, strict, report) || tag != Tag::EXTM3U) {
        report.error(u"invalid HLS playlist, does not start with #EXTM3U");
        return false;
    }

    // Assume valid playlist, invalidate when necessary.
    _valid = true;

    // Initial download time.
    _utc_download = _utc_termination = Time::CurrentUTC();

    // Loop on all lines in file.
    uint32_t lineNumber = 0;
    for (const auto& it : _loaded_content) {

        // In non-strict mode, ignore leading and trailing spaces.
        UString line(it);
        if (!strict) {
            line.trim();
        }
        lineNumber++;
        report.log(2, u"playlist: %s", line);

        // A line is one of blank, comment, tag, URI.
        if (isURI(line, strict, report)) {
            // URI line, add media segment or media playlist description, depending on current playlist type.
            if (isMaster()) {
                // Enqueue a new playlist description.
                buildURL(plNext, line);
                _playlists.push_back(plNext);
                if (!plNext.file_path.ends_with(u".m3u8", CASE_INSENSITIVE)) {
                    report.debug(u"unexpected playlist file extension in reference URI: %s", line);
                }
                // Reset description of next playlist.
                plNext = plGlobal;
            }
            else if (isMedia()) {
                // Enqueue a new media segment.
                buildURL(segNext, line);
                _utc_termination += segNext.duration;
                _segments.push_back(segNext);
                if (!segNext.file_path.ends_with(u".ts", CASE_INSENSITIVE)) {
                    report.debug(u"unexpected segment file extension in reference URI: %s", line);
                }
                // Reset description of next segment.
                segNext = segGlobal;
            }
            else {
                report.debug(u"unknown URI: %s", line);
                _valid = false;
            }
        }
        else if (getTag(line, tag, tagParams, strict, report)) {
            // The line contains a tag.
            switch (tag) {
                case Tag::EXTM3U: {
                    if (strict && lineNumber > 1) {
                        report.error(u"misplaced: %s", line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::VERSION: {
                    if (!tagParams.toInteger(_version) && strict) {
                        report.error(u"invalid HLS playlist version: %s", line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::EXTINF: {
                    // #EXTINF:duration,[title]
                    // Apply to next segment only.
                    const size_t comma = tagParams.find(u",");  // can be NPOS
                    if (!TagAttributes::ToMilliValue(segNext.duration, tagParams.substr(0, comma))) {
                        report.error(u"invalid segment duration in %s", line);
                        _valid = false;
                    }
                    if (comma != NPOS) {
                        segNext.title.assign(tagParams, comma + 1);
                        segNext.title.trim();
                    }
                    break;
                }
                case Tag::BITRATE: {
                    // #EXT-X-BITRATE:<rate>
                    BitRate kilobits = 0;
                    if (kilobits.fromString(tagParams)) {
                        // Apply to one or more segments.
                        segGlobal.bitrate = segNext.bitrate = 1024 * kilobits;
                    }
                    else if (strict) {
                        report.error(u"invalid segment bitrate in %s", line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::GAP: {
                    // #EXT-X-GAP
                    // Apply to next segment only.
                    segNext.gap = true;
                    break;
                }
                case Tag::TARGETDURATION: {
                    // #EXT-X-TARGETDURATION:s
                    if (!tagParams.toChrono(_target_duration) && strict) {
                        report.error(u"invalid target duration in %s", line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::MEDIA_SEQUENCE: {
                    // #EXT-X-MEDIA-SEQUENCE:number
                    if (!tagParams.toInteger(_media_sequence) && strict) {
                        report.error(u"invalid media sequence in %s", line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::ENDLIST: {
                    _end_list = true;
                    break;
                }
                case Tag::PLAYLIST_TYPE: {
                    if (tagParams.similar(u"VOD")) {
                        setType(PlayListType::VOD, report);
                    }
                    else if (tagParams.similar(u"EVENT")) {
                        setType(PlayListType::EVENT, report);
                    }
                    else {
                        report.error(u"invalid playlist type '%s' in %s", tagParams, line);
                        _valid = false;
                    }
                    break;
                }
                case Tag::STREAM_INF: {
                    // #EXT-X-STREAM-INF:<attribute-list>
                    // Apply to next playlist only.
                    const TagAttributes attr(tagParams);
                    attr.getValue(plNext.bandwidth, u"BANDWIDTH");
                    attr.getValue(plNext.average_bandwidth, u"AVERAGE-BANDWIDTH");
                    attr.value(u"RESOLUTION").scan(u"%dx%d", &plNext.width, &plNext.height);
                    attr.getMilliValue(plNext.frame_rate, u"FRAME-RATE");
                    plNext.codecs = attr.value(u"CODECS");
                    plNext.hdcp = attr.value(u"HDCP-LEVEL");
                    plNext.video_range = attr.value(u"VIDEO-RANGE");
                    plNext.video = attr.value(u"VIDEO");
                    plNext.audio = attr.value(u"AUDIO");
                    plNext.subtitles = attr.value(u"SUBTITLES");
                    plNext.closed_captions = attr.value(u"CLOSED-CAPTIONS");
                    break;
                }
                case Tag::MEDIA: {
                    // #EXT-X-MEDIA:<attribute-list>
                    const TagAttributes attr(tagParams);
                    AltPlayList pl;
                    pl.name = attr.value(u"NAME");
                    pl.type = attr.value(u"TYPE");
                    pl.group_id = attr.value(u"GROUP-ID");
                    pl.stable_rendition_id = attr.value(u"STABLE-RENDITION-ID");
                    pl.language = attr.value(u"LANGUAGE");
                    pl.assoc_language = attr.value(u"ASSOC-LANGUAGE");
                    pl.in_stream_id = attr.value(u"INSTREAM-ID");
                    pl.characteristics = attr.value(u"CHARACTERISTICS");
                    pl.channels = attr.value(u"CHANNELS");
                    pl.is_default = attr.value(u"DEFAULT").similar(u"YES");
                    pl.auto_select = attr.value(u"AUTOSELECT").similar(u"YES");
                    pl.forced = attr.value(u"FORCED").similar(u"YES");
                    const UString uri(attr.value(u"URI"));
                    if (!uri.empty()) {
                        buildURL(pl, uri);
                        if (!pl.file_path.ends_with(u".m3u8", CASE_INSENSITIVE)) {
                            report.debug(u"unexpected playlist file extension in reference URI: %s", uri);
                        }
                    }
                    _alt_playlists.push_back(pl);
                    break;
                }
                case Tag::BYTERANGE:
                case Tag::DISCONTINUITY:
                case Tag::KEY:
                case Tag::MAP:
                case Tag::PROGRAM_DATE_TIME:
                case Tag::DATERANGE:
                case Tag::SKIP:
                case Tag::PRELOAD_HINT:
                case Tag::RENDITION_REPORT:
                case Tag::DISCONTINUITY_SEQUENCE:
                case Tag::I_FRAMES_ONLY:
                case Tag::PART_INF:
                case Tag::SERVER_CONTROL:
                case Tag::I_FRAME_STREAM_INF:
                case Tag::SESSION_DATA:
                case Tag::SESSION_KEY:
                case Tag::CONTENT_STEERING:
                case Tag::INDEPENDENT_SEGMENTS:
                case Tag::START:
                case Tag::DEFINE:
                case Tag::PART:
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
    if (!line.starts_with(u"#EXT", strict ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
        return false;
    }

    // This is a tag line. Locate the tag name (letters, digits and dash).
    size_t pos = 1;
    while (pos < line.size() && (IsAlpha(line[pos]) || IsDigit(line[pos]) || line[pos] == u'-')) {
        ++pos;
    }

    // Identify the tag. Report unknown tag but do not invalidate the playlist.
    if (!TagNames().getValue(tag, line.substr(1, pos - 1), strict)) {
        report.log(strict ? Severity::Error : Severity::Debug, u"unsupported HLS tag: %s", line.substr(1, pos - 1));
        return false;
    }

    // Set playlist type based on tags which are unique to a playlist type.
    const TagFlags flags = TagProperties(tag);
    if ((flags & (TagFlags::MASTER | TagFlags::MEDIA)) == TagFlags::MASTER) {
        // This is a master-only tag.
        setType(PlayListType::MASTER, report);
    }
    else if ((flags & (TagFlags::MASTER | TagFlags::MEDIA)) == TagFlags::MEDIA) {
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
            report.error(u"invalid HLS playlist line: %s", line);
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
    if (line.empty() || line.starts_with(u"#")) {
        // Not an URI line.
        return false;
    }

    // Build a full path of the URI and extract the path name (without trailing query or fragment).
    MediaElement me;
    buildURL(me, line);
    const UString name(me.url.isValid() ? me.url.getPath() : me.file_path);

    // If the URI extension is known, set playlist type.
    if (name.ends_with(u".m3u8", CASE_INSENSITIVE) || name.ends_with(u".m3u", CASE_INSENSITIVE)) {
        // Reference to another playlist, this is a master playlist.
        setType(PlayListType::MASTER, report);
    }
    else if (name.ends_with(u".ts", CASE_INSENSITIVE)) {
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
    if (_auto_save_dir.empty() || _original.empty()) {
        // No need to save
        return true;
    }
    else {
        const UString name(_auto_save_dir + fs::path::preferred_separator + BaseName(_original));
        report.verbose(u"saving playlist to %s", name);
        const bool ok = UString::Save(_loaded_content, name);
        if (!ok) {
            report.warning(u"error saving playlist to %s", name);
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

    if (_is_url) {
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
        str.format(u", %d segments", _segments.size());
    }
    else if (_type == PlayListType::MASTER) {
        str.format(u", %d media playlists", _playlists.size());
        if (!_alt_playlists.empty()) {
            str.format(u", %d alternative rendition playlists", _alt_playlists.size());
        }
    }
    if (_target_duration > cn::seconds::zero()) {
        str.format(u", %s/segment", _target_duration);
    }
    return str;
}


//----------------------------------------------------------------------------
// Save the playlist to a text file.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::saveFile(const ts::UString &filename, ts::Report &report) const
{
    // Check that we have a valid file name to store the file.
    if (filename.empty() && (_is_url || _original.empty())) {
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
        report.error(u"error saving HLS playlist in %s", name);
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
    text.format(u"#%s\n#%s:%d\n", TagNames().name(Tag::EXTM3U), TagNames().name(Tag::VERSION), _version);

    // Insert application-specific tags before standard tags.
    for (const auto& tag : _extra_tags) {
        text.format(u"%s%s\n", tag.starts_with(u"#") ? u"" : u"#", tag);
    }

    if (isMaster()) {
        // Loop on all alternative rendition playlists.
        for (const auto& pl : _alt_playlists) {
            // The initial fields are required.
            text.format(u"#%s:TYPE=%s,GROUP-ID=\"%s\",NAME=\"%s\"", TagNames().name(Tag::MEDIA), pl.type, pl.group_id, pl.name);
            if (pl.is_default) {
                text.append(u",DEFAULT=YES");
            }
            if (pl.auto_select) {
                text.append(u",AUTOSELECT=YES");
            }
            if (pl.forced) {
                text.append(u",FORCED=YES");
            }
            if (!pl.language.empty()) {
                text.format(u",LANGUAGE=\"%s\"", pl.language);
            }
            if (!pl.assoc_language.empty()) {
                text.format(u",ASSOC-LANGUAGE=\"%s\"", pl.assoc_language);
            }
            if (!pl.stable_rendition_id.empty()) {
                text.format(u",STABLE-RENDITION-ID=\"%s\"", pl.stable_rendition_id);
            }
            if (!pl.in_stream_id.empty()) {
                text.format(u",INSTREAM-ID=\"%s\"", pl.in_stream_id);
            }
            if (!pl.characteristics.empty()) {
                text.format(u",CHARACTERISTICS=\"%s\"", pl.characteristics);
            }
            if (!pl.channels.empty()) {
                text.format(u",CHANNELS=\"%s\"", pl.channels);
            }
            if (!pl.relative_uri.empty()) {
                text.format(u",URI=\"%s\"", pl.relative_uri);
            }
            // Close the #EXT-X-MEDIA line.
            text.append(u'\n');
        }
        // Loop on all media playlists.
        for (const auto& pl : _playlists) {
            if (!pl.relative_uri.empty()) {
                // The #EXT-X-STREAM-INF line must exactly preceed the URI line.
                // Take care about string parameters: some are documented as quoted-string and
                // some as enumerated-string. The former shall be quoted, the latter shall not.
                text.format(u"#%s:BANDWIDTH=%d", TagNames().name(Tag::STREAM_INF), pl.bandwidth.toInt());
                if (pl.average_bandwidth > 0) {
                    text.format(u",AVERAGE-BANDWIDTH=%d", pl.average_bandwidth.toInt());
                }
                if (pl.frame_rate > 0) {
                    text.format(u",FRAME-RATE=%d.%03d", pl.frame_rate / 1000, pl.frame_rate % 1000);
                }
                if (pl.width > 0 && pl.height > 0) {
                    text.format(u",RESOLUTION=%dx%d", pl.width, pl.height);
                }
                if (!pl.codecs.empty()) {
                    text.format(u",CODECS=\"%s\"", pl.codecs);
                }
                if (!pl.hdcp.empty()) {
                    text.format(u",HDCP-LEVEL=%s", pl.hdcp);
                }
                if (!pl.video_range.empty()) {
                    text.format(u",VIDEO-RANGE=%s", pl.video_range);
                }
                if (!pl.video.empty()) {
                    text.format(u",VIDEO=\"%s\"", pl.video);
                }
                if (!pl.audio.empty()) {
                    text.format(u",AUDIO=\"%s\"", pl.audio);
                }
                if (!pl.subtitles.empty()) {
                    text.format(u",SUBTITLES=\"%s\"", pl.subtitles);
                }
                if (!pl.closed_captions.empty()) {
                    if (pl.closed_captions.similar(u"NONE")) {
                        // enumerated-string
                        text.append(u",CLOSED-CAPTIONS=NONE");
                    }
                    else {
                        // quoted-string
                        text.format(u",CLOSED-CAPTIONS=\"%s\"", pl.closed_captions);
                    }
                }
                // Close the #EXT-X-STREAM-INF line.
                text.append(u'\n');
                // The URI line must come right after #EXT-X-STREAM-INF.
                text.format(u"%s\n", pl.relative_uri);
            }
        }
    }
    else if (isMedia()) {
        // Global tags.
        text.format(u"#%s:%d\n", TagNames().name(Tag::TARGETDURATION), _target_duration.count());
        text.format(u"#%s:%d\n", TagNames().name(Tag::MEDIA_SEQUENCE), _media_sequence);
        if (_type == PlayListType::VOD) {
            text.format(u"#%s:VOD\n", TagNames().name(Tag::PLAYLIST_TYPE));
        }
        else if (_type == PlayListType::EVENT) {
            text.format(u"#%s:EVENT\n", TagNames().name(Tag::PLAYLIST_TYPE));
        }

        // Loop on all media segments.
        for (const auto& seg : _segments) {
            if (!seg.relative_uri.empty()) {
                text.format(u"#%s:%d.%03d,%s\n", TagNames().name(Tag::EXTINF), seg.duration.count() / 1000, seg.duration.count() % 1000, seg.title);
                if (seg.bitrate > 1024) {
                    text.format(u"#%s:%d\n", TagNames().name(Tag::BITRATE), (seg.bitrate / 1024).toInt());
                }
                if (seg.gap) {
                    text.format(u"#%s\n", TagNames().name(Tag::GAP));
                }
                text.format(u"%s\n", seg.relative_uri);
            }
        }

        // Mark end of list when necessary.
        if (_end_list) {
            text.format(u"#%s\n", TagNames().name(Tag::ENDLIST));
        }
    }
    else {
        report.error(u"unknown HLS playlist type (master or media playlist)");
        text.clear();
    }

    return text;
}
