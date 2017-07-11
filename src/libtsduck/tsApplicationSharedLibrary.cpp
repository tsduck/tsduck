//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

// Name of the environment variable which contains a list of paths for plugins.
const char* const ts::ApplicationSharedLibrary::PluginsPathEnvironmentVariable = "TSPLUGINS_PATH";


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::ApplicationSharedLibrary::ApplicationSharedLibrary(const std::string& filename,
                                                       const std::string& prefix,
                                                       const std::string& library_path,
                                                       bool permanent,
                                                       ReportInterface& report) :
    SharedLibrary(filename, permanent, report),
    _prefix(prefix)
{
    const std::string basename(BaseName(filename));
    const std::string suffix(PathSuffix(filename));
    const bool nodir = basename == filename;

    // If not loaded, try with standard extension
    if (!isLoaded() && suffix.empty()) {
        load(filename + SharedLibrary::Extension);
    }

    // If still not loaded, search in several directories if the original name has no directory part.
    if (!isLoaded() && nodir) {

        // Get a list of directories from environment variable.
        StringList dirs;
        if (!library_path.empty()) {
            GetEnvironmentPath(dirs, library_path);
        }

        // Then, try in same directory as executable
        dirs.push_back(DirectoryName(ExecutableFile()));

       // Try in each directory.
       for (StringList::const_iterator it = dirs.begin(); !isLoaded() && it != dirs.end(); ++it) {
           // Try specific name only.
           load(AddPathSuffix(*it + PathSeparator + basename, SharedLibrary::Extension));

           // And try with prefix
           if (!isLoaded()) {
               load(AddPathSuffix(*it + PathSeparator + prefix + basename, SharedLibrary::Extension));
           }
       }
    }
}


//----------------------------------------------------------------------------
// The module name is derived from the file name
//----------------------------------------------------------------------------

std::string ts::ApplicationSharedLibrary::moduleName() const
{
    const std::string name(PathPrefix(BaseName(fileName())));
    return !_prefix.empty() && name.find(_prefix) == 0 ? name.substr(_prefix.size()) : name;
}


//----------------------------------------------------------------------------
// Get a list of plugins.
//----------------------------------------------------------------------------

void ts::ApplicationSharedLibrary::GetPluginList(StringVector& files, const std::string& prefix, const std::string& library_path)
{
    // Reset output arguments.
    files.clear();

    // Get list of directories: search path, then same directory as executable.
    StringList dirs;
    if (!library_path.empty()) {
        GetEnvironmentPath(dirs, library_path);
    }
    dirs.push_back(DirectoryName(ExecutableFile()));

    // Try in each directory.
    for (StringList::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
        // Get list of shared library files matching the requested pattern in this directory.
        ExpandWildcardAndAppend(files, *it + PathSeparator + prefix + "*" + SharedLibrary::Extension);
    }
}
