//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsVersion.h"
TSDUCK_SOURCE;

// We also include all libtsduck header files. This is done on purpose to
// make sure that all inlined functions are compiled at least once. Otherwise,
// on Windows, the libtsduck DLL will no contain the referenced code.
#include "tsduck.h"

// Exported version of the TSDuck library.
const int tsduckLibraryVersionMajor = TS_VERSION_MAJOR;
const int tsduckLibraryVersionMinor = TS_VERSION_MINOR;
const int tsduckLibraryVersionCommit = TS_COMMIT;

// Enumeration description of ts::VersionFormat.
const ts::Enumeration ts::VersionFormatEnum({
    {u"short",   ts::VERSION_SHORT},
    {u"long",    ts::VERSION_LONG},
    {u"date",    ts::VERSION_DATE},
    {u"nsis",    ts::VERSION_NSIS},
    {u"dektec",  ts::VERSION_DEKTEC},
});



//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

ts::UString ts::GetVersion(VersionFormat format, const UString& applicationName)
{
    switch (format) {
        case VERSION_SHORT: {
            // The simplest version.
            return ts::UString::Format(u"%d.%d-%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT});
        }
        case VERSION_LONG: {
            // The long explanatory version.
            return (applicationName.empty() ? UString() : applicationName + u": ") +
                    ts::UString::Format(u"TSDuck - The MPEG Transport Stream Toolkit - version %d.%d-%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT});
        }
        case VERSION_DATE: {
            // The build date.
            return UString::Format(u"%s - %s", {__DATE__, __TIME__});
        }
        case VERSION_NSIS: {
            // A definition directive for NSIS.
            // The name tsduckVersion contains the visible version.
            // The name tsduckVersionInfi contains a Window normalized version number X.X.X.X.
            return UString::Format(u"!define tsduckVersion \"%d.%d-%d\"\n!define tsduckVersionInfo \"%d.%d.%d.0\"",
                                   {TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT, TS_VERSION_MAJOR, TS_VERSION_MINOR, TS_COMMIT});
        }
        case VERSION_DEKTEC: {
            // The version of Dektec components.
            return GetDektecVersions();
        }
        default: {
            // Undefined type, return an empty string.
            return UString();
        }
    }
}
