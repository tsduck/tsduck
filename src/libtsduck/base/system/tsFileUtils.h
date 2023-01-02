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
//!
//!  @file
//!  @ingroup system
//!  File utilities (file path, file properties, etc).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsEnumUtils.h"
#include "tsTime.h"
#include "tsCerrReport.h"

//!
//! Executable file suffix.
//!
#if defined(DOXYGEN)
    #define TS_EXECUTABLE_SUFFIX platform-specific (".exe", "") // for doc only
#elif defined(TS_WINDOWS)
    #define TS_EXECUTABLE_SUFFIX u".exe"
#else
    #define TS_EXECUTABLE_SUFFIX u""
#endif

//!
//! File name extension of shared library file names (".so" on Linux, '.dylib" on macOS, ".dll" on Windows).
//!
#if defined(DOXYGEN)
#define TS_SHARED_LIB_SUFFIX platform-specific (".dll", ".so", ".dylib") // for doc only
#elif defined(TS_WINDOWS)
    #define TS_SHARED_LIB_SUFFIX u".dll"
#elif defined(TS_MAC)
    #define TS_SHARED_LIB_SUFFIX u".dylib"
#else
    #define TS_SHARED_LIB_SUFFIX u".so"
#endif

//!
//! Environment variable containing the command search path.
//!
#if defined(DOXYGEN)
    #define TS_COMMAND_PATH platform-specific ("PATH", "Path") // for doc only
#elif defined(TS_WINDOWS)
    #define TS_COMMAND_PATH u"Path"
#elif defined(TS_UNIX)
    #define TS_COMMAND_PATH u"PATH"
#else
    #error "Unimplemented operating system"
#endif

//!
//! Name of the environment variable which contains a list of paths for plugins.
//!
#define TS_PLUGINS_PATH u"TSPLUGINS_PATH"

namespace ts {
    //!
    //! Directory separator character in file paths.
    //!
#if defined(DOXYGEN)
    const UChar PathSeparator = platform-specific ('/', '\\'); // for doc only
#elif defined(TS_WINDOWS)
    const UChar PathSeparator = u'\\';
#elif defined(TS_UNIX)
    const UChar PathSeparator = u'/';
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Case-sensitivity of the names in the file system.
    //!
#if defined(DOXYGEN)
    const CaseSensitivity FileSystemCaseSensitivity = platform-specific;
#elif defined(TS_WINDOWS)
    const CaseSensitivity FileSystemCaseSensitivity = CASE_INSENSITIVE;
#elif defined(TS_UNIX)
    const CaseSensitivity FileSystemCaseSensitivity = CASE_SENSITIVE;
#else
#error "Unimplemented operating system"
#endif

    //!
    //! Return a "vernacular" version of a file path.
    //!
    //! @param [in] path A file path.
    //! @return A copy of @a path where all '/' and '\' have been
    //! translated into the local directory separator.
    //!
    TSDUCKDLL UString VernacularFilePath(const UString& path);

    //!
    //! Check if a file path is absolute (starting at a root of a file system).
    //!
    //! @param [in] path A file path.
    //! @return True if @a path is an absolute file path.
    //!
    TSDUCKDLL bool IsAbsoluteFilePath(const UString& path);

    //!
    //! Build the absolute form of a file path.
    //!
    //! @param [in] path A file path.
    //! @param [in] base The base directory to use if @a path is a relative file path.
    //! By default, when @a base is empty, the current working directory is used.
    //! @return The absolute form of @a path after cleanup.
    //!
    TSDUCKDLL UString AbsoluteFilePath(const UString& path, const UString& base = UString());

    //!
    //! Build a relative form of a file path, relative to a base directory.
    //!
    //! @param [in] path A file path.
    //! @param [in] base The base directory to use.
    //! By default, when @a base is empty, the current working directory is used.
    //! @param [in] caseSensitivity Case sensitivity of file names comparison.
    //! By default, use the local file system case sensitivity.
    //! @param [in] portableSlashes If true, the relative path contains forward slashes ('/'),
    //! even on Windows. The resulting path can be used in relative URL's for instance.
    //! @return The absolute form of @a path after cleanup.
    //!
    TSDUCKDLL UString RelativeFilePath(const UString& path,
                                       const UString& base = UString(),
                                       CaseSensitivity caseSensitivity = FileSystemCaseSensitivity,
                                       bool portableSlashes = false);

    //!
    //! Cleanup a file path.
    //!
    //! @param [in] path A file path.
    //! @return The clean form of @a path. Double slashes are removed.
    //! Forms such as "." or ".." are reduced.
    //!
    TSDUCKDLL UString CleanupFilePath(const UString& path);

    //!
    //! Return the directory name of a file path ("dir/foo.bar" => "dir").
    //!
    //! @param [in] path A file path.
    //! @return The directory name of @a path ("dir/foo.bar" => "dir").
    //!
    TSDUCKDLL UString DirectoryName(const UString& path);

