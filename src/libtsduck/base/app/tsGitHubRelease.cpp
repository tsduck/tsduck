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
//
// GitHub REST API v3 host: https://api.github.com
// Documentation: https://developer.github.com/v3
//
// To get the full descriptions of all releases:
//   https://api.github.com/repos/:owner/:repo/releases
//
// To get the full description of the latest release:
//   https://api.github.com/repos/:owner/:repo/releases/latest
//
// To get the full description of a release by tag:
//   https://api.github.com/repos/:owner/:repo/releases/tags/:tag
//
// In case of error, the returned JSON structure looks like:
//   { "message": "Not Found", "documentation_url": "https://developer.github.com/v3" }
//
//----------------------------------------------------------------------------

#include "tsGitHubRelease.h"
#include "tsWebRequest.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsjsonValue.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::GitHubRelease::GitHubRelease() :
    _isValid(false),
    _owner(),
    _repository(),
    _root()
{
}


//----------------------------------------------------------------------------
// Constructor with download of the version information from GitHub.
//----------------------------------------------------------------------------

ts::GitHubRelease::GitHubRelease(const UString& owner, const UString& repository, const UString& tag, Report& report) :
    GitHubRelease()
{
    downloadInfo(owner, repository, tag, report);
}


//----------------------------------------------------------------------------
// Basic validation of the root JSON.
//----------------------------------------------------------------------------

bool ts::GitHubRelease::validate(Report& report)
{
    // We simply check the presence of a few fields in the object.
    _isValid = !_root.isNull() &&
        _root->value(u"name").isString() && !_root->value(u"name").toString().empty() &&
        _root->value(u"tag_name").isString() && !_root->value(u"tag_name").toString().empty();

    if (!_isValid) {
        InvalidResponse(_root, report);
    }

    return _isValid;
}


//----------------------------------------------------------------------------
// Report an invalid response from GitHub.
//----------------------------------------------------------------------------

void ts::GitHubRelease::InvalidResponse(const json::ValuePtr& response, Report& report)
{
    report.error(u"invalid response from GitHub, use --debug for more details");
    if (report.debug()) {
        report.debug(u"GitHub response: %s", {response->printed(2, report)});
    }
}


//----------------------------------------------------------------------------
// Fetch a API request for a repository. Return a JSON structure.
//----------------------------------------------------------------------------

