//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsFileUtils.h"
#include "tsMemory.h"
#include "tsUID.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Get the current working directory.
//----------------------------------------------------------------------------

ts::UString ts::CurrentWorkingDirectory()
{
#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 2048> name;
    const ::DWORD length = ::GetCurrentDirectoryW(::DWORD(name.size()), name.data());
    return UString(name, length);

#else

    // Unix implementation.
    std::array<char, 2048> name;
    if (::getcwd(name.data(), name.size() - 1) == nullptr) {
        name[0] = '\0'; // error
    }
    else {
        name[name.size() - 1] = '\0'; // enforce null termination.
    }
    return UString::FromUTF8(name.data());

#endif
}


//----------------------------------------------------------------------------
// Return a "vernacular" version of a file path.
//----------------------------------------------------------------------------

ts::UString ts::VernacularFilePath(const UString& path)
{
    UString vern(path);

#if defined(TS_WINDOWS)
    // With Windows Linux Subsystem, the syntax "/mnt/c/" means "C:\"
    if (vern.length() >= 7 && vern.startWith(u"/mnt/") && IsAlpha(vern[5]) && vern[6] == u'/') {
        vern.erase(0, 4);
    }

    // On Cygwin, the syntax "/cygdrive/C/" means "C:\"
    if (vern.startWith(u"/cygdrive/")) {
        vern.erase(0, 9);
    }

    // On Windows, transform "/c/" pattern into "C:\" (typical on Msys).
    if (vern.length() >= 3 && vern[0] == u'/' && IsAlpha(vern[1]) && vern[2] == u'/') {
        vern[0] = ToUpper(vern[1]);
        vern[1] = u':';
        vern[2] = u'\\';
    }
#endif

    // Normalize path separators.
    for (size_t i = 0; i < vern.length(); ++i) {
        if (vern[i] == u'/' || vern[i] == u'\\') {
            vern[i] = PathSeparator;
        }
    }

    return vern;
}


//----------------------------------------------------------------------------
// Check if a file path is absolute (starting at a root of a file system).
//----------------------------------------------------------------------------

bool ts::IsAbsoluteFilePath(const ts::UString& path)
{
#if defined(TS_WINDOWS)
    return path.startWith(u"\\\\") || (path.length() >= 3 && IsAlpha(path[0]) && path[1] == u':' && path[2] == u'\\');
#else
    return !path.empty() && path[0] == u'/';
#endif
}


//----------------------------------------------------------------------------
// Cleanup a file path.
//----------------------------------------------------------------------------

ts::UString ts::CleanupFilePath(const UString& path)
{
    // Include a trailing slash for subsequent substitutions.
    UString clean(path);
    clean.append(PathSeparator);

    // Patterns to clean.
    const UString parent{PathSeparator, u'.', u'.', PathSeparator};  //  /../
    const UString current{PathSeparator, u'.', PathSeparator};       //  /./
    const UString dslash{PathSeparator, PathSeparator};              //  //
    size_t pos = NPOS;
    size_t up = NPOS;

    // Remove redundant directory forms.
    while ((pos = clean.find(dslash)) != NPOS) {
        clean.erase(pos, 1);
    }
    while ((pos = clean.find(current)) != NPOS) {
        clean.erase(pos, 2);
    }

    // Remove redundant "parent/../".
    while ((pos = clean.find(parent)) != NPOS) {
        if (pos == 0) {
            // Path starting with "/../" -> can be removed.
            clean.erase(0, 3);
        }
        else if ((up = clean.rfind(PathSeparator, pos - 1)) == NPOS) {
            // No "/" before "/../" -> the start of the string is the parent.
            clean.erase(0, pos + 4);
        }
        else {
            // Replace "/parent/../" by "/".
            clean.erase(up, pos - up + 3);
        }
    }

    // Remove trailing slashes.
    while (!clean.empty() && clean.back() == PathSeparator) {
        clean.pop_back();
    }
    return clean;
}


//----------------------------------------------------------------------------
// Build the absolute form af a file path.
//----------------------------------------------------------------------------