    //!
    //! Return the base file name of a file path ("dir/foo.bar" => "foo.bar").
    //!
    //! @param [in] path A file path.
    //! @param [in] suffix An optional file suffix.
    //! If @a path ends in @a suffix, the suffix is removed.
    //! @return The base file name of @a path ("dir/foo.bar" => "foo.bar").
    //!
    TSDUCKDLL UString BaseName(const UString& path, const UString& suffix = UString());

    //!
    //! Return the suffix of a file path ("dir/foo.bar" => ".bar").
    //!
    //! @param [in] path A file path.
    //! @return The suffix of @a path ("dir/foo.bar" => ".bar").
    //!
    TSDUCKDLL UString PathSuffix(const UString& path);

    //!
    //! Conditionally add a suffix to a file path.
    //!
    //! If the file path does not contain a suffix, add the specified one.
    //! Otherwise, return the name unchanged.
    //!
    //! @param [in] path A file path.
    //! @param [in] suffix The suffix to conditionally add.
    //! @return The @a path with a suffix (for conditional suffix ".bar",
    //! "dir/foo" => "dir/foo.bar" and "dir/foo.too" => "dir/foo.too").
    //!
    TSDUCKDLL UString AddPathSuffix(const UString& path, const UString& suffix);

    //!
    //! Return the prefix of a file path ("dir/foo.bar" => "dir/foo").
    //!
    //! @param [in] path A file path.
    //! @return The prefix of @a path ("dir/foo.bar" => "dir/foo").
    //!
    TSDUCKDLL UString PathPrefix(const UString& path);

    //!
    //! Get the current user's home directory.
    //!
    //! @return The full path of the current user's home directory.
    //! @throw ts::Exception In case of operating system error.
    //!
    TSDUCKDLL UString UserHomeDirectory();

    //!
    //! Check if a file path is a symbolic link.
    //! @param [in] path A file path.
    //! @return True if @a path is a symbolic link.
    //!
    TSDUCKDLL bool IsSymbolicLink(const UString& path);

    //!
    //! Flags for ResolveSymbolicLinks().
    //!
    enum ResolveSymbolicLinksFlags {
        LINK_SINGLE   = 0x0000,  //!< Default: simply single name resolution.
        LINK_RECURSE  = 0x0001,  //!< Resolve symbolic recursively.
        LINK_ABSOLUTE = 0x0002,  //!< Rebuild absolute path.
    };

    //!
    //! Resolve symbolic links.
    //! On Unix systems, resolve symbolic links and return the corresponding link.
    //! On Windows and systems without symbolic links, return @a path.
    //! @param [in] path A file path.
    //! @param [in] flags Option flags, bit mask of ResolveSymbolicLinksFlags values.
    //! @return The fully resolved path.
    //!
    TSDUCKDLL UString ResolveSymbolicLinks(const UString& path, ResolveSymbolicLinksFlags flags = LINK_SINGLE);

    //!
    //! Get the current working directory.
    //! @return The current working directory.
    //!
    TSDUCKDLL UString CurrentWorkingDirectory();

    //!
    //! Create a directory
    //! @param [in] path A directory path.
    //! @param [in] intermediate When true, also create intermediate directories.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateDirectory(const UString& path, bool intermediate = false, Report& report = CERR);

    //!
    //! Return the name of a directory for temporary files.
    //! @return A system-dependent location where temporary files can be created.
    //!
    TSDUCKDLL UString TempDirectory();

    //!
    //! Return the name of a unique temporary file.
    //! @param [in] suffix An optional suffix to add to the file name.
    //! @return A unique temporary file name.
    //!
    TSDUCKDLL UString TempFile(const UString& suffix = u".tmp");

    //!
    //! Get the size in bytes of a file.
    //!
    //! @param [in] path A file path.
    //! @return Size in bytes of the file or -1 in case of error.
    //!
    TSDUCKDLL int64_t GetFileSize(const UString& path);

    //!
    //! Get the local time of the last modification of a file.
    //!
    //! @param [in] path A file path.
    //! @return Last modification time or Time::Epoch in case of error.
    //!
    TSDUCKDLL Time GetFileModificationTimeLocal(const UString& path);

    //!
    //! Get the UTC time of the last modification of a file.
    //! @param [in] path A file path.
    //! @return Last modification time or Time::Epoch in case of error.
    //!
    TSDUCKDLL Time GetFileModificationTimeUTC(const UString& path);

    //!
    //! Check if a file or directory exists
    //! @param [in] path A file path.
    //! @return True if a file or directory exists with that name, false otherwise.
    //!
    TSDUCKDLL bool FileExists(const UString& path);

    //!
    //! Check if a path exists and is a directory.
    //! @param [in] path A directory path.
    //! @return True if a directory exists with that name, false otherwise.
    //!
    TSDUCKDLL bool IsDirectory(const UString& path);

    //!
    //! Check if a file exists and is executable.
    //! @param [in] path A file path.
    //! @return True if a file exists with that name and is executable, false otherwise.
    //!
    TSDUCKDLL bool IsExecutable(const UString& path);

