//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsEnvironment.h"
#include "tsTime.h"
#include "tsErrCodeReport.h"

namespace ts {
    //!
    //! Executable file suffix.
    //!
#if defined(DOXYGEN)
    constexpr const UChar* EXECUTABLE_FILE_SUFFIX = platform - specific(".exe", "");  // for doc only
#elif defined(TS_WINDOWS)
    constexpr const UChar* EXECUTABLE_FILE_SUFFIX = u".exe";
#else
    constexpr const UChar* EXECUTABLE_FILE_SUFFIX = u"";
#endif

    //!
    //! File name extension of shared library file names (".so" on Linux, '.dylib" on macOS, ".dll" on Windows).
    //!
#if defined(DOXYGEN)
    constexpr const UChar* SHARED_LIBRARY_SUFFIX = platform - specific(".dll", ".so", ".dylib");  // for doc only
#elif defined(TS_WINDOWS)
    constexpr const UChar* SHARED_LIBRARY_SUFFIX = u".dll";
#elif defined(TS_MAC)
    constexpr const UChar* SHARED_LIBRARY_SUFFIX = u".dylib";
#else
    constexpr const UChar* SHARED_LIBRARY_SUFFIX = u".so";
#endif

    //!
    //! Case-sensitivity of the names in the file system.
    //!
#if defined(DOXYGEN)
    constexpr CaseSensitivity FILE_SYSTEM_CASE_SENSITVITY = platform-specific;
#elif defined(TS_WINDOWS)
    constexpr CaseSensitivity FILE_SYSTEM_CASE_SENSITVITY = CASE_INSENSITIVE;
#elif defined(TS_UNIX)
    constexpr CaseSensitivity FILE_SYSTEM_CASE_SENSITVITY = CASE_SENSITIVE;
#else
#error "Unimplemented operating system"
#endif

    //!
    //! Default separator in CSV (comma-separated values) format.
    //! CSV files are suitable for analysis using tools such as Microsoft Excel.
    //!
    constexpr const UChar* DEFAULT_CSV_SEPARATOR = u",";

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
                                       CaseSensitivity caseSensitivity = FILE_SYSTEM_CASE_SENSITVITY,
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
    //! Get the current user's home directory.
    //!
    //! @return The full path of the current user's home directory.
    //! @throw ts::Exception In case of operating system error.
    //!
    TSDUCKDLL fs::path UserHomeDirectory();

    //!
    //! Return the name of a unique temporary file.
    //! @param [in] suffix An optional suffix to add to the file name.
    //! @return A unique temporary file name.
    //!
    TSDUCKDLL fs::path TempFile(const UString& suffix = u".tmp");

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
    //! @return The path to an existing file or an empty path if not found.
    //!
    TSDUCKDLL UString SearchExecutableFile(const UString& fileName, const UString& pathName = PATH_ENVIRONMENT_VARIABLE);

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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Get all files matching a specified wildcard pattern and append them into a container.
template <class CONTAINER>
bool ts::ExpandWildcardAndAppend(CONTAINER& container, const UString& pattern)
{
#if defined(TS_WINDOWS)

    // On Win32, FindFirstFile / FindNextFile return the file name without directory.
    // We keep the directory part in the pattern to add it later to all file names.
    const UString::size_type pos = pattern.rfind(fs::path::preferred_separator);
    const UString dir(pos == NPOS ? u"" : pattern.substr(0, pos + 1));

    ::WIN32_FIND_DATAW fdata;

    // Initiate the search
    ::HANDLE handle = ::FindFirstFileW(pattern.wc_str(), &fdata);
    if (handle == INVALID_HANDLE_VALUE) {
        // No file matching the pattern is not an error
        const ::DWORD status = ::GetLastError();
        return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
    }

    // Loop on all file matching the pattern
    do {
        // Get next file name.
        fdata.cFileName[sizeof(fdata.cFileName) / sizeof(fdata.cFileName[0]) - 1] = 0;
        const UString file(reinterpret_cast<const UChar*>(fdata.cFileName));

        // Filter out . and ..
        if (file != u"." && file != u"..") {
            container.push_back(dir + file);
        }
    } while (::FindNextFileW(handle, &fdata) != 0);
    const ::DWORD status = ::GetLastError(); // FindNextFile status

    // Cleanup the search context
    ::FindClose(handle);
    return status == ERROR_SUCCESS || status == ERROR_NO_MORE_FILES; // normal end of search

#elif defined(TS_UNIX)

    ::glob_t gl;
    std::memset(&gl, 0, sizeof (gl));
    int status = ::glob(pattern.toUTF8().c_str(), 0, nullptr, &gl);
    if (status == 0) {
        for (size_t n = 0; n < gl.gl_pathc; n++) {
            const UString file(UString::FromUTF8(gl.gl_pathv[n]));
            // Filter out . and ..
            if (file != u"." && file != u"..") {
                container.push_back(file);
            }
        }
    }
    ::globfree(&gl);
    return status == 0 || status == GLOB_NOMATCH;

#else
    #error "Unimplemented operating system"
#endif
}

// Search all files matching a specified wildcard pattern in a directory tree.
template <class CONTAINER>
bool ts::SearchWildcardAndAppend(CONTAINER& container, const UString& root, const UString& pattern, size_t max_levels, bool skip_symlinks)
{
    // Append all files directly matching the wildcard in root directory.
    bool status = ExpandWildcardAndAppend(container, root + fs::path::preferred_separator + pattern);

    // If the maximum number of recursion levels is not reached, recurse in all subdirectories.
    if (max_levels > 0) {
        // Search all files under root and will select directories only.
        UStringList locals;
        ExpandWildcard(locals, root + fs::path::preferred_separator + u"*");
        for (const auto& loc : locals) {
            if (fs::is_directory(loc) && (!skip_symlinks || !fs::is_symlink(loc, &ErrCodeReport()))) {
                status = SearchWildcardAndAppend(container, loc, pattern, max_levels - 1) && status;
            }
        }
    }

    return status;
}