ts::UString ts::AbsoluteFilePath(const UString& path, const UString& base)
{
    // Starting with a local form of the file path.
    UString full(VernacularFilePath(path));

    // If the path is already absolute, nothing to do.
    if (IsAbsoluteFilePath(full)) {
        return CleanupFilePath(full);
    }
    else {
        return CleanupFilePath((base.empty() ? CurrentWorkingDirectory() : base) + PathSeparator + full);
    }
}


//----------------------------------------------------------------------------
// Build a relative form of a file path, relative to a base directory.
//----------------------------------------------------------------------------

ts::UString ts::RelativeFilePath(const ts::UString &path, const ts::UString &base, ts::CaseSensitivity caseSensitivity, bool portableSlashes)
{
    // Build absolute file path of the target.
    UString target(AbsoluteFilePath(path));

    // Build absolute file path of the base directory, with a trailing path separator.
    UString ref(AbsoluteFilePath(base.empty() ? CurrentWorkingDirectory() : base));
    ref.append(PathSeparator);

    // See how many leading characters are matching.
    size_t same = target.commonPrefixSize(ref, caseSensitivity);

    // Move backward right after the previous path separator to
    // get the length of the common directory parts
    while (same > 0 && target[same - 1] != PathSeparator) {
        --same;
    }

    // If there is zero common character, no relative path is possible.
    // In that case, return the absolute path.
    // Note that this can normally happen on Windows only with paths
    // such as C:\foo\bar and D:\other. On Unix systems, there is at
    // least the root '/' in common.
    if (same > 0) {

        // There is a leading common part, remove it from target.
        target.erase(0, same);

        // For each remaining directory level in reference, insert a "../" in target.
        const UString up{u'.', u'.', PathSeparator};
        for (size_t i = same; i < ref.length(); ++i) {
            if (ref[i] == PathSeparator) {
                target.insert(0, up);
            }
        }
    }

    // Convert portable slashes.
    if (portableSlashes && PathSeparator != u'/') {
        target.substitute(PathSeparator, u'/');
    }

    return target;
}


//----------------------------------------------------------------------------
// Find the last path separator in a name (including portable separator).
//----------------------------------------------------------------------------

namespace {
    ts::UString::size_type LastPathSeparator(const ts::UString& path)
    {
#if defined(TS_WINDOWS)
        // Also accept slash as path separator.
        ts::UString::size_type i = path.length();
        while (i > 0) {
            if (path[--i] == u'\\' || path[i] == u'/') {
                return i;
            }
        }
        return ts::NPOS;
#else
        // Only one possibility.
        return path.rfind(ts::PathSeparator);
#endif
    }
}


//----------------------------------------------------------------------------
// Return the directory name of a file path.
//----------------------------------------------------------------------------

ts::UString ts::DirectoryName(const UString& path)
{
    UString::size_type sep = LastPathSeparator(path);

    if (sep == NPOS) {
        return u".";               // No '/' in path => current directory
    }
    else if (sep == 0) {
        return path.substr(0, 1);  // '/' at beginning => root
    }
    else {
        return path.substr(0, sep);
    }
}


//----------------------------------------------------------------------------
// Return the base name of a file path.
//----------------------------------------------------------------------------

ts::UString ts::BaseName(const UString& path, const UString& suffix)
{
    const UString::size_type sep = LastPathSeparator(path);
    const UString base(path.substr(sep == NPOS ? 0 : sep + 1));
    const bool suffixFound = !suffix.empty() && base.endWith(suffix, FileSystemCaseSensitivity);
    return suffixFound ? base.substr(0, base.size() - suffix.size()) : base;
}


//----------------------------------------------------------------------------
// Return the suffix of a file path (eg. "dir/foo.bar" => ".bar")
//----------------------------------------------------------------------------

ts::UString ts::PathSuffix(const UString& path)
{
    UString::size_type sep = LastPathSeparator(path);
    UString::size_type dot = path.rfind(u'.');

    if (dot == NPOS) {
        return ts::UString();  // no dot in path
    }
    else if (sep != NPOS && dot < sep) {
        return ts::UString();  // dot in directory part, not in base name
    }
    else {
        return path.substr(dot); // dot in base name
    }
}


//----------------------------------------------------------------------------
// If the file path does not contain a suffix, add the specified one.
// Otherwise, return the name unchanged.
//----------------------------------------------------------------------------

