//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsVersionInfo.h"
#include "tsVersionString.h"
#include "tsGitHubRelease.h"
#include "tsNullReport.h"
#include "tsErrCodeReport.h"
#include "tsCryptoLibrary.h"
#include "tsVatekUtils.h"
#include "tsSysInfo.h"
#include "tsSysUtils.h"
#include "tsFileUtils.h"
#include "tsDektecUtils.h"
#include "tsWebRequest.h"
#include "tsSRTSocket.h"
#include "tsRIST.h"

// Exported version of the TSDuck library.
// The names of these symbols are constant, their values are not.
const int tsduckLibraryVersionMajor = TS_VERSION_MAJOR;
const int tsduckLibraryVersionMinor = TS_VERSION_MINOR;
const int tsduckLibraryVersionCommit = TS_COMMIT;

// Exported symbols, the names of which depend on the TSDuck version or BitRate implementation.
// When an executable or shared library references these symbols, it is guaranteed that a
// compatible TSDuck library is activated. Otherwise, the dynamic references would have failed.
// Only the symbol names matter, the value is just unimportant.
const int TSDUCK_LIBRARY_VERSION_SYMBOL = TS_VERSION_INTEGER;
const int TSDUCK_LIBRARY_BITRATE_SYMBOL = 0;

// Enumeration description of ts::VersionFormat.
const ts::Enumeration ts::VersionInfo::FormatEnum({
    {u"short",        ts::VersionInfo::Format::SHORT},
    {u"long",         ts::VersionInfo::Format::LONG},
    {u"integer",      ts::VersionInfo::Format::INTEGER},
    {u"date",         ts::VersionInfo::Format::DATE},
    {u"compiler",     ts::VersionInfo::Format::COMPILER},
    {u"system",       ts::VersionInfo::Format::SYSTEM},
    {u"acceleration", ts::VersionInfo::Format::ACCELERATION},
    {u"bitrate",      ts::VersionInfo::Format::BITRATE},
    {u"nsis",         ts::VersionInfo::Format::NSIS},
    {u"crypto",       ts::VersionInfo::Format::CRYPTO},
    {u"dektec",       ts::VersionInfo::Format::DEKTEC},
    {u"http",         ts::VersionInfo::Format::HTTP},
    {u"srt",          ts::VersionInfo::Format::SRT},
    {u"rist",         ts::VersionInfo::Format::RIST},
    {u"vatek",        ts::VersionInfo::Format::VATEK},
    {u"all",          ts::VersionInfo::Format::ALL},
});

// Enumeration of supported features.
const ts::Enumeration ts::VersionInfo::SupportEnum({
#if defined(TS_NO_DTAPI)
        {u"dektec", 0},
#else
        {u"dektec", 1},
#endif
#if defined(TS_NO_HIDES)
        {u"hides", 0},
#else
        {u"hides", 1},
#endif
#if defined(TS_NO_NOCURL) && !defined(TS_WINDOWS)
        {u"http", 0},
#else
        {u"http", 1},
#endif
#if defined(TS_NO_PCSC)
        {u"pcsc", 0},
#else
        {u"pcsc", 1},
#endif
#if defined(TS_NO_RIST)
        {u"rist", 0},
#else
        {u"rist", 1},
#endif
#if defined(TS_NO_SRT)
        {u"srt", 0},
#else
        {u"srt", 1},
#endif
#if defined(TS_NO_VATEK)
        {u"vatek", 0},
#else
        {u"vatek", 1},
#endif
});


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
        _debug.debug(u"last new version check done %s, not done again", {lasttime.UTCToLocal()});
        return;
    }

    // Create the time-stamp file. Delete it first. Create intermediate directory if necessary.
    fs::remove(filename, &ErrCodeReport());
    fs::create_directories(dirname, &ErrCodeReport(_debug, u"error creating directory", dirname));
    if (!UString::Save(UStringVector(), filename)) {
        _debug.error(u"error creating file %s", {filename});
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
        _report.info(u"new TSDuck version %s is available (yours is %s), use 'tsversion --upgrade' or see https://tsduck.io/", {remote, current});
    }

#endif
}


//----------------------------------------------------------------------------
// Build a string representing the compiler version.
//----------------------------------------------------------------------------

