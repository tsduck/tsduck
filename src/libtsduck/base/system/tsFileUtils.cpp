//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsMemory.h"
#include "tsUID.h"
#include "tsErrCodeReport.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <userenv.h>
    #include <memory.h>
    #include <io.h>
    #include <mmsystem.h>  // Memory management
    #include <psapi.h>     // Process API
    #include <comutil.h>   // COM utilities
    #include <Shellapi.h>
    #include "tsAfterStandardHeaders.h"
#else
    #include "tsBeforeStandardHeaders.h"
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <pwd.h>
    #include "tsAfterStandardHeaders.h"
#endif

// Required link libraries under Windows.
#if defined(TS_WINDOWS) && defined(TS_MSC)
    #pragma comment(lib, "userenv.lib")  // GetUserProfileDirectory
#endif


//----------------------------------------------------------------------------
// Return a "vernacular" version of a file path.
//----------------------------------------------------------------------------

ts::UString ts::VernacularFilePath(const UString& path)
{
    UString vern(path);

#if defined(TS_WINDOWS)
    // With Windows Linux Subsystem, the syntax "/mnt/c/" means "C:\"
    if (vern.length() >= 6 && vern.startWith(u"/mnt/") && IsAlpha(vern[5]) && (vern.length() == 6 || vern[6] == u'/')) {
        vern.erase(0, 4);
    }

    // With Cygwin, the syntax "/cygdrive/C/" means "C:\"
    if (vern.startWith(u"/cygdrive/")) {
        vern.erase(0, 9);
    }

    // On Windows, transform "/c/" pattern into "C:\" (typical on Msys).
    if (vern.length() >= 2 && vern[0] == u'/' && IsAlpha(vern[1]) && (vern.length() == 2 || vern[2] == u'/')) {
        vern[0] = ToUpper(vern[1]);
        vern[1] = u':';
        if (vern.length() == 2) {
            vern.append(u'\\');
        }
        else {
            vern[2] = u'\\';
        }
    }
#endif

    // Normalize path separators.
    for (size_t i = 0; i < vern.length(); ++i) {
        if (vern[i] == u'/' || vern[i] == u'\\') {
            vern[i] = fs::path::preferred_separator;
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
    clean.append(fs::path::preferred_separator);

    // Patterns to clean.
    const UString parent{fs::path::preferred_separator, u'.', u'.', fs::path::preferred_separator};  //  /../
    const UString current{fs::path::preferred_separator, u'.', fs::path::preferred_separator};       //  /./
    const UString dslash{fs::path::preferred_separator, fs::path::preferred_separator};              //  //
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
        else if ((up = clean.rfind(fs::path::preferred_separator, pos - 1)) == NPOS) {
            // No "/" before "/../" -> the start of the string is the parent.
            clean.erase(0, pos + 4);
        }
        else {
            // Replace "/parent/../" by "/".
            clean.erase(up, pos - up + 3);
        }
    }

    // Remove trailing slashes.
    while (!clean.empty() && clean.back() == fs::path::preferred_separator) {
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
        return CleanupFilePath((base.empty() ? UString(fs::current_path(&ErrCodeReport())) : base) + fs::path::preferred_separator + full);
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
    UString ref(AbsoluteFilePath(base.empty() ? UString(fs::current_path(&ErrCodeReport())) : base));
    ref.append(fs::path::preferred_separator);

    // See how many leading characters are matching.
    size_t same = target.commonPrefixSize(ref, caseSensitivity);

    // Move backward right after the previous path separator to
    // get the length of the common directory parts
    while (same > 0 && target[same - 1] != fs::path::preferred_separator) {
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
        const UString up{u'.', u'.', fs::path::preferred_separator};
        for (size_t i = same; i < ref.length(); ++i) {
            if (ref[i] == fs::path::preferred_separator) {
                target.insert(0, up);
            }
        }
    }

    // Convert portable slashes.
    if (portableSlashes && fs::path::preferred_separator != u'/') {
        target.substitute(fs::path::preferred_separator, u'/');
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
        return path.rfind(fs::path::preferred_separator);
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
    const bool suffixFound = !suffix.empty() && base.endWith(suffix, FILE_SYSTEM_CASE_SENSITVITY);
    return suffixFound ? base.substr(0, base.size() - suffix.size()) : base;
}


//----------------------------------------------------------------------------
// Get the current user's home directory.
//----------------------------------------------------------------------------

fs::path ts::UserHomeDirectory()
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
// Return the name of a unique temporary file name.
//----------------------------------------------------------------------------

fs::path ts::TempFile(const UString& suffix)
{
    fs::path name(fs::temp_directory_path());
    name /= UString::Format(u"tstmp-%X%s", {UID::Instance().newUID(), suffix});
    return name;
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
// Search an executable file.
//----------------------------------------------------------------------------

ts::UString ts::SearchExecutableFile(const UString& fileName, const UString& pathName)
{
    // Don't search if empty.
    if (fileName.empty()) {
        return UString();
    }

    // Adjust file name with the executable suffix.
    UString name(fileName);
    if (!name.endWith(EXECUTABLE_FILE_SUFFIX, FILE_SYSTEM_CASE_SENSITVITY)) {
        name.append(EXECUTABLE_FILE_SUFFIX);
    }

    // Executable mask at any level.
    const fs::perms exec = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;

    // If there is at least one path separator in the middle, there is a directory specified, don't search.
    if (LastPathSeparator(fileName) != NPOS) {
        // If the file does not exist or is not executable, not suitable.
        return fs::exists(name) && (fs::status(name, &ErrCodeReport()).permissions() & exec) != fs::perms::none ? name : UString();
    }

    // Search in the path.
    UStringList dirs;
    GetEnvironmentPath(dirs, pathName);
    for (const auto& dir : dirs) {
        const UString full(dir + fs::path::preferred_separator + name);
        if (fs::exists(full) && (fs::status(full, &ErrCodeReport()).permissions() & exec) != fs::perms::none) {
            return full;
        }
    }

    // Not found.
    return UString();
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
    if (fs::exists(fileName)) {
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
    GetEnvironmentPathAppend(dirList, PLUGINS_PATH_ENVIRONMENT_VARIABLE);

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
    GetEnvironmentPathAppend(dirList, PATH_ENVIRONMENT_VARIABLE);

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
    for (const auto& dir : dirList) {
        const UString path(dir + fs::path::preferred_separator + fileName);
        if (fs::exists(path)) {
            return path;
        }
    }

    // Not found.
    return UString();
}


//----------------------------------------------------------------------------
// Build the name of a user-specific configuration file.
//----------------------------------------------------------------------------

ts::UString ts::UserConfigurationFileName(const UString& fileName, const UString& winFileName)
{
#if defined(TS_WINDOWS)
    UString root(GetEnvironment(u"APPDATA"));
    if (root.empty()) {
        root = UserHomeDirectory();
    }
    else {
        root.append(u"\\tsduck");
    }
    return root + u"\\" + (winFileName.empty() ? fileName : winFileName);
#else
    return UserHomeDirectory() + u"/" + fileName;
#endif
}
