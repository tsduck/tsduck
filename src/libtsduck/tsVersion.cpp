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


//----------------------------------------------------------------------------
// Enumeration description of ts::VersionFormat.
//----------------------------------------------------------------------------

const ts::Enumeration ts::VersionFormatEnum
    ("short",  ts::VERSION_SHORT,
     "long",   ts::VERSION_LONG,
     "date",   ts::VERSION_DATE,
     "nsis",   ts::VERSION_NSIS,
     "dektec", ts::VERSION_DEKTEC,
     "files",  ts::VERSION_FILES,
     TS_NULL);


//----------------------------------------------------------------------------
// Get the list of all TSDuck executables and shared libraries.
//----------------------------------------------------------------------------

namespace {
    bool GetAllModules(ts::StringVector& files)
    {
        files.clear();

        // Directory name of executables and libraries.
        const std::string dir(ts::DirectoryName(ts::ExecutableFile()) + ts::PathSeparator);

        // Get all TSDuck executable file names.
#if defined(__windows)
        // Windows: all ts*.exe files
        if (!ts::ExpandWildcard(files, dir + "ts*.exe")) {
            return false;
        }
#else
        // UNIX: all ts* files without extension (no dot).
        ts::StringVector allfiles;
        if (!ts::ExpandWildcard(allfiles, dir + "ts*")) {
            return false;
        }
        for (ts::StringVector::const_iterator it = allfiles.begin(); it != allfiles.end(); ++it) {
            if (it->find('.') == std::string::npos) {
                files.push_back(*it);
            }
        }
#endif

        // Append all TSDuck shared library files.
        return ts::ExpandWildcardAndAppend(files, (dir + "ts*") + ts::SharedLibrary::Extension);
    }
}


//----------------------------------------------------------------------------
// Build version string.
//----------------------------------------------------------------------------

std::string ts::GetVersion(VersionFormat format, const std::string& applicationName, const std::string& revisionFile)
{
    switch (format) {
        case VERSION_LONG: {
            return (applicationName.empty() ? "" : applicationName + ": ") +
                "TSDuck - The MPEG Transport Stream Toolkit "
            #if defined(TS_NO_DTAPI)
                "(no Dektec support) "
            #endif
                "- version " + GetVersion(VERSION_SHORT, applicationName, revisionFile);
        }
        case VERSION_DATE: {
            return Format("%s - %s", __DATE__, __TIME__);
        }
        case VERSION_NSIS: {
            return "!define tsduckVersion \"" + GetVersion(VERSION_SHORT, applicationName, revisionFile) + '"';
        }
        case VERSION_DEKTEC: {
            return GetDektecVersions();
        }
        case VERSION_SHORT: {
            std::string version(TS_STRINGIFY(TS_VERSION_MAJOR) "." TS_STRINGIFY(TS_VERSION_MINOR));
            const int revision = GetRevision(revisionFile, true);
            if (revision != 0) {
                version += Format("-%d", revision);
            }
            return version;
        }
        default: {
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
            word += c;
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
        
        const std::string monthName(ts::LowerCaseValue(word.substr(0, 3)));
        int month = 0;
        if (monthName == "jan") {
            month = 1;
        }
        else if (monthName == "feb") {
            month = 2;
        }
        else if (monthName == "mar") {
            month = 3;
        }
        else if (monthName == "apr") {
            month = 4;
        }
        else if (monthName == "may") {
            month = 5;
        }
        else if (monthName == "jun") {
            month = 6;
        }
        else if (monthName == "jul") {
            month = 7;
        }
        else if (monthName == "aug") {
            month = 8;
        }
        else if (monthName == "sep") {
            month = 9;
        }
        else if (monthName == "oct") {
            month = 10;
        }
        else if (monthName == "nov") {
            month = 11;
        }
        else if (monthName == "dec") {
            month = 12;
        }
        else {
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

    // Search for the revision in the specified file
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