bool ts::GitHubRelease::CallGitHub(json::ValuePtr& response, json::Type expectedType, const UString& owner, const UString& repository, const UString& request, Report& report)
{
    // Get the GitHub URL. The default value is hardcoded but an alternate value
    // can be specified in environment variable TSDUCK_GITHUB_URL.
    UString github(GetEnvironment(u"TSDUCK_GITHUB_URL", u"https://api.github.com/"));

    // Remove all leading slashes.
    while (!github.empty() && github.back() == u'/') {
        github.pop_back();
    }

    // Build the request.
    WebRequest req(report);
    const UString url(github + u"/repos/" + owner + u"/" + repository + request);

    // Look for an optional GitHub authorization token.
    UString token(GetEnvironment(u"TSDUCK_GITHUB_API_TOKEN"));
    if (token.empty()) {
        token = GetEnvironment(u"GITHUB_API_TOKEN");
    }

    // On macOS, use the HomeBrew token if no other is found.
#if defined(TS_MAC)
    if (token.empty()) {
        token = GetEnvironment(u"HOMEBREW_GITHUB_API_TOKEN");
    }
#endif

    // If a GitHub API token is found, add it in the request headers.
    if (!token.empty()) {
        req.setRequestHeader(u"Authorization", u"token " + token);
    }

    // Send the request, fetch the response, analyze the JSON.
    UString text;
    if (!req.downloadTextContent(url, text) || !json::Parse(response, text, report)) {
        return false;
    }
    assert(!response.isNull());

    // If the response is an object containing a "message" field, this is an error.
    const UString message(response->value(u"message").toString());
    if (!message.empty()) {
        report.error(u"GitHub error: %s", {message});
        return false;
    }

    // Check response type
    if (response->type() != expectedType) {
        InvalidResponse(response, report);
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Download the version information from GitHub.
//----------------------------------------------------------------------------

bool ts::GitHubRelease::downloadInfo(const UString& owner, const UString& repository, const UString& tag, Report& report)
{
    _root.clear();
    _owner = owner;
    _repository = repository;
    _isValid = false;

    // Send the request to GitHub. We expect a JSON object.
    return CallGitHub(_root, json::Type::Object, owner, repository, tag.empty() ? u"/releases/latest" : u"/releases/tags/" + tag, report) && validate(report);
}


//----------------------------------------------------------------------------
// Download information from GitHub for all versions of a product.
//----------------------------------------------------------------------------

bool ts::GitHubRelease::GetAllVersions(GitHubReleaseVector& versions, const UString& owner, const UString& repository, Report& report)
{
    versions.clear();

    // Send the request to GitHub. We expect an array of release objects.
    json::ValuePtr response;
    if (!CallGitHub(response, json::Type::Array, owner, repository, u"/releases", report)) {
        return false;
    }

    // Extract all elements of the array and build release objects.
    // We treat elements in reverse order for performance reasons.
    while (response->size() > 0) {
        const GitHubReleasePtr vers(new GitHubRelease);
        vers->_root = response->extractAt(response->size() - 1);
        if (vers->validate(report)) {
            versions.push_back(vers);
        }
    }

    // Now restore the original order from the request.
    std::reverse(versions.begin(), versions.end());
    return true;
}


//----------------------------------------------------------------------------
// Description of an "asset" of the release (typically a binary installer).
//----------------------------------------------------------------------------

ts::GitHubRelease::Asset::Asset() :
    name(),
    size(0),
    mimeType(),
    url(),
    downloadCount(0)
{
}

// Build an asset description from a JSON object.
void ts::GitHubRelease::BuildAsset(Asset& asset, const json::Value& value)
{
    if (!value.isNull()) {

        // Direct values from JSON.
        asset.name = value.value(u"name").toString();
        asset.size = value.value(u"size").toInteger();
        asset.mimeType = value.value(u"content_type").toString();
        asset.url = value.value(u"browser_download_url").toString();
        asset.downloadCount = int(value.value(u"download_count").toInteger());

        // If name is empty, take base name of URL.
        if (asset.name.empty() && !asset.url.empty()) {
            asset.name = asset.url;
            // Remove trailing slashes.
            while (!asset.name.empty() && asset.name.back() == u'/') {
                asset.name.pop_back();
            }
            // Remove everything before the last slash.
            const size_t slash = asset.name.rfind(u'/');
            if (slash != NPOS) {
                asset.name.erase(0, slash + 1);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Convert a GitHub date/time string into a Time object.
//----------------------------------------------------------------------------

ts::Time ts::GitHubRelease::StringToTime(const UString& str)
{
    // See https://developer.github.com/v3/#timezones
    // "These timestamps look something like 2014-02-27T15:05:06+01:00."

    // Replace all non-digit characters by spaces.
    UString s(str);
    for (size_t i = 0; i < s.size(); ++i) {
        if (!IsDigit(s[i])) {
            s[i] = u' ';
        }
    }

    // Decode up to 6 fields.
    Time::Fields f;
    s.scan(u"%d %d %d %d %d %d", {&f.year, &f.month, &f.day, &f.hour, &f.minute, &f.second});
    try {
        return Time(f);
    }
    catch (...) {
        return Time::Epoch;
    }
}


//----------------------------------------------------------------------------
// Get information about the release.
//----------------------------------------------------------------------------

ts::UString ts::GitHubRelease::tag() const
{
    return _isValid ? _root->value(u"tag_name").toString() : UString();
}

ts::UString ts::GitHubRelease::version() const
{
    UString s(tag());
    while (!s.empty() && !IsDigit(s.front())) {
        s.erase(0, 1);
    }
    return s;
}

ts::UString ts::GitHubRelease::versionName() const
{
    return _isValid ? _root->value(u"name").toString() : UString();
}

ts::Time ts::GitHubRelease::publishDate() const
{
    return _isValid ? StringToTime(_root->value(u"published_at").toString()) : Time::Epoch;
}

ts::UString ts::GitHubRelease::sourceTarURL() const
{
    return _isValid ? _root->value(u"tarball_url").toString() : UString();
}

ts::UString ts::GitHubRelease::sourceZipURL() const
{
    return _isValid ? _root->value(u"zipball_url").toString() : UString();
}

bool ts::GitHubRelease::useSourceZip() const
{
#if defined(TS_UNIX)
    // On UNIX, prefer tarballs. Use zip only is tarball not present.
    return sourceTarURL().empty();
#else
    // On Windows (or other systems), prefer zip files when present.
    return !sourceZipURL().empty();
#endif

}

ts::UString ts::GitHubRelease::sourceURL() const
{
    return useSourceZip() ? sourceZipURL() : sourceTarURL();
}

ts::UString ts::GitHubRelease::sourceFileName() const
{
    return (_repository + u"-" + version() + u"-src") + (useSourceZip() ? u".zip" : u".tgz");
}

int ts::GitHubRelease::assetDownloadCount() const
{
    int count = 0;
    if (_isValid) {
        // Get the array of assets.
        const json::Value& arr(_root->value(u"assets"));
        for (size_t i = 0; i < arr.size(); ++i) {
            count += int(arr.at(i).value(u"download_count").toInteger());
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Get the list of all assets for the release.
//----------------------------------------------------------------------------

void ts::GitHubRelease::getAssets(AssetList& assets) const
{
    assets.clear();

    if (_isValid) {
        // Get the array of assets.
        const json::Value& arr(_root->value(u"assets"));
        for (size_t i = 0; i < arr.size(); ++i) {
            // Each element of the array represents an asset.
            Asset a;
            BuildAsset(a, arr.at(i));
            if (!a.name.empty()) {
                assets.push_back(a);
            }
        }

        // Sort assets by name.
        assets.sort([](const Asset& a1, const Asset& a2) { return a1.name < a2.name; });
    }
}


//----------------------------------------------------------------------------
// Check if a binary file is appropriate for the current platform.
//----------------------------------------------------------------------------

bool ts::GitHubRelease::IsPlatformAsset(const UString& fileName)
{
    const SysInfo& sys(*SysInfo::Instance());

    if (sys.isWindows() && sys.isIntel64()) {
        return fileName.contain(u"win64", ts::CASE_INSENSITIVE) && fileName.endWith(u".exe", ts::CASE_INSENSITIVE);
    }
    else if (sys.isWindows() && sys.isIntel32()) {
        return fileName.contain(u"win32", ts::CASE_INSENSITIVE) && fileName.endWith(u".exe", ts::CASE_INSENSITIVE);
    }
    else if (sys.isMacOS() && sys.isIntel64()) {
        return fileName.endWith(u".dmg");
    }
    else if (sys.isFedora() && sys.isIntel64()) {
        return fileName.contain(u".fc") && (fileName.endWith(u".x86_64.rpm") || fileName.endWith(u".noarch.rpm"));
    }
    else if (sys.isFedora() && sys.isIntel32()) {
        return fileName.contain(u".fc") && (fileName.endWith(u".i386.rpm") || fileName.endWith(u".i686.rpm") || fileName.endWith(u".noarch.rpm"));
    }
    else if (sys.isRedHat() && sys.isIntel64()) {
        return fileName.contain(u".el") && (fileName.endWith(u".x86_64.rpm") || fileName.endWith(u".noarch.rpm"));
    }
    else if (sys.isRedHat() && sys.isIntel32()) {
        return fileName.contain(u".el") && (fileName.endWith(u".i386.rpm") || fileName.endWith(u".i686.rpm") || fileName.endWith(u".noarch.rpm"));
    }
    else if (sys.isUbuntu() && sys.isIntel64()) {
        return fileName.contain(u".ubuntu") && (fileName.endWith(u"_amd64.deb") || fileName.endWith(u"_all.deb"));
    }
    else if (sys.isUbuntu() && sys.isIntel32()) {
        return fileName.contain(u".ubuntu") && (fileName.endWith(u"_i386.deb") || fileName.endWith(u"_i686.deb") || fileName.endWith(u"_all.deb"));
    }
    else if (sys.isDebian() && sys.isIntel64()) {
        return fileName.contain(u".debian") && (fileName.endWith(u"_amd64.deb") || fileName.endWith(u"_all.deb"));
    }
    else if (sys.isDebian() && sys.isIntel32()) {
        return fileName.contain(u".debian") && (fileName.endWith(u"_i386.deb") || fileName.endWith(u"_i686.deb") || fileName.endWith(u"_all.deb"));
    }
    else if (sys.isRaspbian() && sys.isIntel64()) {
        return fileName.contain(u".raspbian") && (fileName.endWith(u"_amd64.deb") || fileName.endWith(u"_all.deb"));
    }
    else if (sys.isRaspbian() && sys.isIntel32()) {
        return fileName.contain(u".raspbian") && (fileName.endWith(u"_i386.deb") || fileName.endWith(u"_i686.deb") || fileName.endWith(u"_all.deb"));
    }
    else {
        return false;  // unknown platform.
    }
}


//----------------------------------------------------------------------------
// Get the list of assets for the current platform.
//----------------------------------------------------------------------------

void ts::GitHubRelease::getPlatformAssets(AssetList& assets) const
{
    // First, get all assets.
    getAssets(assets);

    // Then, remove assets which are not suitable for the local platform.
    for (auto it = assets.begin(); it != assets.end(); ) {
        if (IsPlatformAsset(it->name)) {
            ++it;
        }
        else {
            it = assets.erase(it);
        }
    }
}