ts::UString ts::AddPathSuffix(const UString& path, const UString& suffix)
{
    UString::size_type sep = LastPathSeparator(path);
    UString::size_type dot = path.rfind(u'.');

    if (dot == NPOS || (sep != NPOS && dot < sep)) {
        return path + suffix;
    }
    else {
        return path;
    }
}


//----------------------------------------------------------------------------
// Return the prefix of a file path (eg. "dir/foo.bar" => "dir/foo")
//----------------------------------------------------------------------------

ts::UString ts::PathPrefix(const UString& path)
{
    UString::size_type sep = LastPathSeparator(path);
    UString::size_type dot = path.rfind(u'.');

    if (dot == NPOS) {
        return path;  // no dot in path
    }
    else if (sep != NPOS && dot < sep) {
        return path;  // dot in directory part, not in base name
    }
    else {
        return path.substr(0, dot); // dot in base name
    }
}


//----------------------------------------------------------------------------
// Get the current user's home directory.
//----------------------------------------------------------------------------

ts::UString ts::UserHomeDirectory()
{
#if defined(TS_WINDOWS)

    ::HANDLE process = 0;
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &process) == 0) {
        throw ts::Exception(u"cannot open current process", ::GetLastError());
    }
    std::array<::WCHAR, 2048> name;
    ::DWORD length = ::DWORD(name.size());
    const ::BOOL status = ::GetUserProfileDirectoryW(process, name.data(), &length);
    const ::DWORD error = ::GetLastError();
    ::CloseHandle(process);
    if (status == 0) {
        throw ts::Exception(u"error getting user profile directory", ::GetLastError());
    }
    return UString(name, length);

#else

    return GetEnvironment(u"HOME");

#endif
}


//----------------------------------------------------------------------------
// Create a directory
//----------------------------------------------------------------------------

ts::SysErrorCode ts::CreateDirectory(const UString& path, bool intermediate)
{
    // Create intermediate directories.
    if (intermediate) {
        const UString dir(DirectoryName(path));
        // Create only if does not exist or is identical to path (meaning root).
        if (dir != path && !IsDirectory(dir)) {
            // Create recursively.
            const SysErrorCode err = CreateDirectory(dir, true);
            if (err != SYS_SUCCESS) {
                return err;
            }
        }
    }

    // Create the final directory.
#if defined(TS_WINDOWS)
    return ::CreateDirectoryW(path.wc_str(), nullptr) == 0 ? ::GetLastError() : SYS_SUCCESS;
#else
    return ::mkdir(path.toUTF8().c_str(), 0777) < 0 ? errno : SYS_SUCCESS;
#endif
}


//----------------------------------------------------------------------------
// Return the name of a directory for temporary files.
//----------------------------------------------------------------------------

ts::UString ts::TempDirectory()
{
#if defined(TS_WINDOWS)
    std::array<::WCHAR, 2048> buf;
    ::DWORD status = ::GetTempPathW(::DWORD(buf.size()), buf.data());
    return status <= 0 ? u"C:\\" : UString(buf);
#else
    return u"/tmp";
#endif
}


//----------------------------------------------------------------------------
// Return the name of a unique temporary file name.
//----------------------------------------------------------------------------

ts::UString ts::TempFile(const UString& suffix)
{
    return TempDirectory() + PathSeparator + UString::Format(u"tstmp-%X", {UID::Instance()->newUID()}) + suffix;
}


//----------------------------------------------------------------------------
// Get the size in byte of a file. Return -1 in case of error.
//----------------------------------------------------------------------------

int64_t ts::GetFileSize(const UString& path)
{
#if defined(TS_WINDOWS)
    ::WIN32_FILE_ATTRIBUTE_DATA info;
    return ::GetFileAttributesExW(path.wc_str(), ::GetFileExInfoStandard, &info) == 0 ? -1 :
        (int64_t(info.nFileSizeHigh) << 32) | (int64_t(info.nFileSizeLow) & 0xFFFFFFFFL);
#else
    struct stat st;
    return ::stat(path.toUTF8().c_str(), &st) < 0 ? -1 : int64_t(st.st_size);
#endif
}


