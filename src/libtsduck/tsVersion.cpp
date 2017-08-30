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
    ("short",   ts::VERSION_SHORT,
     "global",  ts::VERSION_GLOBAL,
     "long",    ts::VERSION_LONG,
     "date",    ts::VERSION_DATE,
     "nsis",    ts::VERSION_NSIS,
     "dektec",  ts::VERSION_DEKTEC,
     "tinyxml", ts::VERSION_TINYXML,
     "files",   ts::VERSION_FILES,
     TS_NULL);

// Month names in compiler-generated dates.
namespace {
    const ts::Enumeration MonthEnum
        ("jan", 1,
         "feb", 2,
         "mar", 3,
         "apr", 4,
         "may", 5,
         "jun", 6,
         "jul", 7,
         "aug", 8,
         "sep", 9,
         "oct", 10,
         "nov", 11,
         "dec", 12,
         TS_NULL);
}


//----------------------------------------------------------------------------
// Add the specification of one or more files to check for revision number.
//----------------------------------------------------------------------------

namespace {
    std::set<std::string> RevisionFilesPattern {std::string("ts*")};
}

void ts::AddRevisionFile(const std::string& wildcard)
{
    RevisionFilesPattern.insert(wildcard);
}


//----------------------------------------------------------------------------
// Get the list of all TSDuck executables and shared libraries.
//----------------------------------------------------------------------------

namespace {
    bool GetAllModules(ts::StringVector& files)
    {
        files.clear();

        // Directory name of executables and libraries.
        const std::string dir(ts::DirectoryName(ts::ExecutableFile()) + ts::PathSeparator);

        // Loop on all file name templates. The default is "ts*" only.
        for (std::set<std::string>::const_iterator it1 = RevisionFilesPattern.begin(); it1 != RevisionFilesPattern.end(); ++it1) {
            const std::string& name(*it1);

            // Get all TSDuck executable file names.
#if defined(__windows)
            // Windows: all .exe files
            ts::ExpandWildcardAndAppend(files, dir + name + ".exe");
#else
            // UNIX: all ts* files without extension (no dot).
            ts::StringVector allfiles;
            ts::ExpandWildcard(allfiles, dir + name);
            for (ts::StringVector::const_iterator it2 = allfiles.begin(); it2 != allfiles.end(); ++it2) {
                if (it2->find('.') == std::string::npos) {
                    files.push_back(*it2);
                }
            }
#endif

            // Append all TSDuck shared library files.
            ts::ExpandWildcardAndAppend(files, dir + name + ts::SharedLibrary::Extension);
        }

        // Sort the array.
        std::sort(files.begin(), files.end());
        return true;
    }
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

std::string ts::GetVersion(VersionFormat format, const std::string& applicationName, const std::string& revisionFile)
{
    switch (format) {
        case VERSION_LONG: {
            // The long explanatory version.
            return (applicationName.empty() ? "" : applicationName + ": ") +
                "TSDuck - The MPEG Transport Stream Toolkit - version " + GetVersion(VERSION_SHORT, applicationName, revisionFile);
        }
        case VERSION_DATE: {
            // The build date.
            return Format("%s - %s", __DATE__, __TIME__);
        }
        case VERSION_NSIS: {
            // A definition directive for NSIS.
            return "!define tsduckVersion \"" + GetVersion(VERSION_GLOBAL, applicationName, revisionFile) + '"';
        }
        case VERSION_DEKTEC: {
            // The version of Dektec components.
            return GetDektecVersions();
        }
        case VERSION_TINYXML: {
            // The version of TinyXML-2.
            return Format("TinyXML-2 %d.%d.%d", TIXML2_MAJOR_VERSION, TIXML2_MINOR_VERSION, TIXML2_PATCH_VERSION);
        }
        case VERSION_SHORT: {
            // The simple version with the revision from the current executable and the TSDuck library.
            const int revision = GetRevision(revisionFile, true);
            return revision == 0 ?
                Format("%d.%d", TS_VERSION_MAJOR, TS_VERSION_MINOR) :
                Format("%d.%d-%d", TS_VERSION_MAJOR, TS_VERSION_MINOR, revision);
        }
        case VERSION_GLOBAL: {
            // Same as short but use the highest revision of all TSDuck files.
            StringVector files;
            GetAllModules(files);
            int revision = 0;
            for (StringVector::const_iterator it = files.begin(); it != files.end(); ++it) {
                revision = std::max(revision, GetRevision(*it, false));
            }
            return revision == 0 ?
                Format("%d.%d", TS_VERSION_MAJOR, TS_VERSION_MINOR) :
                Format("%d.%d-%d", TS_VERSION_MAJOR, TS_VERSION_MINOR, revision);
        }
        case VERSION_FILES: {
            // A list of revisions for all files.
            StringVector files;
            GetAllModules(files);
            std::string list("Revision  File\n--------  ");
            list.append(std::max<size_t>(4, LargestLength(files)), '-');
            for (StringVector::const_iterator it = files.begin(); it != files.end(); ++it) {
                const int rev = GetRevision(*it, false);
                if (rev != 0) {
                    list.append(Format("\n%8d  %s", rev, it->c_str()));
                }
            }
            return list;
        }
        default: {
            // Undefined type, return an empty string.
            return std::string();
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
    void NextWord(std::ifstream& file, std::string& word, size_t max_size, char delim)
    {
        word.clear();
        char c;
        while (word.size() < max_size && (file >> c) && (c != delim)) {
            word.append(1, c);
        }
        ts::Trim(word);
    }
}


//-----------------------------------------------------------------------------
// Decode a __DATE__ string and return the corresponding integer value.
// The string is "Mmm dd yyyy", where the names of the months are the
// same as those generated by the asctime function, and the first character
// of dd is a space character if the value is less than 10.
//-----------------------------------------------------------------------------

namespace {
    int DecodeDate(const std::string& word)
    {
        int day = 0;
        int year = 0;
        if (word.length() < 11 ||
            !ts::ToInteger(day, word.substr(4, 2)) ||
            day < 1 || day > 31 ||
            !ts::ToInteger(year, word.substr(7, 4)) ||
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

int ts::GetRevision(const std::string& fileName, bool includeLibrary)
{
    int revision = 0;
    std::string execFile(fileName.empty() ? ExecutableFile() : fileName);
    const char delim = TS_BUILD_MARK_SEPARATOR[0];
    std::ifstream file(execFile.c_str(), std::ios::binary);
    std::string word;
    int rev;
    char c;

    // Search for the revision in the specified file.
    file >> std::noskipws;
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
        std::string shlibName(DirectoryName(execFile));
        shlibName += PathSeparator;
        shlibName += "tsduck";
        shlibName += SharedLibrary::Extension;
        revision = std::max(revision, GetRevision(shlibName, false));
    }

    return revision;
}
