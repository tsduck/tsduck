//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Build version string
//
//  We also include all libtsduck header files. This is done on purpose to
//  make sure that all inlined functions are compiled at least once. Otherwise,
//  on Windows, the libtsduck DLL will no contain the referenced code.
//
//----------------------------------------------------------------------------

#include "tsVersion.h"
#include "tsduck.h"
TSDUCK_SOURCE;


// Enumeration description of ts::VersionFormat.
const ts::Enumeration ts::VersionFormatEnum
    ({{u"short",   ts::VERSION_SHORT},
      {u"global",  ts::VERSION_GLOBAL},
      {u"long",    ts::VERSION_LONG},
      {u"date",    ts::VERSION_DATE},
      {u"nsis",    ts::VERSION_NSIS},
      {u"dektec",  ts::VERSION_DEKTEC},
      {u"tinyxml", ts::VERSION_TINYXML},
      {u"files",   ts::VERSION_FILES}});

// Month names in compiler-generated dates.
namespace {
    const ts::Enumeration MonthEnum({{u"jan", 1}, {u"feb",  2}, {u"mar",  3}, {u"apr",  4},
                                     {u"may", 5}, {u"jun",  6}, {u"jul",  7}, {u"aug",  8},
                                     {u"sep", 9}, {u"oct", 10}, {u"nov", 11}, {u"dec", 12}});
}


//----------------------------------------------------------------------------
// Add the specification of one or more files to check for revision number.
//----------------------------------------------------------------------------

namespace {
    std::set<ts::UString> RevisionFilesPattern {u"ts*"};
}

void ts::AddRevisionFile(const UString& wildcard)
{
    RevisionFilesPattern.insert(wildcard);
}


//----------------------------------------------------------------------------
// Get the list of all TSDuck executables and shared libraries.
//----------------------------------------------------------------------------