//----------------------------------------------------------------------------
// Get the time of last modification of a file.
// Return Time::Epoch in case of error.
//----------------------------------------------------------------------------

ts::Time ts::GetFileModificationTimeUTC(const UString& path)
{
#if defined(TS_WINDOWS)
    ::WIN32_FILE_ATTRIBUTE_DATA info;
    return ::GetFileAttributesExW(path.wc_str(), ::GetFileExInfoStandard, &info) == 0 ? Time::Epoch : Time::Win32FileTimeToUTC(info.ftLastWriteTime);
#else
    struct stat st;
    return ::stat(path.toUTF8().c_str(), &st) < 0 ? Time::Epoch : Time::UnixTimeToUTC(st.st_mtime);
#endif
}

ts::Time ts::GetFileModificationTimeLocal(const UString& path)
{
    const Time time(GetFileModificationTimeUTC(path));
    return time == Time::Epoch ? time : time.UTCToLocal();
}


//----------------------------------------------------------------------------
// Check if a file or directory exists
//----------------------------------------------------------------------------

bool ts::FileExists(const UString& path)
{
#if defined(TS_WINDOWS)
    return ::GetFileAttributesW(path.wc_str()) != INVALID_FILE_ATTRIBUTES;
#else
    // Flawfinder: ignore
    return ::access(path.toUTF8().c_str(), F_OK) == 0;
#endif
}


//----------------------------------------------------------------------------
// Check if a path exists and is a directory
//----------------------------------------------------------------------------