ts::UString ts::VersionInfo::GetCompilerVersion()
{
    UString version;

    // Add compiler type and version.
#if defined(_MSC_FULL_VER)
    version.format(u"MSVC %02d.%02d.%05d", _MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000);
    #if defined(_MSC_BUILD)
        version.append(UString::FormatNew(u".%02d", _MSC_BUILD));
    #endif
#elif defined(_MSC_VER)
    version.format(u"MSVC %02d.%02d", _MSC_VER / 100, _MSC_VER % 100);
    #if defined(_MSC_BUILD)
        version.append(UString::FormatNew(u".%02d", _MSC_BUILD));
    #endif
#elif defined(__clang_version__)
    version.format(u"Clang %s", __clang_version__);
#elif defined(__llvm__) || defined(__clang__) || defined(__clang_major__)
    version.assign(u"Clang ");
    #if defined(__clang_major__)
        version.append(UString::FormatNew(u"%d", __clang_major__));
    #endif
    #if defined(__clang_minor__)
        version.append(UString::FormatNew(u".%d", __clang_minor__));
    #endif
    #if defined(__clang_patchlevel__)
        version.append(UString::FormatNew(u".%d", __clang_patchlevel__));
    #endif
#elif defined(__GNUC__)
    version.format(u"GCC %d", __GNUC__);
    #if defined(__GNUC_MINOR__)
        version.append(UString::Format(u".%d", __GNUC_MINOR__));
    #endif
    #if defined(__GNUC_PATCHLEVEL__)
        version.append(UString::Format(u".%d", __GNUC_PATCHLEVEL__));
    #endif
#else
    version.assign(u"unknown compiler");
#endif

    // Add C++ revision level.
#if defined(_MSVC_LANG)
    // With MSVC, the standard macro __cplusplus is stuck at 199711 for obscure reasons.
    // The actual level of language standard is in the system-specific macro _MSVC_LANG.
    version.append(UString::Format(u", C++ std %04d.%02d", _MSVC_LANG / 100, _MSVC_LANG % 100));
#elif defined(__cplusplus)
    version.append(UString::Format(u", C++ std %04d.%02d", __cplusplus / 100, __cplusplus % 100));
#endif

    return version;
}


//----------------------------------------------------------------------------
// Build a string representing the system on which the application runs.
//----------------------------------------------------------------------------

ts::UString ts::VersionInfo::GetSystemVersion()
{
    UString name(SysInfo::Instance().systemName());
    const UString version(SysInfo::Instance().systemVersion());
    if (!version.empty()) {
        name.format(u" (%s)", version);
    }
    name.format(u", on %s, %d-bit, %s-endian, page size: %d bytes",
                SysInfo::Instance().cpuName(),
                TS_ADDRESS_BITS,
                #if defined(TS_LITTLE_ENDIAN)
                    u"little",
                #elif defined(TS_BIG_ENDIAN)
                   u"big",
                #else
                    u"unknown",
                #endif
                SysInfo::Instance().memoryPageSize());
    return name;
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

ts::UString ts::VersionInfo::GetVersion(Format format, const UString& applicationName)
{
    switch (format) {
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
        case Format::COMPILER: {
            return GetCompilerVersion();
        }
        case Format::SYSTEM: {
            return GetSystemVersion();
        }
        case Format::BITRATE: {
            return BitRate().description();
        }
        case Format::NSIS: {
            // A definition directive for NSIS.
            // The name tsduckVersion contains the visible version.
            // The name tsduckVersionInfi contains a Window normalized version number X.X.X.X.
            return UString::Format(u"!define tsduckVersion \"%s\"\n!define tsduckVersionInfo \"%d.%d.%d.0\"",
                                   GetVersion(Format::SHORT), TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT);
        }
        case Format::CRYPTO: {
            // The version of the cryptographiclibrary.
            return ts::GetCryptographicLibraryVersion();
        }
        case Format::DEKTEC: {
            // The version of Dektec components.
            return GetDektecVersions();
        }
        case Format::VATEK: {
            // The version of the Vatek library.
            return ts::GetVatekVersion();
        }
        case Format::HTTP: {
            // The version of the HTTP library.
            return WebRequest::GetLibraryVersion();
        }
        case Format::SRT: {
            // The version of the SRT library.
            return SRTSocket::GetLibraryVersion();
        }
        case Format::RIST: {
            // The version of the RIST library.
            return GetRISTLibraryVersion();
        }
        case Format::ACCELERATION: {
            // Support for accelerated instructions.
            return UString::Format(u"CRC32: %s", UString::YesNo(SysInfo::Instance().crcInstructions()));
        }
        case Format::ALL: {
            return GetVersion(Format::LONG, applicationName) + LINE_FEED +
                u"Built " + GetVersion(Format::DATE) + LINE_FEED +
                u"Using " + GetVersion(Format::COMPILER) + LINE_FEED +
                u"System: " + GetVersion(Format::SYSTEM) + LINE_FEED +
                u"Acceleration: " + GetVersion(Format::ACCELERATION) + LINE_FEED +
                u"Bitrate: " + GetVersion(Format::BITRATE) + LINE_FEED +
                u"Dektec: " + GetVersion(Format::DEKTEC) + LINE_FEED +
                u"VATek: " + GetVersion(Format::VATEK) + LINE_FEED +
                u"Cryptographic library: " + GetVersion(Format::CRYPTO) + LINE_FEED +
                u"Web library: " + GetVersion(Format::HTTP) + LINE_FEED +
                u"SRT library: " + GetVersion(Format::SRT) + LINE_FEED +
                u"RIST library: " + GetVersion(Format::RIST);
        }
        default: {
            // Undefined type, return an empty string.
            return UString();
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
