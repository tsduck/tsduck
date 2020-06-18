//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsSysUtils.h"
#include "tsStaticInstance.h"
#include "tsMemory.h"
#include "tsUID.h"
#include "tsTime.h"
#include "tsMutex.h"
#include "tsGuard.h"
#include "tsArgs.h"
TSDUCK_SOURCE;

#if defined(TS_WINDOWS)
#include "tsWinUtils.h"
#endif

#if defined(TS_MAC)
#include <sys/resource.h>
#include <mach/mach.h>
#include <mach/message.h>
#include <mach/kern_return.h>
#include <mach/task_info.h>
extern char **environ; // not defined in public headers
#endif

// External calls to environment variables are not reentrant. Use a global mutex.
TS_STATIC_INSTANCE(ts::Mutex, (), EnvironmentMutex)


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
// Return the name of the current application executable file.
//----------------------------------------------------------------------------

ts::UString ts::ExecutableFile()
{
#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 2048> name;
    ::DWORD length = ::GetModuleFileNameW(nullptr, name.data(), ::DWORD(name.size()));
    return UString(name, length);

#elif defined(TS_LINUX)

    // Linux implementation.
    // /proc/self/exe is a symbolic link to the executable.
    return ResolveSymbolicLinks(u"/proc/self/exe");

#elif defined(TS_MAC)

    // MacOS implementation.
    // The function proc_pidpath is documented as "private" and "subject to change".
    // Another option is _NSGetExecutablePath (not tested here yet).
    int length;
    char name[PROC_PIDPATHINFO_MAXSIZE];
    if ((length = ::proc_pidpath(getpid(), name, sizeof(name))) < 0) {
        throw ts::Exception(u"proc_pidpath error", errno);
    }
    else {
        assert(length <= int(sizeof(name)));
        return UString::FromUTF8(name, length);
    }


#else
#error "ts::ExecutableFile not implemented on this system"
#endif
}


//----------------------------------------------------------------------------
// Suspend the current thread for the specified period
//----------------------------------------------------------------------------

void ts::SleepThread(MilliSecond delay)
{
#if defined(TS_WINDOWS)

    // Window implementation.
    ::Sleep(::DWORD(delay));

#else

    // POSIX implementation.
    ::timespec requested, remain;
    requested.tv_sec = time_t(delay / 1000); // seconds
    requested.tv_nsec = long((delay % 1000) * 1000000); // nanoseconds
    while (::nanosleep(&requested, &remain) < 0) {
        if (errno == EINTR) {
            // Interrupted by a signal. Wait again.
            requested = remain;
        }
        else {
            // Actual error
            throw ts::Exception(u"nanosleep error", errno);
        }
    }

#endif
}


//----------------------------------------------------------------------------
// Get current process characteristics.
//----------------------------------------------------------------------------