bool ts::IsDirectory(const UString& path)
{
#if defined(TS_WINDOWS)
    const ::DWORD attr = ::GetFileAttributesW(path.wc_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    return ::stat(path.toUTF8().c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}


//----------------------------------------------------------------------------
// Delete a file. Return an error code.
//----------------------------------------------------------------------------

ts::SysErrorCode ts::DeleteFile(const UString& path)
{
#if defined(TS_WINDOWS)
    if (IsDirectory(path)) {
        return ::RemoveDirectoryW(path.wc_str()) == 0 ? ::GetLastError() : SYS_SUCCESS;
    }
    else {
        return ::DeleteFileW(path.wc_str()) == 0 ? ::GetLastError() : SYS_SUCCESS;
    }
#else
    return ::remove(path.toUTF8().c_str()) < 0 ? errno : SYS_SUCCESS;
#endif
}


//----------------------------------------------------------------------------
// Truncate a file to the specified size. Return an error code.
//----------------------------------------------------------------------------

ts::SysErrorCode ts::TruncateFile(const UString& path, uint64_t size)
{
#if defined(TS_WINDOWS)

    ::LONG size_high = ::LONG(size >> 32);
    ::DWORD status = ERROR_SUCCESS;
    ::HANDLE h = ::CreateFileW(path.wc_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) {
        return ::GetLastError();
    }
    else if (::SetFilePointer(h, ::LONG(size), &size_high, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        status = ::GetLastError();
    }
    else if (::SetEndOfFile(h) == 0) {
        status = ::GetLastError();
    }
    ::CloseHandle(h);
    return status;

#else

    return ::truncate(path.toUTF8().c_str(), off_t(size)) < 0 ? errno : 0;

#endif
}


//----------------------------------------------------------------------------
// Rename / move a file. Return an error code.
// Not guaranteed to work across volumes or file systems.
//----------------------------------------------------------------------------

ts::SysErrorCode ts::RenameFile(const UString& old_path, const UString& new_path)
{
#if defined(TS_WINDOWS)
    return ::MoveFileW(old_path.wc_str(), new_path.wc_str()) == 0 ? ::GetLastError() : ERROR_SUCCESS;
#else
    return ::rename(old_path.toUTF8().c_str(), new_path.toUTF8().c_str()) < 0 ? errno : 0;
#endif
}


//----------------------------------------------------------------------------
// Search a configuration file.
//----------------------------------------------------------------------------

ts::UString ts::SearchConfigurationFile(const UString& fileName)
{
    if (fileName.empty()) {
        // No file specified, no file found...
        return UString();
    }
    if (FileExists(fileName)) {
        // The file exists as is, no need to search.
        return fileName;
    }
    if (LastPathSeparator(fileName) != NPOS) {
        // There is a path separator, there is a directory specified and the file does not exist, don't search.
        return UString();
    }

    // At this point, the file name has no directory and is not found in the current directory.
    // Build the list of directories to search. First, start with all directories from $TSPLUGINS_PATH.
    UStringList dirList;
    GetEnvironmentPathAppend(dirList, TS_PLUGINS_PATH);

    // Then, try in same directory as executable.
    const UString execDir(DirectoryName(ExecutableFile()));
    dirList.push_back(execDir);

    // On Unix systens, try etc and lib directories.
#if defined(TS_UNIX)
    const UString execParent(DirectoryName(execDir));
    const UString execGrandParent(DirectoryName(execParent));
    dirList.push_back(execParent + u"/etc/tsduck");
    dirList.push_back(execGrandParent + u"/etc/tsduck");
#if TS_ADDRESS_BITS == 64
    dirList.push_back(execParent + u"/lib64/tsduck");
#endif
    dirList.push_back(execParent + u"/lib/tsduck");
    dirList.push_back(execParent + u"/share/tsduck");
    // Try all directories from $LD_LIBRARY_PATH.
    GetEnvironmentPathAppend(dirList, u"LD_LIBRARY_PATH");
#endif

    // Finally try all directories from $PATH.
    GetEnvironmentPathAppend(dirList, TS_COMMAND_PATH);

    // Add default system locations of the configuration files. This is useful when the
    // application is not a TSDuck one but a third-party application which uses the
    // TSDuck library. In that case, relative paths from the executables are useless.
#if defined(TS_WINDOWS)
    const UString tsroot(GetEnvironment(u"TSDUCK"));
    if (!tsroot.empty()) {
        dirList.push_back(tsroot + u"\\bin");
    }
#elif defined(TS_MAC)
    dirList.push_back(u"/usr/local/share/tsduck");
#elif defined(TS_UNIX)
    dirList.push_back(u"/usr/share/tsduck");
#endif

    // Search the file.
    for (UStringList::const_iterator it = dirList.begin(); it != dirList.end(); ++it) {
        const UString path(*it + PathSeparator + fileName);
        if (FileExists(path)) {
            return path;
        }
    }

    // Not found.
    return UString();
}


//----------------------------------------------------------------------------
// Check if a file path is a symbolic link.
//----------------------------------------------------------------------------

bool ts::IsSymbolicLink(const UString& path)
{
#if defined(TS_UNIX)
    struct stat st;
    TS_ZERO(st);
    if (::lstat(path.toUTF8().c_str(), &st) != 0) {
        return false; // lstat() error
    }
    else {
        return (st.st_mode & S_IFMT) == S_IFLNK;
    }
#else
    // Non Unix systems, no symbolic links.
    return false;
#endif
}


//----------------------------------------------------------------------------
// Resolve symbolic links.
//----------------------------------------------------------------------------

ts::UString ts::ResolveSymbolicLinks(const ts::UString &path, ResolveSymbolicLinksFlags flags)
{
    UString link((flags & LINK_ABSOLUTE) != 0 ? AbsoluteFilePath(path) : path);

#if defined(TS_UNIX)

    // Only on Unix systems: resolve symbolic links.
    std::array<char, 2048> name;
    int foolproof = 64; // Avoid endless loops in failing links.

    // Loop on nested symbolic links.
    while (IsSymbolicLink(link)) {

        // Translate the symbolic link.
        const ssize_t length = ::readlink(link.toUTF8().c_str(), name.data(), name.size());
        if (length <= 0) {
            // Error, cannot translate the link or empty value, return the path.
            break;
        }
        assert(length <= ssize_t(name.size()));

        // Next step is the translated link.
        if ((flags & LINK_ABSOLUTE) != 0) {
            link = AbsoluteFilePath(UString::FromUTF8(name.data(), size_t(length)), DirectoryName(link));
        }
        else {
            link.assignFromUTF8(name.data(), size_t(length));
        }

        // Without recursion, do not loop.
        if ((flags & LINK_RECURSE) == 0 || foolproof-- <= 0) {
            break;
        }
    }

#endif

    return link;
}