    //!
    //! Delete a file or directory.
    //!
    //! If the specified path is a directory, it must be empty.
    //! Otherwise, an error is returned.
    //!
    //! @param [in] path A file or directory path.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteFile(const UString& path, Report& report = CERR);

    //!
    //! Truncate a file to the specified size.
    //!
    //! @param [in] path A file path.
    //! @param [in] size Size in bytes after which the file shall be truncated.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool TruncateFile(const UString& path, uint64_t size, Report& report = CERR);

    //!
    //! Rename / move a file or directory.
    //!
    //! If the path specifies a directory, all files in the directory
    //! are moved as well.
    //!
    //! This method is not guaranteed to work when the new and old names
    //! are on distinct volumes or file systems.
    //!
    //! @param [in] old_path The file path of an existing file or directory.
    //! @param [in] new_path The new name for the file or directory.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool RenameFile(const UString& old_path, const UString& new_path, Report& report = CERR);

    //!
    //! Get all files matching a specified wildcard pattern and append them into a container.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c UString receiving the
    //! the names of all files matching the wildcard. The names are appended
    //! at the end of the existing content of the container.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of
    //! the wildcards is system-dependent.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    bool ExpandWildcardAndAppend(CONTAINER& container, const UString& pattern);

    //!
    //! Get all files matching a specified wildcard pattern.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c UString receiving the
    //! the names of all files matching the wildcard.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of
    //! the wildcards is system-dependent.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    inline bool ExpandWildcard(CONTAINER& container, const UString& pattern)
    {
        container.clear();
        return ExpandWildcardAndAppend(container, pattern);
    }

    //!
    //! Search all files matching a specified wildcard pattern in a directory tree and append them into a container.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c UString receiving the the names of all files matching the wildcard.
    //! The names are appended at the end of the existing content of the container.
    //! @param [in] root Root directory into which the files are searched.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of the wildcards is system-dependent.
    //! @param [in] max_levels Maximum number of directory recursions. Since some operating systems allow
    //! loops in the file system, it is a good idea to set some limit to avoid infinite recursion.
    //! @param [in] skip_symlinks If true, do not recurse through symbolic links to directories.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    bool SearchWildcardAndAppend(CONTAINER& container, const UString& root, const UString& pattern, size_t max_levels = 64, bool skip_symlinks = true);

    //!
    //! Search all files matching a specified wildcard pattern in a directory tree.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [out] container A container of @c UString receiving the the names of all files matching the wildcard.
    //! @param [in] root Root directory into which the files are searched.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of the wildcards is system-dependent.
    //! @param [in] max_levels Maximum number of directory recursions. Since some operating systems allow
    //! loops in the file system, it is a good idea to set some limit to avoid infinite recursion.
    //! @param [in] skip_symlinks If true, do not recurse through symbolic links to directories.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    inline bool SearchWildcard(CONTAINER& container, const UString& root, const UString& pattern, size_t max_levels = 64, bool skip_symlinks = true)
    {
        container.clear();
        return SearchWildcardAndAppend(container, root, pattern, max_levels);
    }

    //!
    //! Search an executable file.
    //! @param [in] fileName Name of the file to search.
    //! @param [in] pathName Name of the seach path environment variable.
    //! @return The path to an existing file or an empty string if not found.
    //!
    TSDUCKDLL UString SearchExecutableFile(const UString& fileName, const UString& pathName = TS_COMMAND_PATH);

    //!
    //! Search a configuration file.
    //! @param [in] fileName Name of the file to search.
    //! If @a fileName is not found and does not contain any directory part, search this file
    //! in the following places:
    //! - All directories in @c TSPLUGINS_PATH environment variable.
    //! - Directory of the current executable.
    //! - Directory ../etc/tsduck from current executable (UNIX only).
    //! - Directory ../../etc/tsduck from current executable (UNIX only).
    //! - Directory ../lib64/tsduck from current executable (64-bit UNIX only).
    //! - Directory ../lib/tsduck from current executable (UNIX only).
    //! - Directory ../share/tsduck from current executable (UNIX only).
    //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
    //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
    //! @return The path to an existing file or an empty string if not found.
    //!
    TSDUCKDLL UString SearchConfigurationFile(const UString& fileName);

    //!
    //! Build the name of a user-specific configuration file.
    //! @param [in] fileName Base name of the configuration file.
    //! @param [in] winFileName Alternative base name on Windows. If empty, @a fileName is used.
    //! @return The path to the user-specific configuration file. The file may exist or not.
    //! The default file location depends on the operating system:
    //! - Windows: @c \%APPDATA%\\tsduck
    //! - Unix: @c $HOME
    //!
    TSDUCKDLL UString UserConfigurationFileName(const UString& fileName, const UString& winFileName = UString());
}

TS_ENABLE_BITMASK_OPERATORS(ts::ResolveSymbolicLinksFlags);
#include "tsFileUtilsTemplate.h"