namespace {
    bool GetAllModules(ts::UStringVector& files)
    {
        files.clear();

        // Directory name of executables and libraries.
        const ts::UString dir(ts::DirectoryName(ts::ExecutableFile()) + ts::PathSeparator);

        // Loop on all file name templates. The default is "ts*" only.
        for (std::set<ts::UString>::const_iterator it1 = RevisionFilesPattern.begin(); it1 != RevisionFilesPattern.end(); ++it1) {
            const ts::UString& name(*it1);

            // Get all TSDuck executable file names.
#if defined(TS_WINDOWS)
            // Windows: all .exe files
            ts::ExpandWildcardAndAppend(files, dir + name + TS_EXECUTABLE_SUFFIX);
#else
            // UNIX: all ts* files without extension (no dot).
            ts::UStringVector allfiles;
            ts::ExpandWildcard(allfiles, dir + name);
            for (ts::UStringVector::const_iterator it2 = allfiles.begin(); it2 != allfiles.end(); ++it2) {
                if (it2->find(u'.') == ts::UString::NPOS) {
                    files.push_back(*it2);
                }
            }
#endif

            // Append all TSDuck shared library files.
            ts::ExpandWildcardAndAppend(files, dir + name + TS_SHARED_LIB_SUFFIX);
        }

        // Sort the array.
        std::sort(files.begin(), files.end());
        return true;
    }
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

ts::UString ts::GetVersion(VersionFormat format, const UString& applicationName, const UString& revisionFile)
{
    switch (format) {
        case VERSION_LONG: {
            // The long explanatory version.
            return (applicationName.empty() ? UString() : applicationName + u": ") +
                u"TSDuck - The MPEG Transport Stream Toolkit - version " +
                GetVersion(VERSION_SHORT, applicationName, revisionFile);
        }
        case VERSION_DATE: {
            // The build date.
            return UString::Format(u"%s - %s", {__DATE__, __TIME__});
        }
        case VERSION_NSIS: {
            // A definition directive for NSIS.
            return u"!define tsduckVersion \"" + GetVersion(VERSION_GLOBAL, applicationName, revisionFile) + u'"';
        }
        case VERSION_DEKTEC: {
            // The version of Dektec components.
            return GetDektecVersions();
        }
        case VERSION_TINYXML: {
            // The version of TinyXML-2.
            return UString::Format(u"TinyXML-2 %d.%d.%d", {TIXML2_MAJOR_VERSION, TIXML2_MINOR_VERSION, TIXML2_PATCH_VERSION});
        }
        case VERSION_SHORT: {
            // The simple version with the revision from the current executable and the TSDuck library.
            const int revision = GetRevision(revisionFile, true);
            return revision == 0 ?
                UString::Format(u"%d.%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR}) :
                UString::Format(u"%d.%d-%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR, revision});
        }
        case VERSION_GLOBAL: {
            // Same as short but use the highest revision of all TSDuck files.
            UStringVector files;
            GetAllModules(files);
            int revision = 0;
            for (UStringVector::const_iterator it = files.begin(); it != files.end(); ++it) {
                revision = std::max(revision, GetRevision(*it, false));
            }
            return revision == 0 ?
                UString::Format(u"%d.%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR}) :
                UString::Format(u"%d.%d-%d", {TS_VERSION_MAJOR, TS_VERSION_MINOR, revision});
        }
        case VERSION_FILES: {
            // A list of revisions for all files.
            UStringVector files;
            GetAllModules(files);
            UString list(u"Revision  File\n--------  ");
            list.append(std::max<size_t>(4, LargestSize(files)), u'-');
            for (UStringVector::const_iterator it = files.begin(); it != files.end(); ++it) {
                const int rev = GetRevision(*it, false);
                if (rev != 0) {
                    list.append(UString::Format(u"\n%8d  %s", {rev, *it}));
                }
            }
            return list;
        }
        default: {
            // Undefined type, return an empty string.
            return UString();
        }
    }
}


//-----------------------------------------------------------------------------
// Warning: GetRevision() should be quite simple. However, it triggers
// some code generation bug with Visual C++ and, depending on the
// executable file, the revision value may not be found. This code
// has been modified in some ugly way to avoid the VC++ bug.
// If you modify this code, make sure it works with VC++, in both
// Release and Debug configuration, on all executables.
//-----------------------------------------------------------------------------

namespace {
    // Check is next characters in the binary file match the specified text.
    bool FollowingIs(std::ifstream& file, const char* text)
    {
        char c;
        while (*text != 0 && (file >> c)) {
            if (c == *text) {
                text++;
            }
            else {
                file.unget();
                return false;
            }
        }
        return *text == 0;
    }

    // Get next "word" in the binary file, up to the delimiter.
    void NextWord(std::ifstream& file, ts::UString& word, size_t max_size, char delim)
    {
        word.clear();
        char c;
        while (word.size() < max_size && (file >> c) && (c != delim)) {
            word.append(1, ts::UChar(c));
        }
        word.trim();
    }
}


//-----------------------------------------------------------------------------
// Decode a __DATE__ string and return the corresponding integer value.
// The string is "Mmm dd yyyy", where the names of the months are the
// same as those generated by the asctime function, and the first character
// of dd is a space character if the value is less than 10.
//-----------------------------------------------------------------------------

namespace {
    int DecodeDate(const ts::UString& word)
    {
        int day = 0;
        int year = 0;
        if (word.length() < 11 ||
            !word.substr(4, 2).toInteger(day) ||
            day < 1 || day > 31 ||
            !word.substr(7, 4).toInteger(year) ||
            year < 2000 || year > 2100)
        {
            return 0;
        }

        const int month = MonthEnum.value(word.substr(0, 3), false);
        if (month == ts::Enumeration::UNKNOWN) {
            return 0;
        }

        return (year * 10000) + (month * 100) + day;
    }
}


//----------------------------------------------------------------------------
// Get the TSDuck revision number as integer from a binary file.
//----------------------------------------------------------------------------

int ts::GetRevision(const UString& fileName, bool includeLibrary)
{
    const char delim = TS_BUILD_MARK_SEPARATOR[0];
    int revision = 0;
    UString word;
    int rev;

    // Open the file in binary mode.
    const UString execFile(fileName.empty() ? ExecutableFile() : fileName);
    const std::string execFileUTF8(execFile.toUTF8());
    std::ifstream file(execFileUTF8.c_str(), std::ios::binary);

    // Search for the revision in the specified file.
    file >> std::noskipws;
    char c;
    while (file >> c) {

        // Loop on bytes until we find the first one of the build marker.
        if (c != delim) {
            continue;
        }

        // Check if this is really a marker prefix.
        if (!FollowingIs(file, TS_BUILD_MARK_MARKER TS_BUILD_MARK_SEPARATOR)) {
            continue;
        }

        // We have found the marker prefix, read the next field.
        NextWord(file, word, 32, delim);
        rev = DecodeDate(word);

        // Keep this revision if higher than all preceeding ones.
        if (rev > revision) {
            revision = rev;
        }
    }

    // Then, search into TSDuck shared library if necessary.
    if (includeLibrary) {
        const UString shlibName(DirectoryName(execFile) + PathSeparator + u"tsduck" + TS_SHARED_LIB_SUFFIX);
        revision = std::max(revision, GetRevision(shlibName, false));
    }

    return revision;
}
