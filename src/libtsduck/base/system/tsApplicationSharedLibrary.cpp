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

#include "tsApplicationSharedLibrary.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::ApplicationSharedLibrary::ApplicationSharedLibrary(const UString& filename,
                                                       const UString& prefix,
                                                       const UString& library_path,
                                                       SharedLibraryFlags flags,
                                                       Report& report) :
    // Do not load in superclass since plain filename is not the first choice.
    SharedLibrary(UString(), flags, report),
    _prefix(prefix)
{
    // Without file name, nothing to do.
    if (filename.empty()) {
        return;
    }

    const UString basename(BaseName(filename));
    const bool has_directory = basename != filename;

    // If there is no directory in file name, use search rules in specific directories.
    if (!has_directory) {
        // Get a list of directories from environment variable.
        UStringList dirs;
        GetSearchPath(dirs, library_path);

        // Try in each directory.
        for (auto it = dirs.begin(); !isLoaded() && it != dirs.end(); ++it) {
            // First, try name with prefix.
            load(AddPathSuffix(*it + PathSeparator + prefix + basename, TS_SHARED_LIB_SUFFIX));

            // And then try specified name without prefix.
            if (!isLoaded()) {
                load(AddPathSuffix(*it + PathSeparator + basename, TS_SHARED_LIB_SUFFIX));
            }
        }
    }

    // Without directory and still not loaded, try the standard system lookup rules with prefix.
    if (!isLoaded() && !has_directory) {
        load(AddPathSuffix(prefix + filename, TS_SHARED_LIB_SUFFIX));
    }

    // With a directory in name or if still not loaded, try the standard system lookup rules with plain name.
    if (!isLoaded()) {
        load(AddPathSuffix(filename, TS_SHARED_LIB_SUFFIX));
    }
}

ts::ApplicationSharedLibrary::~ApplicationSharedLibrary()
{
}


//----------------------------------------------------------------------------
// The module name is derived from the file name
//----------------------------------------------------------------------------

ts::UString ts::ApplicationSharedLibrary::moduleName() const
{
    const UString name(PathPrefix(BaseName(fileName())));
    return !_prefix.empty() && name.find(_prefix) == 0 ? name.substr(_prefix.size()) : name;
}


//----------------------------------------------------------------------------
// Get the list of directories where to search plugins.
//----------------------------------------------------------------------------

void ts::ApplicationSharedLibrary::GetSearchPath(UStringList& directories, const UString& library_path)
{
    directories.clear();

    if (!library_path.empty()) {
        GetEnvironmentPathAppend(directories, library_path);
    }

    // Then, try in same directory as executable.
    const UString exec_dir(DirectoryName(ExecutableFile()));
    directories.push_back(exec_dir);

    // On Unix systens, try directory ../lib[64]/tsduck/ from main executable.
#if defined(TS_UNIX)
    const UString exec_parent(DirectoryName(exec_dir));
#if (TS_ADDRESS_BITS == 64) && defined(TS_LINUX)
    directories.push_back(exec_parent + u"/lib64/tsduck");
    directories.push_back(exec_parent + u"/lib64");
#endif
    directories.push_back(exec_parent + u"/lib/tsduck");
    directories.push_back(exec_parent + u"/lib");
#if (TS_ADDRESS_BITS == 64) && defined(TS_LINUX)
    directories.push_back(u"/usr/lib64/tsduck");
    directories.push_back(u"/usr/lib64");
#endif
#if defined(TS_MAC) && defined(TS_X86_64)
    directories.push_back(u"/usr/local/lib/tsduck");
    directories.push_back(u"/usr/local/lib");
#elif defined(TS_MAC) && defined(TS_ARM64)
    directories.push_back(u"/opt/homebrew/lib/tsduck");
    directories.push_back(u"/opt/homebrew/lib");
#else
    directories.push_back(u"/usr/lib/tsduck");
    directories.push_back(u"/usr/lib");
#endif
#endif // TS_UNIX

    // On Windows system, try the PATH.
#if defined(TS_WINDOWS)
    GetEnvironmentPathAppend(directories, TS_COMMAND_PATH);
#endif

    // Make sure that the same directory is not present twice.
    RemoveDuplicates(directories);
}


//----------------------------------------------------------------------------
// Get a list of plugins.
//----------------------------------------------------------------------------

void ts::ApplicationSharedLibrary::GetPluginList(UStringVector& files, const UString& prefix, const UString& library_path)
{
    // Reset output arguments.
    files.clear();

    // Get list of directories to search path, then same directory as executable.
    UStringList path_dirs;
    GetSearchPath(path_dirs, library_path);

    // Assume that distinct shared libraries with the same base name contain the same plugin,
    // or two distinct versions of the same plugin. Since they are likely to contain the same
    // symbols, do nat load them both. Keep a set of loaded base names.
    std::set<UString> basenames;

    // Try in each directory.
    size_t index = 0;
    CERR.log(2, u"Searching for plugins %s*%s", {prefix, TS_SHARED_LIB_SUFFIX});
    for (const auto& dir : path_dirs) {
        // Get list of shared library files matching the requested pattern in this directory.
        CERR.log(2, u"Searching in \"%s\"", {dir});
        ExpandWildcardAndAppend(files, dir + PathSeparator + prefix + u"*" TS_SHARED_LIB_SUFFIX);
        // Eliminate files with already registered base names.
        while (index < files.size()) {
            const UString base(BaseName(files[index]));
            if (basenames.find(base) != basenames.end()) {
                CERR.log(2, u"  \"%s\", duplicated, ignored", {files[index]});
                files.erase(files.begin() + index);
            }
            else {
                basenames.insert(base);
                CERR.log(2, u"  \"%s\"", {files[index]});
                index++;
            }
        }
    }

    // Sort the list of plugins.
    std::sort(files.begin(), files.end());

    // Debug section when TS_CERR_DEBUG_LEVEL environment variable is 2 or higher.
    if (CERR.maxSeverity() >= 2) {
        CERR.log(2, u"Results for plugins %s*%s:", {prefix, TS_SHARED_LIB_SUFFIX});
        for (const auto& it : files) {
            CERR.log(2, u"  \"%s\"", {it});
        }
    }
}
