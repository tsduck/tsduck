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

#include "tsSysUtils.h"
#include "tsStaticInstance.h"
#include "tsMemoryUtils.h"
#include "tsUID.h"
#include "tsTime.h"
#include "tsMutex.h"
#include "tsGuard.h"
#include "tsArgs.h"
TSDUCK_SOURCE;

#if defined(TS_WINDOWS)
#include "tsComUtils.h"
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
// Return a "vernacular" version of a file path.
//----------------------------------------------------------------------------

ts::UString ts::VernacularFilePath(const UString& path)
{
    UString vern(path);

#if defined(TS_WINDOWS)
    // On Windows, transform "/c/" pattern into "C:\"
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
// Return the directory name of a file path.
//----------------------------------------------------------------------------

ts::UString ts::DirectoryName(const UString& path)
{
    UString::size_type sep = path.rfind(PathSeparator);

    if (sep == UString::NPOS) {
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
    const UString::size_type sep = path.rfind(PathSeparator);
    const UString base(path.substr(sep == UString::NPOS ? 0 : sep + 1));
    const bool suffixFound = !suffix.empty() && base.endWith(suffix, FileSystemCaseSensitivity);
    return suffixFound ? base.substr(0, base.size() - suffix.size()) : base;
}


//----------------------------------------------------------------------------
// Return the suffix of a file path (eg. "dir/foo.bar" => ".bar")
//----------------------------------------------------------------------------

ts::UString ts::PathSuffix(const UString& path)
{
    UString::size_type sep = path.rfind(PathSeparator);
    UString::size_type dot = path.rfind(u'.');

    if (dot == UString::NPOS) {
        return ts::UString();  // no dot in path
    }
    else if (sep != UString::NPOS && dot < sep) {
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
    UString::size_type sep = path.rfind(PathSeparator);
    UString::size_type dot = path.rfind(u'.');

    if (dot == UString::NPOS || (sep != UString::NPOS && dot < sep)) {
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
    UString::size_type sep = path.rfind(PathSeparator);
    UString::size_type dot = path.rfind(u'.');

    if (dot == UString::NPOS) {
        return path;  // no dot in path
    }
    else if (sep != UString::NPOS && dot < sep) {
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
    ::DWORD length = ::GetModuleFileNameW(NULL, name.data(), ::DWORD(name.size()));
    return UString(name, length);

#elif defined(TS_LINUX)

    // Linux implementation.
    // /proc/self/exe is a symbolic link to the executable.
    // Read the value of the symbolic link.
    int length;
    char name[1024];
    // Flawfinder: ignore: readlink does not terminate with ASCII NUL.
    if ((length = ::readlink("/proc/self/exe", name, sizeof(name))) < 0) {
        throw ts::Exception(u"Symbolic link /proc/self/exe error", errno);
    }
    else {
        assert(length <= int(sizeof(name)));
        // We handle here the fact that readlink does not terminate with ASCII NUL.
        return UString::FromUTF8(name, length);
    }

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
// Return the current hostname
//----------------------------------------------------------------------------

ts::UString ts::HostName()
{
#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 1024> name;
    ::DWORD length = ::DWORD(name.size());
    if (::GetComputerNameW(name.data(), &length) == 0) {
        throw ts::Exception(u"GetComputerName error", ::GetLastError());
    }
    return UString(name, length);

#else

    // POSIX implementation.
    char name[1024];
    if (::gethostname(name, sizeof(name)) < 0) {
        return UString();
    }
    else {
        name[sizeof(name) - 1] = '\0';
        return UString::FromUTF8(name);
    }

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
            break;
        }
    }

#endif
}


//----------------------------------------------------------------------------
// Get system memory page size
//----------------------------------------------------------------------------

size_t ts::MemoryPageSize()
{
#if defined(TS_WINDOWS)

    ::SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    return size_t(sysinfo.dwPageSize);

#else

    // POSIX implementation.
    long size = ::sysconf(_SC_PAGESIZE);
    if (size < 0) {
        throw ts::Exception(u"sysconf (page size) error", errno);
    }
    return size_t(size);

#endif
}


//----------------------------------------------------------------------------
// Get current process id
//----------------------------------------------------------------------------

ts::ProcessId ts::CurrentProcessId()
{
#if defined(TS_WINDOWS)
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}


//----------------------------------------------------------------------------
// Create a directory
//----------------------------------------------------------------------------

ts::ErrorCode ts::CreateDirectory(const UString& path)
{
#if defined(TS_WINDOWS)
    return ::CreateDirectoryW(path.wc_str(), NULL) == 0 ? ::GetLastError() : SYS_SUCCESS;
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
    if (fileName.find(PathSeparator) != UString::NPOS) {
        // There is a path separator, there is a directory specified and the file does not exist, don't search.
        return UString();
    }

    // At this point, the file name has no directory and is not found in the current directory.
    // Build the list of directories to search.
    UStringList dirList;
    UStringList tmp;
    dirList.push_back(DirectoryName(ExecutableFile()));
    GetEnvironmentPath(tmp, TS_PLUGINS_PATH);
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());
#if defined(TS_UNIX)
    GetEnvironmentPath(tmp, u"LD_LIBRARY_PATH");
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());
#endif
    GetEnvironmentPath(tmp, TS_COMMAND_PATH);
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());

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

ts::UString ts::ErrorCodeMessage(ts::ErrorCode code)
{
#if defined(TS_WINDOWS)

    // Windows implementation
    std::array<::WCHAR, 1024> message;
    ::DWORD length = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, message.data(), ::DWORD(message.size()), NULL);

    // Remove trailing newlines (if any)
    while (length > 0 && (message[length - 1] == 0 || message[length - 1] == '\n' || message[length - 1] == '\r')) {
        --length;
    }
    if (length > 0) {
        return UString(message, length);
    }

#else

    char* result;
    char message[1024];
    TS_ZERO(message);

#if defined(HAVE_INT_STRERROR_R)
    // POSIX version, strerror_r returns int
    const bool found = 0 == strerror_r(code, message, sizeof(message));
    result = message;
#else
    // GNU version, strerror_r returns char*, not necessarilly in buffer
    result = strerror_r(code, message, sizeof(message));
    const bool found = result != NULL;
#endif

    if (found) {
        // Make sure message is nul-terminated.
        message[sizeof(message) - 1] = 0;
        // Remove trailing newlines (if any)
        for (size_t i = ::strlen(result); i > 0 && (result[i - 1] == '\n' || result[i - 1] == '\r'); result[--i] = 0) {}
        return UString::FromUTF8(result);
    }

#endif

    // At this point, the message is not found.
    return UString::Format(u"System error %d (0x%X)", {code, code});
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
    if (fp == 0) {
        throw ts::Exception(UString::Format(u"error opening %s", {filename}), errno);
        return;
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
        return;
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
        return;
    }
    metrics.vmem_size = taskinfo.virtual_size;

    // Then get CPU time using getrusage.
    ::rusage usage;
    const int status2 = ::getrusage(RUSAGE_SELF, &usage);
    if (status2 < 0) {
        throw ts::Exception(u"getrusage error");
        return;
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
    return ::getenv(name.toUTF8().c_str()) != 0;
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
    return value != 0 ? UString::FromUTF8(value) : def;
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
    return ::SetEnvironmentVariableW(name.wc_str(), NULL) != 0;
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
                    if (last == UString::NPOS) {
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
                    if (last == UString::NPOS) {
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
// Add a "name=value" string to a container
//----------------------------------------------------------------------------

namespace {
    void AddNameValue(ts::Environment& env, const ts::UString& s)
    {
        const size_t pos = s.find(u"=");
        if (pos == ts::UString::NPOS) {
            // No "=", empty value
            env.insert(std::make_pair(s, ts::UString()));
        }
        else {
            env.insert(std::make_pair(s.substr(0, pos), s.substr(pos + 1)));
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
            AddNameValue(env, UString(reinterpret_cast<const UChar*>(p), len));
        }
        ::FreeEnvironmentStringsW(strings);
    }

#else

    for (char** p = ::environ; *p != 0; ++p) {
        AddNameValue(env, UString::FromUTF8(*p));
    }

#endif
}
