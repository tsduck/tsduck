//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

ts::ApplicationSharedLibrary::ApplicationSharedLibrary(const fs::path& filename,
                                                       const UString& prefix,
                                                       const UString& library_path,
                                                       SharedLibraryFlags flags,
                                                       Report& report) :
    // Do not load in superclass since plain filename is not the first choice.
    SharedLibrary(fs::path(), flags, report),
    _prefix(prefix)
{
    // Without file name, nothing to do.
    if (filename.empty()) {
        return;
    }

    const fs::path basename(filename.filename());
    const bool has_directory = basename != filename;

    // If there is no directory in file name, use search rules in specific directories.
    if (!has_directory) {
        // Get a list of directories from environment variable.
        UStringList dirs;
        GetSearchPath(dirs, library_path);

        // Try in each directory.
        for (auto it = dirs.begin(); !isLoaded() && it != dirs.end(); ++it) {
            // First, try name with prefix.
            fs::path lib(*it);
            lib /= prefix + basename;
            lib.replace_extension(SHARED_LIBRARY_SUFFIX);
            load(lib);

            // And then try specified name without prefix.
            if (!isLoaded()) {
                lib = *it;
                lib /= basename;
                lib.replace_extension(SHARED_LIBRARY_SUFFIX);
                load(lib);
            }
        }
    }

    // Without directory and still not loaded, try the standard system lookup rules with prefix.
    if (!isLoaded() && !has_directory) {
        fs::path lib(prefix + filename);
        lib.replace_extension(SHARED_LIBRARY_SUFFIX);
        load(lib);
    }

    // With a directory in name or if still not loaded, try the standard system lookup rules with plain name.
    if (!isLoaded()) {
        fs::path lib(filename);
        lib.replace_extension(SHARED_LIBRARY_SUFFIX);
        load(lib);
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
    const UString name(fileName().stem());
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
    const UString exec_dir(ExecutableFile().parent_path());
    directories.push_back(exec_dir);

    // On Unix systens, try directory ../lib[64]/tsduck/ from main executable.
#if defined(TS_UNIX)
    const UString exec_parent(DirectoryName(exec_dir));
#if defined(TS_LINUX)
    if constexpr (sizeof(void*) == 8) {
        directories.push_back(exec_parent + u"/lib64/tsduck");
        directories.push_back(exec_parent + u"/lib64");
    }
#endif
    directories.push_back(exec_parent + u"/lib/tsduck");
    directories.push_back(exec_parent + u"/lib");
#if defined(TS_LINUX)
    if constexpr (sizeof(void*) == 8) {
        directories.push_back(u"/usr/lib64/tsduck");
        directories.push_back(u"/usr/lib64");
    }
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
    GetEnvironmentPathAppend(directories, PATH_ENVIRONMENT_VARIABLE);
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
    CERR.log(2, u"Searching for plugins %s*%s", prefix, SHARED_LIBRARY_SUFFIX);
    for (const auto& dir : path_dirs) {
        // Get list of shared library files matching the requested pattern in this directory.
        CERR.log(2, u"Searching in \"%s\"", dir);
        ExpandWildcardAndAppend(files, dir + fs::path::preferred_separator + prefix + u"*" + SHARED_LIBRARY_SUFFIX);
        // Eliminate files with already registered base names.
        while (index < files.size()) {
            const UString base(BaseName(files[index]));
            if (basenames.find(base) != basenames.end()) {
                CERR.log(2, u"  \"%s\", duplicated, ignored", files[index]);
                files.erase(files.begin() + index);
            }
            else {
                basenames.insert(base);
                CERR.log(2, u"  \"%s\"", files[index]);
                index++;
            }
        }
    }

    // Sort the list of plugins.
    std::sort(files.begin(), files.end());

    // Debug section when TS_CERR_DEBUG_LEVEL environment variable is 2 or higher.
    if (CERR.maxSeverity() >= 2) {
        CERR.log(2, u"Results for plugins %s*%s:", prefix, SHARED_LIBRARY_SUFFIX);
        for (const auto& it : files) {
            CERR.log(2, u"  \"%s\"", it);
        }
    }
}
