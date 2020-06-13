//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsVersionInfo.h"
#include "tsVersionString.h"
#include "tsSysUtils.h"
#include "tsDektecUtils.h"
#include "tsWebRequest.h"
#include "tsSRTSocket.h"
TSDUCK_SOURCE;

// Exported version of the TSDuck library.
const int tsduckLibraryVersionMajor = TS_VERSION_MAJOR;
const int tsduckLibraryVersionMinor = TS_VERSION_MINOR;
const int tsduckLibraryVersionCommit = TS_COMMIT;

// Enumeration description of ts::VersionFormat.
const ts::Enumeration ts::VersionFormatEnum({
    {u"short",    ts::VERSION_SHORT},
    {u"long",     ts::VERSION_LONG},
    {u"integer",  ts::VERSION_INTEGER},
    {u"date",     ts::VERSION_DATE},
    {u"nsis",     ts::VERSION_NSIS},
    {u"dektec",   ts::VERSION_DEKTEC},
    {u"http",     ts::VERSION_HTTP},
    {u"compiler", ts::VERSION_COMPILER},
    {u"srt",      ts::VERSION_SRT},
    {u"all",      ts::VERSION_ALL},
});


//----------------------------------------------------------------------------
// Build a string representing the compiler version.
//----------------------------------------------------------------------------

namespace {
    ts::UString CompilerVersion()
    {
        ts::UString version;

        // Add compiler type and version.
#if defined(_MSC_FULL_VER)
        version.format(u"MSVC %02d.%02d.%05d", {_MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000});
        #if defined(_MSC_BUILD)
            version.append(ts::UString::Format(u".%02d", {_MSC_BUILD}));
        #endif
#elif defined(_MSC_VER)
        version.format(u"MSVC %02d.%02d", {_MSC_VER / 100, _MSC_VER % 100});
        #if defined(_MSC_BUILD)
            version.append(ts::UString::Format(u".%02d", {_MSC_BUILD}));
        #endif
#elif defined(__clang_version__)
        version.format(u"Clang %s", {__clang_version__});
#elif defined(__llvm__) || defined(__clang__) || defined(__clang_major__)
        version.assign(u"Clang ");
        #if defined(__clang_major__)
            version.append(ts::UString::Format(u"%d", {__clang_major__}));
        #endif
        #if defined(__clang_minor__)
            version.append(ts::UString::Format(u".%d", {__clang_minor__}));
        #endif
        #if defined(__clang_patchlevel__)
            version.append(ts::UString::Format(u".%d", {__clang_patchlevel__}));
        #endif
#elif defined(__GNUC__)
        version.format(u"GCC %d", {__GNUC__});
        #if defined(__GNUC_MINOR__)
            version.append(ts::UString::Format(u".%d", {__GNUC_MINOR__}));
        #endif
        #if defined(__GNUC_PATCHLEVEL__)
            version.append(ts::UString::Format(u".%d", {__GNUC_PATCHLEVEL__}));
        #endif
#else
        version.assign(u"unknown compiler");
#endif

        // Add C++ revision level.
#if defined(__cplusplus)
        version.append(ts::UString::Format(u", C++ std %04d.%02d", {__cplusplus / 100, __cplusplus % 100}));
#endif
        return version;
    }
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

ts::UString ts::GetVersion(VersionFormat format, const UString& applicationName)
{
    switch (format) {
        case VERSION_SHORT: {
            // The simplest version.
            // This undocumented environment variable can be used to force the version (for debug purpose).
            const UString forcedVersion(GetEnvironment(u"TS_FORCED_VERSION"));
            return forcedVersion.empty() ? ts::UString::Format(u"%d.%d-%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT}) : forcedVersion;
        }
        case VERSION_LONG: {
            // The long explanatory version.
            return (applicationName.empty() ? UString() : applicationName + u": ") +
                u"TSDuck - The MPEG Transport Stream Toolkit - version " +
                GetVersion(VERSION_SHORT);
        }
        case VERSION_INTEGER: {
            // An integer value, suitable for comparison.
            return UString::Decimal(TS_VERSION_INTEGER, 0, true, u"");
        }
        case VERSION_DATE: {
            // The build date.
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(date-time)
            return UString::Format(u"%s - %s", {__DATE__, __TIME__});
            TS_POP_WARNING()
        }
        case VERSION_NSIS: {
            // A definition directive for NSIS.
            // The name tsduckVersion contains the visible version.
            // The name tsduckVersionInfi contains a Window normalized version number X.X.X.X.
            return UString::Format(u"!define tsduckVersion \"%s\"\n!define tsduckVersionInfo \"%d.%d.%d.0\"",
                                   {GetVersion(VERSION_SHORT), TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT});
        }
        case VERSION_DEKTEC: {
            // The version of Dektec components.
            return GetDektecVersions();
        }
        case VERSION_HTTP: {
            // The version of the HTTP library.
            return WebRequest::GetLibraryVersion();
        }
        case VERSION_COMPILER: {
            return CompilerVersion();
        }
        case VERSION_SRT: {
            // The version of the SRT library.
            return SRTSocket::GetLibraryVersion();
        }
        case VERSION_ALL: {
            return GetVersion(VERSION_LONG, applicationName) + LINE_FEED +
                u"Built " + GetVersion(VERSION_DATE) + LINE_FEED +
                u"Using " + GetVersion(VERSION_COMPILER) + LINE_FEED +
                u"Web library: " + GetVersion(VERSION_HTTP) + LINE_FEED +
                u"SRT library: " + GetVersion(VERSION_SRT) + LINE_FEED +
                u"Dektec: " + GetVersion(VERSION_DEKTEC);
        }
        default: {
            // Undefined type, return an empty string.
            return UString();
        }
    }
}


//----------------------------------------------------------------------------
// Compare two version strings.
//----------------------------------------------------------------------------

namespace {
    // Convert a version string into a vector of integers.
    void VersionToInts(std::vector<int>& ints, const ts::UString& version)
    {
        // Replace all non-digit characters by spaces.
        ts::UString s(version);
        for (size_t i = 0; i < s.size(); ++i) {
            if (!ts::IsDigit(s[i])) {
                s[i] = u' ';
            }
        }

        // Split into a list of strings.
        ts::UStringList strings;
        s.split(strings, u' ', true, true);

        // Convert strings into integers.
        ints.clear();
        int val = 0;
        for (ts::UStringList::const_iterator it = strings.begin(); it != strings.end(); ++it) {
            if (it->toInteger(val)) {
                ints.push_back(val);
            }
        }
    }
}

int ts::CompareVersions(const UString& v1, const UString& v2)
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
