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

#include "tshlsPlayList.h"
#include "tsWebRequest.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::PlayList::PlayList() :
    _valid(false),
    _version(1),
    _type(UNKNOWN_PLAYLIST),
    _url(),
    _urlBase(),
    _isURL(false)
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
    _url.clear();
    _urlBase.clear();
    _isURL = false;
}


//----------------------------------------------------------------------------
// Build an URL for a media segment or sub playlist.
//----------------------------------------------------------------------------

ts::UString ts::hls::PlayList::buildURL(const ts::UString& uri) const
{
    if (_isURL) {
        // The base URL is really a URL, check if the URI starts with a protocol.
        // Search the position of the first non-alpha character.
        size_t pos = 0;
        while (pos < uri.size() && IsAlpha(uri[pos])) {
            ++pos;
        }
        if (pos > 0 && pos < uri.size() && uri[pos] == u':') {
            // There is a protocol, this is an absolute URL.
            return uri;
        }
    }

    // This is a relative URI.
    return _urlBase + uri;
}


//----------------------------------------------------------------------------
// Load the playlist from a URL.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadURL(const UString& url, bool strict, const WebRequestArgs args, Report& report)
{
    // Erase current content.
    clear();

    // Keep the URL.
    _url = url;
    _isURL = true;
    const size_t slash = url.rfind(u"/");
    if (slash != NPOS) {
        // The URL base up the last "/" (inclusive).
        _urlBase = _url.substr(0, slash + 1);
    }

    // Build a web request to download the playlist.
    WebRequest web(report);
    web.setURL(url);
    web.setArgs(args);

    // Download the content.
    UString text;
    report.debug(u"downloading %s", {url});
    if (!web.downloadTextContent(text)) {
        return false;
    }

    // Check MIME type of the downloaded content.
    const UString mime(web.mimeType());
    report.debug(u"MIME type: %s", {mime});

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u - OR -
    // HTTP Content-Type is application/vnd.apple.mpegurl or audio/mpegurl.
    if (strict &&
        !url.endWith(u".m3u8", CASE_INSENSITIVE) &&
        !url.endWith(u".m3u", CASE_INSENSITIVE) &&
        mime != u"application/vnd.apple.mpegurl" &&
        mime != u"application/mpegurl" &&
        mime != u"audio/mpegurl")
    {
        report.error(u"Invalid MIME type \"%s\" for HLS playlist at %s", {mime, url});
        return false;
    }

    // Load from the text.
    return parse(text, strict, report);
}


//----------------------------------------------------------------------------
// Load the playlist from a text file.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadFile(const UString& filename, bool strict, Report& report)
{
    clear();

    // Keep file name.
    _url = filename;
    _urlBase = DirectoryName(filename) + PathSeparator;
    _isURL = false;

    // Check strict conformance: according to RFC 8216, a playlist must either ends in .m3u8 or .m3u.
    if (strict && !filename.endWith(u".m3u8", CASE_INSENSITIVE) && !filename.endWith(u".m3u", CASE_INSENSITIVE)) {
        report.error(u"Invalid file name extension for HLS playlist in %s", {filename});
        return false;
    }

    // Load the file.
    UStringList lines;
    if (UString::Load(lines, filename)) {
        return parse(lines, strict, report);
    }
    else {
        report.error(u"error loading %s", {filename});
        return false;
    }
}


//----------------------------------------------------------------------------
// Load the playlist from its text content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::loadText(const UString& text, bool strict, Report& report)
{
    clear();
    return parse(text, strict, report);
}


//----------------------------------------------------------------------------
// Load from the text content.
//----------------------------------------------------------------------------

bool ts::hls::PlayList::parse(const UString& text, bool strict, Report& report)
{
    UStringList lines;
    text.toRemoved(CARRIAGE_RETURN).split(lines, LINE_FEED, false, false);
    return parse(lines, strict, report);
}

bool ts::hls::PlayList::parse(const UStringList& lines, bool strict, Report& report)
{
    const CaseSensitivity cs = strict ? CASE_SENSITIVE : CASE_INSENSITIVE;

    // The playlist must always start with #EXTM3U.
    if (lines.empty() || !lines.front().startWith(u"#EXTM3U", cs)) {
        report.error(u"invalid HLS playlist, does not start with #EXTM3U");
        return false;
    }

    // Assume valid playlist, invalidate when necessary.
    _valid = true;

    // Loop on all lines in file.
    for (auto it = lines.begin(); it != lines.end(); ++it) {

        // In non-strict mode, ignore leading and trailing spaces.
        UString line(*it);
        if (!strict) {
            line.trim();
        }

        // A line is one of blank, comment, tag, URI.
        if (line.startWith(u"#EXT", cs)) {
            // This is a tag line. Locate the tag name.
            size_t pos = 1;
            while (pos < line.size() && (IsAlpha(line[pos]) || IsDigit(line[pos]) || line[pos] == u'-')) {
                ++pos;
            }
            const UString tagName(line.substr(1, pos - 1));

            // The tag must be alone of followed by ':'.
            bool ok = pos >= line.size() || line[pos] == u':';
            if (ok && pos < line.size()) {
                ++pos; // skip ':'
            }

            // Decode the tag.
            Tag tag = EXTM3U;
            if (!ok || !TagNames.getValue(tag, tagName, strict)) {
                report.error(u"invalid HLS playlist line: %s", {line});
                _valid = false;
                continue;
            }

            // Keep only the value of the tag in the line.
            line.erase(0, pos);

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

            // Process specific tags.
            switch (tag) {
                case VERSION:
                    if (!line.toInteger(_version) && strict) {
                        report.error(u"invalid HLS playlist version: %s", {line});
                        _valid = false;
                    }
                    break;
                default:
                    break;
            }

            //@@@@@@


        }
        else if (!line.empty() && !line.startWith(u"#")) {
            // This is an URI line.
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

            //@@@@@
        }
    }

    return _valid;
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
