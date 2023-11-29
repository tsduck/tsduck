//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsURL.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Set URL from a string.
//----------------------------------------------------------------------------

void ts::URL::setURL(const UString& path)
{
    clear();
    parse(path);

    // Default to a file URL.
    if (_scheme.empty()) {
        _scheme = u"file";
        if (!_path.startWith(u"/")) {
            // Make it an absolute path.
            UString dir(fs::current_path(&ErrCodeReport()));
#if defined(TS_WINDOWS)
            dir.substitute(u'\\', u'/');
            dir.insert(0, u"/");
#endif
            // A directory must end with a slash in a URL.
            if (!dir.endWith(u"/") && !_path.empty()) {
                dir.append(u"/");
            }
            _path.insert(0, dir);
        }
    }

    // Cleanup /../ and /./
    cleanupPath();
}


//----------------------------------------------------------------------------
// Set URL from a string and a base.
//----------------------------------------------------------------------------

void ts::URL::setURL(const UString& path, const UString& base)
{
    clear();
    parse(path);
    applyBase(URL(base));
}

void ts::URL::setURL(const UString& path, const URL& base)
{
    clear();
    parse(path);
    applyBase(base);
}


//----------------------------------------------------------------------------
// Parse a URL, leave unspecified fields unmodified.
//----------------------------------------------------------------------------

void ts::URL::parse(const UString& path)
{
    const size_t colon = SchemeLength(path);
    size_t current = 0;

    // Parse scheme://host/ if there is one.
    if (colon > 0) {

        _scheme = path.substr(0, colon);
        _username.clear();
        _password.clear();
        _host.clear();
        _port = 0;

        // There must be "://" at index 'colon'.
        current = colon + 3;
        assert(current <= path.size());

        // Check if a host is present.
        // When there is a third slash, this is the beginning of the path and there is no host.
        bool has_host = current < path.size() && path[current] != u'/';

        // But, on Windows, a file: URL may have only two slashes followed by a device letter.
#if defined(TS_WINDOWS)
        if (has_host && _scheme == u"file" && current + 1 < path.size() && IsAlpha(path[current]) && (path[current+1] == u':' || path[current+1] == u'|')) {
            // URL is "file://C:/..." or "file://C|/..."
            has_host = false;
            // Move backward to make sure the path starts with a "/".
            --current;
        }
#endif

        // Parse [user[:password]@]host[:port].
        if (has_host) {
            size_t start = current;             // start of host part
            current = path.find(u'/', start);   // start of path part
            size_t at = path.find(u'@', start);
            if (at < current) {
                // There is a username part.
                const size_t sep = path.find(u':', start);
                if (sep < at) {
                    _username = path.substr(start, sep - start);
                    _password = path.substr(sep + 1, at - sep - 1);
                }
                else {
                    _username = path.substr(start, at - start);
                    _password.clear();
                }
                start = at + 1;
            }
            const size_t sep = path.find(u':', start);
            if (sep < current) {
                // There is port.
                _host = path.substr(start, sep - start);
                path.substr(sep + 1, current - sep - 1).toInteger(_port);
            }
            else {
                _host = path.substr(start, current - start);
                _port = 0;
            }
        }
    }

    // Parse path[?query][#fragment]
    if (current < path.size()) {
        const size_t qmark = path.find(u'?', current);
        const size_t hash = path.find(u'#', current);
        _path = path.substr(current, std::min(qmark, hash) - current);
        if (qmark < path.size()) {
            _query = path.substr(qmark + 1, hash < qmark ? NPOS : hash - qmark - 1);
        }
        if (hash < path.size() && (qmark > path.size() || hash > qmark)) {
            _fragment = path.substr(hash + 1);
        }
    }

    // On Windows, normalize file URL.
#if defined(TS_WINDOWS)
    if (_scheme.empty() || _scheme == u"file") {
        _path.substitute(u'\\', u'/');
        if (_path.size() >= 2 && IsAlpha(_path[0]) && (_path[1] == u':' || _path[1] == u'|')) {
            // This is an absolute Windows path.
            _path.insert(0, u"/");
        }
        if (_path.size() >= 3 && _path[0] == u'/' && IsAlpha(_path[1]) && _path[2] == u'|') {
            _path[2] = u':';
        }
    }
#endif
}


//----------------------------------------------------------------------------
// Apply missing base components from a base URL.
//----------------------------------------------------------------------------

void ts::URL::applyBase(const URL& base)
{
    // If there is no scheme, this was a relative URL.
    if (_scheme.empty()) {

        // The scheme and host part is fully inherited from the base URL.
        _scheme = base._scheme;
        _username = base._username;
        _password = base._password;
        _host = base._host;
        _port = base._port;

        // The path is built based on the base URL.
        // If the path already starts with a slash, it is absolute on the host.
        if (_path.empty()) {
            // Completely missing path, use base
            _path = base._path;
        }
        else if (!_path.startWith(u"/")) {
            // Relative path, append after base.
            if (base._path.endWith(u"/")) {
                // Base path is a directory, use it.
                _path.insert(0, base._path);
            }
            else {
                // Base path is a file/object, extract directory part.
                const size_t last_slash = base._path.rfind(u'/');
                if (last_slash >= base._path.size()) {
                    // No slash in base path, assume root.
                    _path.insert(0, 1, u'/');
                }
                else {
                    // Insert directory part (including slash) of the base path.
                    _path.insert(0, base._path, 0, last_slash + 1);
                }
            }
        }
    }

    // Cleanup /../ and /./
    cleanupPath();
}


