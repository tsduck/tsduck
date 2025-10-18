//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVersionInfo.h"
#include "tsFeatures.h"
#include "tsGitHubRelease.h"
#include "tsNullReport.h"
#include "tsErrCodeReport.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Enumeration descriptions.
//----------------------------------------------------------------------------

// Enumeration description of ts::VersionFormat.
const ts::Names& ts::VersionInfo::FormatEnum()
{
    static Names data(Features::Instance().versionEnum(), {
        {u"all",     Format::ALL},
        {u"short",   Format::SHORT},
        {u"long",    Format::LONG},
        {u"integer", Format::INTEGER},
        {u"date",    Format::DATE},
    });
    return data;
}


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::VersionInfo::VersionInfo(Report& report) :
    _report(report),
    _debug(GetEnvironment(u"TS_DEBUG_NEW_VERSION").empty() ? NULLREP : _report)
{
}

ts::VersionInfo::~VersionInfo()
{
    // Wait for thread termination, if started.
    waitForTermination();

    // Keep this one to avoid clang warning with NOGITHUB=1
    _started = false;
}


//----------------------------------------------------------------------------
// Start a thread which checks the availability of a new TSDuck version.
//----------------------------------------------------------------------------

void ts::VersionInfo::startNewVersionDetection()
{
#if !defined(TS_NO_GITHUB)

    // Do not start more than once.
    // If the environment variable is not empty, do not start the new version check.
    if (_started || !GetEnvironment(u"TSDUCK_NO_VERSION_CHECK").empty()) {
        return;
    }

    // Do not check new version more than once a day.
    // We create an empty more-or-less-hidden empty file at the same place as the
    // TSDuck configuration file. The creation time of this file is the last check time.
    const UString filename(UserConfigurationFileName(u".tsduck.lastcheck", u"tsduck.lastcheck"));
    const UString dirname(DirectoryName(filename));
    const Time lasttime(GetFileModificationTimeUTC(filename));
    const Time curtime(Time::CurrentUTC());
    if (lasttime != Time::Epoch && curtime != Time::Epoch && curtime >= lasttime && curtime < lasttime + cn::days(1)) {
        // Last check was done less than one day ago, don't try again.
        _debug.debug(u"last new version check done %s, not done again", lasttime.UTCToLocal());
        return;
    }

    // Create the timestamp file. Delete it first. Create intermediate directory if necessary.
    fs::remove(filename, &ErrCodeReport());
    fs::create_directories(dirname, &ErrCodeReport(_debug, u"error creating directory", dirname));
    if (!UString::Save(UStringVector(), filename)) {
        _debug.error(u"error creating file %s", filename);
    }

    // Start the thread.
    _started = start();

#endif
}


//----------------------------------------------------------------------------
// Execute in the context of the thread.
//----------------------------------------------------------------------------

void ts::VersionInfo::main()
{
#if !defined(TS_NO_GITHUB)

    // Get new version from GitHub.
    const ts::GitHubRelease rel(u"tsduck", u"tsduck", UString(), _debug);

    // Current and remote versions.
    const ts::UString current(GetVersion());
    const ts::UString remote(rel.version());

    // If not remote version is available
    if (!rel.isValid() || remote.empty()) {
       _debug.debug(u"unable to identify new TSDuck version");
        return;
    }

    // Compare versions.
    if (ts::VersionInfo::CompareVersions(current, remote) < 0) {
        // The current version is older than latest one on GitHub.
        _report.info(u"new TSDuck version %s is available (yours is %s), use 'tsversion --upgrade' or see https://tsduck.io/", remote, current);
    }

#endif
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

ts::UString ts::VersionInfo::GetVersion(Format format, const UString& applicationName)
{
    switch (format) {
        case Format::ALL: {
            // All features. Start with application description.
            UString version = GetVersion(Format::LONG, applicationName) + LINE_FEED + u"Built " + GetVersion(Format::DATE);
            // Add all dynamically added features.
            UStringList features;
            for (const auto& it : Features::Instance().getAllVersions()) {
                features.push_back(it.first + u": " + it.second);
            }
            features.sort();
            if (!features.empty()) {
                version += LINE_FEED;
            }
            return version + UString::Join(features, LINE_FEED);
        }
        case Format::SHORT: {
            // The simplest version.
            // This environment variable can be used to force the version (for debug purpose).
            const UString forcedVersion(GetEnvironment(u"TS_FORCED_VERSION"));
            return forcedVersion.empty() ? ts::UString::Format(u"%d.%d-%d", TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT) : forcedVersion;
        }
        case Format::LONG: {
            // The long explanatory version.
            return (applicationName.empty() ? UString() : applicationName + u": ") +
                u"TSDuck - The MPEG Transport Stream Toolkit - version " +
                GetVersion(Format::SHORT);
        }
        case Format::INTEGER: {
            // An integer value, suitable for comparison.
            return UString::Decimal(TS_VERSION_INTEGER, 0, true, u"");
        }
        case Format::DATE: {
            // The build date.
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(date-time)
            return UString::Format(u"%s - %s", __DATE__, __TIME__);
            TS_POP_WARNING()
        }
        default: {
            // Look for a dynamically added feature.
            return Features::Instance().getVersion(Features::index_t(format));
        }
    }
}


//----------------------------------------------------------------------------
// Convert a version string into a vector of integers.
//----------------------------------------------------------------------------

void ts::VersionInfo::VersionToInts(std::vector<int>& ints, const ts::UString& version)
{
    // Replace all non-digit characters by spaces.
    UString s(version);
    for (size_t i = 0; i < s.size(); ++i) {
        if (!IsDigit(s[i])) {
            s[i] = u' ';
        }
    }

    // Split into a list of strings.
    UStringList strings;
    s.split(strings, u' ', true, true);

    // Convert strings into integers.
    ints.clear();
    int val = 0;
    for (const auto& str : strings) {
        if (str.toInteger(val)) {
            ints.push_back(val);
        }
    }
}


//----------------------------------------------------------------------------
// Compare two version strings.
//----------------------------------------------------------------------------

int ts::VersionInfo::CompareVersions(const UString& v1, const UString& v2)
{
    // Convert versions to arrays of integers.
    std::vector<int> ints1;
    std::vector<int> ints2;
    VersionToInts(ints1, v1);
    VersionToInts(ints2, v2);

    // Compare arrays of integers.
    size_t i1 = 0;
    size_t i2 = 0;
    while (i1 < ints1.size() && i2 < ints2.size()) {
        if (ints1[i1] < ints2[i2]) {
            return -1;
        }
        else if (ints1[i1] > ints2[i2]) {
            return 1;
        }
        else {
            ++i1;
            ++i2;
        }
    }

    if (i1 < ints1.size()) {
        // i1 = i2 + something
        return 1;
    }
    else if (i2 < ints2.size()) {
        // i2 = i1 + something
        return -1;
    }
    else {
        // i1 == i2
        return 0;
    }
}