ts::ProcessId ts::CurrentProcessId()
{
#if defined(TS_WINDOWS)
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

bool ts::IsPrivilegedUser()
{
#if defined(TS_UNIX)
    return ::geteuid() == 0;
#else
    ::SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ::PSID AdministratorsGroup;
    ::BOOL ok = ::AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
    if (ok) {
        if (!::CheckTokenMembership(nullptr, AdministratorsGroup, &ok)) {
            ok = false;
        }
        ::FreeSid(AdministratorsGroup);
    }
    return ok;
#endif
}


//----------------------------------------------------------------------------
// Create a directory
//----------------------------------------------------------------------------

ts::ErrorCode ts::CreateDirectory(const UString& path, bool intermediate)
{
    // Create intermediate directories.
    if (intermediate) {
        const UString dir(DirectoryName(path));
        // Create only if does not exist or is identical to path (meaning root).
        if (dir != path && !IsDirectory(dir)) {
            // Create recursively.
            const ErrorCode err = CreateDirectory(dir, true);
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

ts::ErrorCode ts::DeleteFile(const UString& path)
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

ts::ErrorCode ts::TruncateFile(const UString& path, uint64_t size)
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

ts::ErrorCode ts::RenameFile(const UString& old_path, const UString& new_path)
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
// Format an error code into a string
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS)
// Depending on GNU vs. POSIX, strerror_r returns an int or a char*.
// There are two short functions to handle the strerror_r result.
// The C++ compiler will automatically invoke the right one.
// The other one is unused (disable unused warning).

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(unused-function)
TS_GCC_NOWARNING(unused-function)
namespace {
    // POSIX version, strerror_r returns an int, leave result unmodified.
    inline void handle_strerror_r(bool& found, char*& result, int strerror_t_ret)
    {
        found = strerror_t_ret == 0; // success
    }
    // GNU version, strerror_r returns char*, not necessarily in buffer.
    inline void handle_strerror_r(bool& found, char*& result, char* strerror_t_ret)
    {
        result = strerror_t_ret; // actual message
        found = result != nullptr;
    }
}
TS_POP_WARNING()
#endif // not Windows

// Portable public interface:
ts::UString ts::ErrorCodeMessage(ts::ErrorCode code)
{
#if defined(TS_WINDOWS)
    return WinErrorMessage(code);
#else
    char message[1024];
    TS_ZERO(message);

    char* result = message;
    bool found = false;
    handle_strerror_r(found, result, strerror_r(code, message, sizeof(message)));

    if (found) {
        // Make sure message is nul-terminated.
        message[sizeof(message) - 1] = 0;
        // Remove trailing newlines (if any)
        for (size_t i = ::strlen(result); i > 0 && (result[i - 1] == '\n' || result[i - 1] == '\r'); result[--i] = 0) {}
        return UString::FromUTF8(result);
    }

    // At this point, the message is not found.
    return UString::Format(u"System error %d (0x%X)", {code, code});
#endif
}


//----------------------------------------------------------------------------
// Get metrics for the current process
//----------------------------------------------------------------------------

void ts::GetProcessMetrics(ProcessMetrics& metrics)
{
    metrics.cpu_time = 0;
    metrics.vmem_size = 0;

#if defined(TS_WINDOWS)

    // Windows implementation

    // Get a handle to the current process
    ::HANDLE proc(::GetCurrentProcess());

    // Get process CPU time
    ::FILETIME creation_time, exit_time, kernel_time, user_time;
    if (::GetProcessTimes(proc, &creation_time, &exit_time, &kernel_time, &user_time) == 0) {
        throw ts::Exception(u"GetProcessTimes error", ::GetLastError());
    }
    metrics.cpu_time = ts::Time::Win32FileTimeToMilliSecond(kernel_time) + ts::Time::Win32FileTimeToMilliSecond(user_time);

    // Get virtual memory size
    ::PROCESS_MEMORY_COUNTERS_EX mem_counters;
    if (::GetProcessMemoryInfo(proc, (::PROCESS_MEMORY_COUNTERS*)&mem_counters, sizeof(mem_counters)) == 0) {
        throw ts::Exception(u"GetProcessMemoryInfo error", ::GetLastError());
    }
    metrics.vmem_size = mem_counters.PrivateUsage;

#elif defined(TS_LINUX)

    // Linux implementation.

    //  Definition of data available from /proc/<pid>/stat
    //  See man page for proc(5) for more details.
    struct ProcessStatus {
        int           pid;         // The process id.
        char          state;       // One char from "RSDZTW"
        int           ppid;        // The PID of the parent.
        int           pgrp;        // The process group ID of the process.
        int           session;     // The session ID of the process.
        int           tty_nr;      // The tty the process uses.
        int           tpgid;       // Process group ID which owns the tty
        unsigned long flags;       // The flags of the process.
        unsigned long minflt;      // Minor faults the process made
        unsigned long cminflt;     // Minor faults the process's children made
        unsigned long majflt;      // Major faults the process made
        unsigned long cmajflt;     // Major faults the process's children made
        unsigned long utime;       // Number of jiffies in user mode
        unsigned long stime;       // Number of jiffies in kernel mode
        long          cutime;      // Jiffies process's children in user mode
        long          cstime;      // Jiffies process's children in kernel mode
        long          priority;    // Standard nice value, plus fifteen.
        long          nice;        // Nice value, from 19 (nicest) to -19 (not nice)
        long          itrealvalue; // Jiffies before the next SIGALRM
        unsigned long starttime;   // Jiffies the process started after system boot
        unsigned long vsize;       // Virtual memory size in bytes.
        long          rss;         // Resident Set Size
        unsigned long rlim;        // Current limit in bytes on the rss
        unsigned long startcode;   // Address above which program text can run.
        unsigned long endcode;     // Address below which program text can run.
        unsigned long startstack;  // Address of the start of the stack
        unsigned long kstkesp;     // Current value of esp (stack pointer)
        unsigned long kstkeip;     // Current EIP (instruction pointer).
        unsigned long signal;      // Bitmap of pending signals (usually 0).
        unsigned long blocked;     // Bitmap of blocked signals
        unsigned long sigignore;   // Bitmap of ignored signals.
        unsigned long sigcatch;    // Bitmap of catched signals.
        unsigned long wchan;       // "Channel" in which the process is waiting
        unsigned long nswap;       // Number of pages swapped - not maintained.
        unsigned long cnswap;      // Cumulative nswap for child processes.
        int           exit_signal; // Signal to be sent to parent when we die.
        int           processor;   // CPU number last executed on.
    };

    static const char filename[] = "/proc/self/stat";
    FILE* fp = fopen(filename, "r");
    if (fp == nullptr) {
        throw ts::Exception(UString::Format(u"error opening %s", {filename}), errno);
    }

    ProcessStatus ps;
    int expected = 37;
    int count = fscanf(fp,
        "%d %*s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu "
        "%ld %ld %ld %ld %*d %ld %lu %lu %ld %lu %lu %lu %lu "
        "%lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d",
        &ps.pid, &ps.state, &ps.ppid, &ps.pgrp, &ps.session,
        &ps.tty_nr, &ps.tpgid, &ps.flags, &ps.minflt,
        &ps.cminflt, &ps.majflt, &ps.cmajflt, &ps.utime,
        &ps.stime, &ps.cutime, &ps.cstime, &ps.priority,
        &ps.nice, &ps.itrealvalue, &ps.starttime, &ps.vsize,
        &ps.rss, &ps.rlim, &ps.startcode, &ps.endcode,
        &ps.startstack, &ps.kstkesp, &ps.kstkeip, &ps.signal,
        &ps.blocked, &ps.sigignore, &ps.sigcatch, &ps.wchan,
        &ps.nswap, &ps.cnswap, &ps.exit_signal, &ps.processor);
    fclose(fp);

    if (count != expected) {
        throw ts::Exception(UString::Format(u"error reading %s, got %d values, expected %d", {filename, count, expected}));
    }

    // Get virtual memory size
    metrics.vmem_size = ps.vsize;

    // Evaluate CPU time
    unsigned long jps = sysconf(_SC_CLK_TCK);   // jiffies per second
    unsigned long jiffies = ps.utime + ps.stime; // CPU time in jiffies
    metrics.cpu_time = (MilliSecond(jiffies) * 1000) / jps;

#elif defined(TS_MAC)

    // MacOS implementation.
    // First, get the virtual memory size using task_info (mach kernel).
    ::mach_task_basic_info_data_t taskinfo;
    TS_ZERO(taskinfo);
    ::mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    const ::kern_return_t status1 = ::task_info(::mach_task_self(), MACH_TASK_BASIC_INFO, ::task_info_t(&taskinfo), &count);
    if (status1 != KERN_SUCCESS) {
        throw ts::Exception(u"task_info error");
    }
    metrics.vmem_size = taskinfo.virtual_size;

    // Then get CPU time using getrusage.
    ::rusage usage;
    const int status2 = ::getrusage(RUSAGE_SELF, &usage);
    if (status2 < 0) {
        throw ts::Exception(u"getrusage error");
    }

    // Add system time and user time, in milliseconds.
    metrics.cpu_time =
        MilliSecond(usage.ru_stime.tv_sec) * MilliSecPerSec +
        MilliSecond(usage.ru_stime.tv_usec) / MicroSecPerMilliSec +
        MilliSecond(usage.ru_utime.tv_sec) * MilliSecPerSec +
        MilliSecond(usage.ru_utime.tv_usec) / MicroSecPerMilliSec;

#else
#error "ts::GetProcessMetrics not implemented on this system"
#endif
}


//----------------------------------------------------------------------------
// Ignore SIGPIPE. On UNIX systems: writing to a broken pipe returns an
// error instead of killing the process. On Windows systems: does nothing.
//----------------------------------------------------------------------------

void ts::IgnorePipeSignal()
{
#if !defined(TS_WINDOWS)
    ::signal(SIGPIPE, SIG_IGN);
#endif
}


//----------------------------------------------------------------------------
// Put standard input / output stream in binary mode.
// On UNIX systems, this does not make any difference.
// On Windows systems, however, in a stream which is not open in
// binary mode, there is automatic translation between LF and CR-LF.
// The standard input / output are open in text mode (non-binary).
// These functions force them into binary mode.
// Return true on success, false on error.
// If report is a subclass or ts::Args, also terminate application.
//----------------------------------------------------------------------------

bool ts::SetBinaryModeStdin(Report& report)
{
#if defined(TS_WINDOWS)
    report.debug(u"setting standard input to binary mode");
    if (::_setmode(_fileno(stdin), _O_BINARY) < 0) {
        report.error(u"cannot set standard input to binary mode");
        Args* args = dynamic_cast<Args*>(&report);
        if (args != 0) {
            args->exitOnError();
        }
        return false;
    }
#endif
    return true;
}

bool ts::SetBinaryModeStdout(Report& report)
{
#if defined(TS_WINDOWS)
    report.debug(u"setting standard output to binary mode");
    if (::_setmode(_fileno(stdout), _O_BINARY) < 0) {
        report.error(u"cannot set standard output to binary mode");
        Args* args = dynamic_cast<Args*>(&report);
        if (args != 0) {
            args->exitOnError();
        }
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Check if an environment variable exists
//----------------------------------------------------------------------------

bool ts::EnvironmentExists(const UString& name)
{
    Guard lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    std::array <::WCHAR, 2> unused;
    return ::GetEnvironmentVariableW(name.wc_str(), unused.data(), ::DWORD(unused.size())) != 0;
#else
    // Flawfinder: ignore: Environment variables are untrustable input.
    return ::getenv(name.toUTF8().c_str()) != nullptr;
#endif
}


//----------------------------------------------------------------------------
// Get the value of an environment variable.
// Return default value if does not exist.
//----------------------------------------------------------------------------

ts::UString ts::GetEnvironment(const UString& name, const UString& def)
{
    Guard lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    std::vector<::WCHAR> value;
    value.resize(512);
    ::DWORD size = ::GetEnvironmentVariableW(name.wc_str(), value.data(), ::DWORD(value.size()));
    if (size >= ::DWORD(value.size())) {
        value.resize(size_t(size + 1));
        size = ::GetEnvironmentVariableW(name.wc_str(), value.data(), ::DWORD(value.size()));
    }
    return size <= 0 ? def : UString(value, size);
#else
    // Flawfinder: ignore: Environment variables are untrustable input.
    const char* value = ::getenv(name.toUTF8().c_str());
    return value != nullptr ? UString::FromUTF8(value) : def;
#endif
}


//----------------------------------------------------------------------------
// Set the value of an environment variable.
//----------------------------------------------------------------------------

bool ts::SetEnvironment(const UString& name, const UString& value)
{
    Guard lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    return ::SetEnvironmentVariableW(name.wc_str(), value.wc_str()) != 0;
#else
    // In case of error, setenv(3) is documented to return -1 but not setting errno.
    return ::setenv(name.toUTF8().c_str(), value.toUTF8().c_str(), 1) == 0;
#endif
}


//----------------------------------------------------------------------------
// Delete an environment variable.
//----------------------------------------------------------------------------

bool ts::DeleteEnvironment(const UString& name)
{
    Guard lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    return ::SetEnvironmentVariableW(name.wc_str(), nullptr) != 0;
#else
    // In case of error, unsetenv(3) is documented to return -1 but and set errno.
    // It is also documented to silently ignore non-existing variables.
    return ::unsetenv(name.toUTF8().c_str()) == 0;
#endif
}


//----------------------------------------------------------------------------
// Expand environment variables inside a file path (or any string).
// Environment variable references are '$name' or '${name}'.
// In the first form, 'name' is the longest combination of letters, digits and underscore.
// A combination \$ is interpreted as a literal $, not an environment variable reference.
//----------------------------------------------------------------------------

ts::UString ts::ExpandEnvironment(const UString& path)
{
    const size_t len = path.length();
    UString expanded;
    expanded.reserve(2 * len);
    size_t index = 0;
    while (index < len) {
        if (path[index] == '\\' && index+1 < len && path[index+1] == '$') {
            // Escaped dollar
            expanded += '$';
            index += 2;
        }
        else if (path[index] != '$') {
            // Regular character
            expanded += path[index++];
        }
        else {
            // Environment variable reference.
            // First, locate variable name and move index in path.
            UString varname;
            if (++index < len) {
                if (path[index] == '{') {
                    // '${name}' format
                    const size_t last = path.find('}', index);
                    if (last == NPOS) {
                        varname = path.substr(index + 1);
                        index = len;
                    }
                    else {
                        varname = path.substr(index + 1, last - index - 1);
                        index = last + 1;
                    }
                }
                else {
                    // '$name' format
                    const size_t last = path.find_first_not_of(u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", index);
                    if (last == NPOS) {
                        varname = path.substr(index);
                        index = len;
                    }
                    else {
                        varname = path.substr(index, last - index);
                        index = last;
                    }
                }
            }
            // Second, replace environment variable
            expanded += GetEnvironment(varname);
        }
    }
    return expanded;
}


//----------------------------------------------------------------------------
// Add a "name=value" string to a container.
// If exact is true, the definition is always valid.
// Otherwise, cleanup the string and ignore lines without "="
//----------------------------------------------------------------------------

namespace {
    void AddNameValue(ts::Environment& env, const ts::UString& line, bool exact)
    {
        ts::UString s(line);

        // With loose line, do some initial cleanup.
        if (!exact) {
            s.trim();
            if (s.empty() || s.front() == u'#') {
                // Empty or comment line
                return;
            }
        }

        // Locate the "=" between name and value.
        const size_t pos = s.find(u"=");

        if (pos == ts::NPOS) {
            // With exact line, no "=" means empty value.
            // With loose line, not a valid definition.
            if (exact) {
                env.insert(std::make_pair(s, ts::UString()));
            }
        }
        else {
            // Isolate name and value.
            ts::UString name(s.substr(0, pos));
            ts::UString value(s.substr(pos + 1));
            // With loose line, do some additional cleanup.
            if (!exact) {
                name.trim();
                value.trim();
                if (value.size() >= 2 && (value.front() == u'\'' || value.front() == u'"') && value.back() == value.front()) {
                    // Remove surrounding quotes in the value.
                    value.pop_back();
                    value.erase(0, 1);
                }
            }
            if (!name.empty()) {
                env.insert(std::make_pair(name, value));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get the content of the entire environment (all environment variables).
//----------------------------------------------------------------------------

void ts::GetEnvironment(Environment& env)
{
    Guard lock(EnvironmentMutex::Instance());
    env.clear();

#if defined(TS_WINDOWS)

    const ::LPWCH strings = ::GetEnvironmentStringsW();
    if (strings != 0) {
        size_t len;
        for (const ::WCHAR* p = strings; (len = ::wcslen(p)) != 0; p += len + 1) {
            assert(sizeof(::WCHAR) == sizeof(UChar));
            AddNameValue(env, UString(reinterpret_cast<const UChar*>(p), len), true);
        }
        ::FreeEnvironmentStringsW(strings);
    }

#else

    for (char** p = ::environ; *p != nullptr; ++p) {
        AddNameValue(env, UString::FromUTF8(*p), true);
    }

#endif
}


//----------------------------------------------------------------------------
// Load a text file containing environment variables.
//----------------------------------------------------------------------------

bool ts::LoadEnvironment(Environment& env, const UString& fileName)
{
    env.clear();
    UStringList lines;
    const bool ok = UString::Load(lines, fileName);
    if (ok) {
        for (UStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {
            AddNameValue(env, *it, false);
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Check if the standard input/output/error is a terminal.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
namespace {
    // On Windows, only the DOS and PowerShell consoles are considered as terminal.
    // We also want to recognize as terminals the Cygwin and Msys consoles (mintty).
    bool StdHandleIsATerminal(::DWORD ns)
    {
        const ::HANDLE handle = ::GetStdHandle(ns);
        switch (::GetFileType(handle)) {
            case FILE_TYPE_CHAR: {
                // A native console (DOS or PowerShell).
                return true;
            }
            case FILE_TYPE_PIPE: {
                // Check if associated file name matches Cygwin or Msys pty name.
                // With mintty, the standard devices are named pipes. With Cygwin,
                // the name starts with \cygwin. With Msys, the name starts with \msys.
                // Then, if the device is the mintty console, the name contains -pty.
                // For actual pipes, -pty is replaced by -pipe.
                const ts::UString name = ts::WinDeviceName(handle).toLower();
                return (name.find(u"\\cygwin") != ts::NPOS || name.find(u"\\msys") != ts::NPOS) && name.find(u"-pty") != ts::NPOS;
            }
            default: {
                // Cannot be a terminal.
                return false;
            }
        }
    }
}
#endif

bool ts::StdInIsTerminal()
{
#if defined(TS_WINDOWS)
    return StdHandleIsATerminal(STD_INPUT_HANDLE);
#else
    return ::isatty(STDIN_FILENO);
#endif
}

bool ts::StdOutIsTerminal()
{
#if defined(TS_WINDOWS)
    return StdHandleIsATerminal(STD_OUTPUT_HANDLE);
#else
    return ::isatty(STDOUT_FILENO);
#endif
}

bool ts::StdErrIsTerminal()
{
#if defined(TS_WINDOWS)
    return StdHandleIsATerminal(STD_ERROR_HANDLE);
#else
    return ::isatty(STDERR_FILENO);
#endif
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


//----------------------------------------------------------------------------
// Get the name of a class from the @c type_info of an object.
//----------------------------------------------------------------------------

#if defined(TS_GCC)
#include <cxxabi.h>
#endif

ts::UString ts::ClassName(const std::type_info& info)
{
    UString name;
    const char* const rtti = info.name();
    if (rtti != nullptr) {
        // By default, use the plain RTTI name. Not always a pretty name.
        name.assignFromUTF8(rtti);
#if defined(TS_GCC)
        // With gcc and clang, this is a C++ mangled name.
        // Demangle it using the portable C++ ABI library.
        int status = 0;
        char* const demangled = abi::__cxa_demangle(rtti, nullptr, nullptr, &status);
        if (demangled != nullptr) {
            name.assignFromUTF8(demangled);
            ::free(demangled);
        }
#endif
        // Cleanup various initial decoration, depending on compiler.
        if (name.startWith(u"class ")) {
            name.erase(0, 6);
        }
        // MSC: `anonymous namespace'::
        // GCC: (anonymous namespace)::
        if (name.find(u"anonymous namespace") == 1 && name.find(u"::") == 21) {
            name.erase(0, 23);
        }
    }
    return name;
}
