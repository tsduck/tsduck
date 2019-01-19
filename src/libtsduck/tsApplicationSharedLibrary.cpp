//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Application shared libraries
//
//----------------------------------------------------------------------------

#include "tsApplicationSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::ApplicationSharedLibrary::ApplicationSharedLibrary(const UString& filename,
                                                       const UString& prefix,
                                                       const UString& library_path,
                                                       bool permanent,
                                                       Report& report) :
    // Do not load in superclass since plain filename is not the first choice.
    SharedLibrary(UString(), permanent, report),
    _prefix(prefix)
{
    // Without file name, nothing to do.
    if (filename.empty()) {
        return;
    }

    const UString basename(BaseName(filename));
    const UString suffix(PathSuffix(filename));
    const bool has_directory = basename != filename;

    if (!has_directory) {
        // There is no directory in file name, use search rules.
        // Get a list of directories from environment variable.
        UStringList dirs;
        if (!library_path.empty()) {
            GetEnvironmentPath(dirs, library_path);
        }

        // Then, try in same directory as executable
        dirs.push_back(DirectoryName(ExecutableFile()));

        // Try in each directory.
        for (UStringList::const_iterator it = dirs.begin(); !isLoaded() && it != dirs.end(); ++it) {
            // First, try name with prefix.
            load(AddPathSuffix(*it + PathSeparator + prefix + basename, TS_SHARED_LIB_SUFFIX));

            // And then try specified name without prefix.
            if (!isLoaded()) {
                load(AddPathSuffix(*it + PathSeparator + basename, TS_SHARED_LIB_SUFFIX));
            }
        }
    }

    // With a directory in name or if still not loaded, try the standard system lookup rules.
    if (!isLoaded()) {
        // Try plain
        load(filename);

        // If not loaded, try with standard extension if filename had no extension.
        if (!isLoaded() && suffix.empty()) {
            load(filename + TS_SHARED_LIB_SUFFIX);
        }
    }
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

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
// Get a list of plugins.
//----------------------------------------------------------------------------

void ts::ApplicationSharedLibrary::GetPluginList(UStringVector& files, const UString& prefix, const UString& library_path)
{
    // Reset output arguments.
    files.clear();

    // Get list of directories: search path, then same directory as executable.
    UStringList dirs;
    if (!library_path.empty()) {
        GetEnvironmentPath(dirs, library_path);
    }
    dirs.push_back(DirectoryName(ExecutableFile()));

    // Try in each directory.
    for (UStringList::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
        // Get list of shared library files matching the requested pattern in this directory.
        ExpandWildcardAndAppend(files, *it + PathSeparator + prefix + u"*" TS_SHARED_LIB_SUFFIX);
    }
}