//----------------------------------------------------------------------------
// Cleanup /../ and /./ from path.
//----------------------------------------------------------------------------

void ts::URL::cleanupPath()
{
    const bool end_slash = _path.endWith(u"/");

    // Use CleanupFilePath() which works on OS separators.
#if defined(TS_WINDOWS)
    _path.substitute(u'/', u'\\');
#endif
    _path = CleanupFilePath(_path);
#if defined(TS_WINDOWS)
    _path.substitute(u'\\', u'/');
#endif

    // Preserve final slash (meaningful in URL) if removed by CleanupFilePath().
    if (end_slash && !_path.endWith(u"/")) {
        _path.append(u"/");
    }
}


//----------------------------------------------------------------------------
// Clear the content of the URL (becomes invalid).
//----------------------------------------------------------------------------

void ts::URL::clear()
{
    _scheme.clear();
    _username.clear();
    _password.clear();
    _host.clear();
    _port = 0;
    _path.clear();
    _query.clear();
    _fragment.clear();
}


//----------------------------------------------------------------------------
// Convert to a string object.
//----------------------------------------------------------------------------

ts::UString ts::URL::toString(bool useWinInet) const
{
    UString url;
    if (!_scheme.empty()) {
        url = _scheme;
        url.append(u"://");
#if defined(TS_WINDOWS)
        if (useWinInet && _scheme == u"file" && _username.empty() && _password.empty() && _host.empty() && _port == 0) {
            // We need the final string 'file://C:/dir/file' to contain 2 slashes instead of the standard 3.
            url.pop_back();
        }
#endif
        if (!_username.empty() || !_password.empty()) {
            url.append(_username);
            if (!_password.empty()) {
                url.append(u":");
                url.append(_password);
            }
            url.append(u"@");
        }
        url.append(_host);
        if (_port != 0) {
            url.append(UString::Format(u":%d", {_port}));
        }
        if (!_path.startWith(u"/")) {
            // Enforce a slash between host and path.
            url.append(u"/");
        }
        url.append(_path);
        if (!_query.empty()) {
            url.append(u"?");
            url.append(_query);
        }
        if (!_fragment.empty()) {
            url.append(u"#");
            url.append(_fragment);
        }
    }
    return url;
}


//----------------------------------------------------------------------------
// Extract a relative URL of this object, from a base URL.
//----------------------------------------------------------------------------

ts::UString ts::URL::toRelative(const UString& base, bool useWinInet) const
{
    return toRelative(URL(base), useWinInet);
}

ts::UString ts::URL::toRelative(const URL& base, bool useWinInet) const
{
    // If the base is not on the same server, there is no relative path, return the full URL.
    if (!sameServer(base)) {
        return toString(useWinInet);
    }

    // Get directory part of base path.
    size_t start = 0;
    const size_t last_slash = base._path.rfind(u'/');
    if (last_slash < base._path.size() && _path.startWith(base._path.substr(0, last_slash + 1))) {
        // The path has the same base, including trailing slash.
        start = last_slash + 1;
    }

    // Build the relative URL.
    UString url(_path, start, _path.size() - start);
    if (!_query.empty()) {
        url.append(u"?");
        url.append(_query);
    }
    if (!_fragment.empty()) {
        url.append(u"#");
        url.append(_fragment);
    }
    return url;
}


//----------------------------------------------------------------------------
// Check if two URL's use the same server (scheme, host, user, etc.)
//----------------------------------------------------------------------------

bool ts::URL::sameServer(const URL& other) const
{
    return _scheme == other._scheme && _username == other._username && _password == other._password && _host == other._host && _port == other._port;
}


//----------------------------------------------------------------------------
// Locate the scheme part of a URL string.
//----------------------------------------------------------------------------

size_t ts::URL::SchemeLength(const UString& path)
{
    // Look for the URL scheme delimiter.
    const size_t colon = path.find(u"://");

    // On Windows, do not consider an absolute path with a device letter
    // as a URL (C://foo/bar is not a URL with scheme C:). We require a
    // scheme name with more than one single letter to avoid that case.

    if (colon < 2 || colon > path.size()) {
        // No scheme found, not a URL.
        return 0;
    }
    else {
        // Check that all preceding characters are alphanumerical.
        for (size_t i = 0; i < colon; ++i) {
            if (!IsAlpha(path[i]) && !IsDigit(path[i])) {
                // Invalid character before scheme, not a URL.
                return 0;
            }
        }
        return colon;
    }
}
