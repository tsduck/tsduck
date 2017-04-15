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
#include "tsMemoryUtils.h"
#include "tsUID.h"
#include "tsTime.h"
#include "tsFormat.h"
#include "tsMutex.h"
#include "tsGuard.h"
#include "tsArgs.h"

#if defined(__windows)
#include "tsComUtils.h"
#endif

#if defined(__mac)
#include <sys/resource.h>
#include <mach/mach.h>
#include <mach/message.h>
#include <mach/kern_return.h>
#include <mach/task_info.h>
#endif

//
// External calls to environment variables are not reentrant.
// Use a global mutex.
//
namespace {
    ts::Mutex _environmentMutex;
}


//----------------------------------------------------------------------------
// Return a "vernacular" version of a file path.
//----------------------------------------------------------------------------

std::string ts::VernacularFilePath(const std::string& path)
{
    std::string vern(path);

#if defined(__windows)
    // On Windows, transform "/c/" pattern into "C:\"
    if (vern.length() >= 3 && vern[0] == '/' && std::isalpha(vern[1]) && vern[2] == '/') {
        vern[0] = char(std::toupper(vern[1]));
        vern[1] = ':';
        vern[2] = '\\';
    }
#endif

    for (size_t i = 0; i < vern.length(); ++i) {
        if (vern[i] == '/' || vern[i] == '\\') {
            vern[i] = PathSeparator;
        }
    }

    return vern;
}


//----------------------------------------------------------------------------
// Return the directory name of a file path.
//----------------------------------------------------------------------------

std::string ts::DirectoryName(const std::string& path)
{
    std::string::size_type sep = path.rfind (PathSeparator);

    if (sep == std::string::npos) {
        return ".";                 // No '/' in path => current directory
    }
    else if (sep == 0) {
        return path.substr (0, 1);  // '/' at beginning => root
    }
    else {
        return path.substr (0, sep);
    }
}


//----------------------------------------------------------------------------
// Return the base name of a file path.
//----------------------------------------------------------------------------

std::string ts::BaseName(const std::string& path, const std::string& suffix)
{
    const std::string::size_type sep = path.rfind(PathSeparator);
    const std::string base(path.substr(sep == std::string::npos ? 0 : sep + 1));
    const bool suffixFound = !suffix.empty() &&
#if defined(__windows)
        EndWithInsensitive(base, suffix);
#else
        EndWith(base, suffix);
#endif
    return suffixFound ? base.substr(0, base.size() - suffix.size()) : base;
}


//----------------------------------------------------------------------------
// Return the suffix of a file path (eg. "dir/foo.bar" => ".bar")
//----------------------------------------------------------------------------

std::string ts::PathSuffix (const std::string& path)
{
    std::string::size_type sep = path.rfind (PathSeparator);
    std::string::size_type dot = path.rfind ('.');

    if (dot == std::string::npos) {
        return "";  // no dot in path
    }
    else if (sep != std::string::npos && dot < sep) {
        return "";  // dot in directory part, not in base name
    }
    else {
        return path.substr (dot); // dot in base name
    }
}


//----------------------------------------------------------------------------
// If the file path does not contain a suffix, add the specified one.
// Otherwise, return the name unchanged.
//----------------------------------------------------------------------------

std::string ts::AddPathSuffix (const std::string& path, const std::string& suffix)
{
    std::string::size_type sep = path.rfind (PathSeparator);
    std::string::size_type dot = path.rfind ('.');

    if (dot == std::string::npos || (sep != std::string::npos && dot < sep)) {
        return path + suffix;
    }
    else {
        return path;
    }
}


//----------------------------------------------------------------------------
// Return the prefix of a file path (eg. "dir/foo.bar" => "dir/foo")
//----------------------------------------------------------------------------

std::string ts::PathPrefix (const std::string& path)
{
    std::string::size_type sep = path.rfind (PathSeparator);
    std::string::size_type dot = path.rfind ('.');

    if (dot == std::string::npos)
        return path;  // no dot in path
    else if (sep != std::string::npos && dot < sep)
        return path;  // dot in directory part, not in base name
    else
        return path.substr (0, dot); // dot in base name
}


//----------------------------------------------------------------------------
// Get the current user's home directory.
//----------------------------------------------------------------------------

std::string ts::UserHomeDirectory() throw(ts::Exception)
{
#if defined(__windows)

    ::HANDLE process = 0;
    if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &process) == 0) {
        throw ts::Exception("cannot open current process", ::GetLastError());
    }
    char name[2048];
    ::DWORD length = sizeof(name);
    const ::BOOL status = ::GetUserProfileDirectory(process, name, &length);
    const ::DWORD error = ::GetLastError();
    ::CloseHandle (process);
    if (status == 0) {
        throw ts::Exception("error getting user profile directory", ::GetLastError());
    }
    name[std::max<::DWORD>(0, std::min<::DWORD>(length, sizeof (name) - 1))] = '\0';
    return std::string(name);

#else

    return GetEnvironment("HOME");

#endif
}


//----------------------------------------------------------------------------
// Return the name of the current application executable file.
//----------------------------------------------------------------------------

std::string ts::ExecutableFile() throw(ts::Exception)
{
#if defined(__windows)

    // Window implementation.
    char name[1024];
    ::DWORD length = ::GetModuleFileName(NULL, name, sizeof(name));
    assert(length >= 0 && length <= sizeof(name));
    return std::string(name, length);

#elif defined(__linux)

    // Linux implementation.
    // /proc/self/exe is a symbolic link to the executable.
    // Read the value of the symbolic link.
    int length;
    char name[1024];
    if ((length = ::readlink("/proc/self/exe", name, sizeof(name))) < 0) {
        throw ts::Exception("Symbolic link /proc/self/exe error", errno);
        return "";
    }
    else {
        assert(length <= int(sizeof(name)));
        return std::string(name, length);
    }

#elif defined(__mac)

    // MacOS implementation.
    // The function proc_pidpath is documented as "private" and "subject to change".
    // Another option is _NSGetExecutablePath (not tested here yet).
    int length;
    char name[PROC_PIDPATHINFO_MAXSIZE];
    if ((length = ::proc_pidpath(getpid(), name, sizeof(name))) < 0) {
        throw ts::Exception("proc_pidpath error", errno);
        return "";
    }
    else {
        assert(length <= int(sizeof(name)));
        return std::string(name, length);
    }
    
    
#else
#error "ts::ExecutableFile not implemented on this system"
#endif
}


//----------------------------------------------------------------------------
// Return the current hostname
//----------------------------------------------------------------------------

std::string ts::HostName() throw(ts::Exception)
{
#if defined (__windows)

    // Window implementation.
    char name [1024];
    ::DWORD length (sizeof(name));
    if (::GetComputerName (name, &length) == 0) {
        throw ts::Exception ("GetComputerName error", ::GetLastError ());
        // return ""; // unreachable code
    }
    else {
        assert (length >= 0 && length <= sizeof(name));
        return std::string (name, length);
    }

#else

    // POSIX implementation.
    char name [1024];
    if (::gethostname (name, sizeof(name)) < 0)
        return "";
    else {
        name [sizeof(name) - 1] = '\0';
        return name;
    }

#endif
}


//----------------------------------------------------------------------------
// Suspend the current thread for the specified period
//----------------------------------------------------------------------------

void ts::SleepThread(MilliSecond delay) throw(ts::Exception)
{
#if defined (__windows)

    // Window implementation.
    ::Sleep (::DWORD (delay));

#else

    // POSIX implementation.
    ::timespec requested, remain;
    requested.tv_sec = time_t (delay / 1000); // seconds
    requested.tv_nsec = long ((delay % 1000) * 1000000); // nanoseconds
    while (::nanosleep (&requested, &remain) < 0) {
        if (errno == EINTR) {
            // Interrupted by a signal. Wait again.
            requested = remain;
        }
        else {
            // Actual error
            throw ts::Exception ("nanosleep error", errno);
            break;
        }
    }

#endif
}


//----------------------------------------------------------------------------
// Get system memory page size
//----------------------------------------------------------------------------

size_t ts::MemoryPageSize() throw(ts::Exception)
{
#if defined (__windows)

    ::SYSTEM_INFO sysinfo;
    ::GetSystemInfo (&sysinfo);
    return size_t (sysinfo.dwPageSize);

#else

    // POSIX implementation.
    long size (::sysconf (_SC_PAGESIZE));
    if (size < 0) {
        throw ts::Exception ("sysconf (page size) error", errno);
        size = 0;
    }
    return size_t (size);

#endif
}


//----------------------------------------------------------------------------
// Get current process id
//----------------------------------------------------------------------------

ts::ProcessId ts::CurrentProcessId() throw(ts::Exception)
{
#if defined(__windows)
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}


//----------------------------------------------------------------------------
// Create a directory
//----------------------------------------------------------------------------

ts::ErrorCode ts::CreateDirectory(const std::string& path)
{
#if defined(__windows)
    return ::CreateDirectory(path.c_str(), NULL) == 0 ? ::GetLastError() : SYS_SUCCESS;
#else
    return ::mkdir(path.c_str(), 0777) < 0 ? errno : SYS_SUCCESS;
#endif
}


//----------------------------------------------------------------------------
// Return the name of a directory for temporary files.
//----------------------------------------------------------------------------

std::string ts::TempDirectory ()
{
#if defined(__windows)
    char buf[2048];
    ::DWORD status = ::GetTempPath(::DWORD(sizeof(buf)), buf);
    if (status <= 0) {
        return "C:"; // Fallback name
    }
    else {
        buf[sizeof(buf) - 1];
        return buf;
    }
#else
    return "/tmp";
#endif
}


//----------------------------------------------------------------------------
// Return the name of a unique temporary file name.
//----------------------------------------------------------------------------

std::string ts::TempFile(const std::string& suffix)
{
    return TempDirectory() + PathSeparator +
        Format("tsduck-tmp-%016" FMT_INT64 "X", UID::Instance()->newUID()) +
        suffix;
}


//----------------------------------------------------------------------------
// Get the size in byte of a file. Return -1 in case of error.
//----------------------------------------------------------------------------

int64_t ts::GetFileSize (const std::string& path)
{
#if defined (__windows)
    ::WIN32_FILE_ATTRIBUTE_DATA info;
    return ::GetFileAttributesEx (path.c_str(), ::GetFileExInfoStandard, &info) == 0 ? -1 :
        (int64_t (info.nFileSizeHigh) << 32) | (int64_t (info.nFileSizeLow) & 0xFFFFFFFFL);
#else
    struct stat st;
    return ::stat (path.c_str(), &st) < 0 ? -1 : int64_t (st.st_size);
#endif
}


//----------------------------------------------------------------------------
// Get the time of last modification of a file.
// Return Time::Epoch in case of error.
//----------------------------------------------------------------------------

ts::Time ts::GetFileModificationTimeUTC (const std::string& path)
{
#if defined (__windows)
    ::WIN32_FILE_ATTRIBUTE_DATA info;
    return ::GetFileAttributesEx (path.c_str(), ::GetFileExInfoStandard, &info) == 0 ? Time::Epoch : Time::Win32FileTimeToUTC (info.ftLastWriteTime);
#else
    struct stat st;
    return ::stat (path.c_str(), &st) < 0 ? Time::Epoch : Time::UnixTimeToUTC (st.st_mtime);
#endif
}

ts::Time ts::GetFileModificationTimeLocal (const std::string& path)
{
    const Time time (GetFileModificationTimeUTC (path));
    return time == Time::Epoch ? time : time.UTCToLocal();
}


//----------------------------------------------------------------------------
// Check if a file or directory exists
//----------------------------------------------------------------------------

bool ts::FileExists (const std::string& path)
{
#if defined (__windows)
    return ::GetFileAttributes (path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    return ::access (path.c_str(), F_OK) == 0;
#endif
}


//----------------------------------------------------------------------------
// Check if a path exists and is a directory
//----------------------------------------------------------------------------

bool ts::IsDirectory(const std::string& path)
{
#if defined (__windows)
    const ::DWORD attr = ::GetFileAttributes (path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    return ::stat (path.c_str(), &st) == 0 && S_ISDIR (st.st_mode);
#endif
}


//----------------------------------------------------------------------------
// Delete a file. Return an error code.
//----------------------------------------------------------------------------

ts::ErrorCode ts::DeleteFile(const std::string& path)
{
#if defined (__windows)
    if (IsDirectory(path)) {
        return ::RemoveDirectory(path.c_str()) == 0 ? ::GetLastError() : SYS_SUCCESS;
    }
    else {
        return ::DeleteFile(path.c_str()) == 0 ? ::GetLastError() : SYS_SUCCESS;
    }
#else
    return ::remove(path.c_str()) < 0 ? errno : SYS_SUCCESS;
#endif
}


//----------------------------------------------------------------------------
// Truncate a file to the specified size. Return an error code.
//----------------------------------------------------------------------------

ts::ErrorCode ts::TruncateFile (const std::string& path, uint64_t size)
{
#if defined (__windows)

    ::LONG size_high = ::LONG (size >> 32);
    ::DWORD status = ERROR_SUCCESS;
    ::HANDLE h = ::CreateFile (path.c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) {
        return ::GetLastError();
    }
    else if (::SetFilePointer (h, ::LONG (size), &size_high, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        status = ::GetLastError();
    }
    else if (::SetEndOfFile (h) == 0) {
        status = ::GetLastError();
    }
    ::CloseHandle (h);
    return status;

#else

    return ::truncate (path.c_str(), off_t (size)) < 0 ? errno : 0;

#endif
}


//----------------------------------------------------------------------------
// Rename / move a file. Return an error code.
// Not guaranteed to work across volumes or file systems.
//----------------------------------------------------------------------------

ts::ErrorCode ts::RenameFile (const std::string& old_path, const std::string& new_path)
{
#if defined (__windows)
    return ::MoveFile (old_path.c_str(), new_path.c_str()) == 0 ? ::GetLastError() : ERROR_SUCCESS;
#else
    return ::rename (old_path.c_str(), new_path.c_str()) < 0 ? errno : 0;
#endif
}


//----------------------------------------------------------------------------
// Format an error code into a string
//----------------------------------------------------------------------------

std::string ts::ErrorCodeMessage (ts::ErrorCode code)
{
    char message [1024];
    char* result;
    bool found;

#if defined (__windows)
    // Windows implementation
    ::DWORD length = ::FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, message, sizeof(message), NULL);
    found = length > 0;
    result = message;
#elif HAVE_INT_STRERROR_R
    // POSIX version, strerror_r returns int
    found = 0 == strerror_r (code, message, sizeof(message));
    result = message;
#else
    // GNU version, strerror_r returns char*, not necessarilly in buffer
    result = strerror_r (code, message, sizeof(message));
    found = result != NULL;
#endif

    if (!found) {
        return Format ("System error %d (0x%08X)", code, code);
    }
    else {
        // Make sure message is nul-terminated.
        message [sizeof(message) - 1] = 0;
        // Remove trailing newlines (if any)
        for (size_t i = ::strlen (result); i > 0 && (result[i-1] == '\n' || result[i-1] == '\r'); result[--i] = 0) {}
        return result;
    }
}


//----------------------------------------------------------------------------
// Get metrics for the current process
//----------------------------------------------------------------------------

void ts::GetProcessMetrics(ProcessMetrics& metrics) throw(ts::Exception)
{
    metrics.cpu_time = 0;
    metrics.vmem_size = 0;

#if defined(__windows)

    // Windows implementation

    // Get a handle to the current process
    ::HANDLE proc(::GetCurrentProcess());

    // Get process CPU time
    ::FILETIME creation_time, exit_time, kernel_time, user_time;
    if (::GetProcessTimes(proc, &creation_time, &exit_time, &kernel_time, &user_time) == 0) {
        throw ts::Exception("GetProcessTimes error", ::GetLastError());
    }
    metrics.cpu_time = ts::Time::Win32FileTimeToMilliSecond(kernel_time) + ts::Time::Win32FileTimeToMilliSecond(user_time);

    // Get virtual memory size
    ::PROCESS_MEMORY_COUNTERS_EX mem_counters;
    if (::GetProcessMemoryInfo(proc, (::PROCESS_MEMORY_COUNTERS*)&mem_counters, sizeof(mem_counters)) == 0) {
        throw ts::Exception("GetProcessMemoryInfo error", ::GetLastError());
    }
    metrics.vmem_size = mem_counters.PrivateUsage;

#elif defined (__linux)

    // Linux implementation.

    //  Definition of data available from /proc/<pid>/stat
    //  See man page for proc(5) for more details.
    struct ProcessStatus {
        int           pid;       // The process id.
        char          state;     // One char from "RSDZTW"
        int           ppid;      // The PID of the parent.
        int           pgrp;      // The process group ID of the process.
        int           session;   // The session ID of the process.
        int           tty_nr;    // The tty the process uses.
        int           tpgid;     // Process group ID which owns the tty
        unsigned long flags;     // The flags of the process.
        unsigned long minflt;    // Minor faults the process made
        unsigned long cminflt;   // Minor faults the process's children made
        unsigned long majflt;    // Major faults the process made
        unsigned long cmajflt;   // Major faults the process's children made
        unsigned long utime;     // Number of jiffies in user mode
        unsigned long stime;     // Number of jiffies in kernel mode
        long          cutime;    // Jiffies process's children in user mode
        long          cstime;    // Jiffies process's children in kernel mode
        long          priority;  // Standard nice value, plus fifteen.
        long          nice;      // Nice value, from 19 (nicest) to -19 (not nice)
        long          itrealvalue; // Jiffies before the next SIGALRM
        unsigned long starttime; // Jiffies the process started after system boot
        unsigned long vsize;     // Virtual memory size in bytes.
        long          rss;       // Resident Set Size
        unsigned long rlim;      // Current limit in bytes on the rss
        unsigned long startcode; // Address above which program text can run.
        unsigned long endcode;   // Address below which program text can run.
        unsigned long startstack;// Address of the start of the stack
        unsigned long kstkesp;   // Current value of esp (stack pointer)
        unsigned long kstkeip;   // Current EIP (instruction pointer).
        unsigned long signal;    // Bitmap of pending signals (usually 0).
        unsigned long blocked;   // Bitmap of blocked signals
        unsigned long sigignore; // Bitmap of ignored signals.
        unsigned long sigcatch;  // Bitmap of catched signals.
        unsigned long wchan;     // "Channel" in which the process is waiting
        unsigned long nswap;     // Number of pages swapped - not maintained.
        unsigned long cnswap;    // Cumulative nswap for child processes.
        int           exit_signal; // Signal to be sent to parent when we die.
        int           processor;   // CPU number last executed on.
    };

    static const char filename[] = "/proc/self/stat";
    FILE* fp = fopen(filename, "r");
    if (fp == 0) {
        throw ts::Exception(Format("error opening %s", filename), errno);
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
        throw ts::Exception(Format("error reading %s, got %d values, expected %d", filename, count, expected));
        return;
    }

    // Get virtual memory size
    metrics.vmem_size = ps.vsize;

    // Evaluate CPU time
    unsigned long jps = sysconf(_SC_CLK_TCK);   // jiffies per second
    unsigned long jiffies = ps.utime + ps.stime; // CPU time in jiffies
    metrics.cpu_time = (MilliSecond(jiffies) * 1000) / jps;

#elif defined(__mac)

    // MacOS implementation.
    // First, get the virtual memory size using task_info (mach kernel).
    ::mach_task_basic_info_data_t taskinfo;
    TS_ZERO(taskinfo);
    ::mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    const ::kern_return_t status1 = ::task_info(::mach_task_self(), MACH_TASK_BASIC_INFO, ::task_info_t(&taskinfo), &count);
    if (status1 != KERN_SUCCESS) {
        throw ts::Exception("task_info error");
        return;
    }
    metrics.vmem_size = taskinfo.virtual_size;

    // Then get CPU time using getrusage.
    ::rusage usage;
    const int status2 = ::getrusage(RUSAGE_SELF, &usage);
    if (status2 < 0) {
        throw ts::Exception("getrusage error");
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
#if !defined (__windows)
    ::signal (SIGPIPE, SIG_IGN);
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

bool ts::SetBinaryModeStdin (ReportInterface& report)
{
#if defined (__windows)
    report.debug ("setting standard input to binary mode");
    if (::_setmode (_fileno (stdin), _O_BINARY) < 0) {
        report.error ("cannot set standard input to binary mode");
        Args* args = dynamic_cast<Args*> (&report);
        if (args != 0) {
            args->exitOnError ();
        }
        return false;
    }
#endif
    return true;
}

bool ts::SetBinaryModeStdout (ReportInterface& report)
{
#if defined (__windows)
    report.debug ("setting standard output to binary mode");
    if (::_setmode (_fileno (stdout), _O_BINARY) < 0) {
        report.error ("cannot set standard output to binary mode");
        Args* args = dynamic_cast<Args*> (&report);
        if (args != 0) {
            args->exitOnError ();
        }
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Check if an environment variable exists
//----------------------------------------------------------------------------

bool ts::EnvironmentExists (const std::string& name)
{
    Guard lock(_environmentMutex);

#if defined(__windows)
    char unused[2];
    return ::GetEnvironmentVariable(name.c_str(), unused, sizeof(unused)) != 0;
#else
    return ::getenv(name.c_str()) != 0;
#endif
}


//----------------------------------------------------------------------------
// Get the value of an environment variable.
// Return default value if does not exist.
//----------------------------------------------------------------------------

std::string ts::GetEnvironment (const std::string& name, const std::string& def)
{
    Guard lock(_environmentMutex);

#if defined(__windows)
    std::vector<char> value;
    value.resize(512);
    ::DWORD size = ::GetEnvironmentVariable(name.c_str(), &value[0], ::DWORD(value.size()));
    if (size >= ::DWORD(value.size())) {
        value.resize(size_t(size + 1));
        size = ::GetEnvironmentVariable(name.c_str(), &value[0], ::DWORD(value.size()));
    }
    return size <= 0 ? def : std::string(&value[0], std::min<size_t>(size, value.size()));
#else
    const char* value = ::getenv(name.c_str());
    return value != 0 ? value : def;
#endif
}


//----------------------------------------------------------------------------
// Set the value of an environment variable.
//----------------------------------------------------------------------------

bool ts::SetEnvironment(const std::string& name, const std::string& value)
{
    Guard lock(_environmentMutex);

#if defined(__windows)
    return ::SetEnvironmentVariable(name.c_str(), value.c_str()) != 0;
#else
    // In case of error, setenv(3) is documented to return -1 but not setting errno.
    return ::setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}


//----------------------------------------------------------------------------
// Delete an environment variable.
//----------------------------------------------------------------------------

bool ts::DeleteEnvironment(const std::string& name)
{
    Guard lock(_environmentMutex);

#if defined(__windows)
    return ::SetEnvironmentVariable(name.c_str(), NULL) != 0;
#else
    // In case of error, unsetenv(3) is documented to return -1 but and set errno.
    // It is also documented to silently ignore non-existing variables.
    return ::unsetenv(name.c_str()) == 0;
#endif
}


//----------------------------------------------------------------------------
// Expand environment variables inside a file path (or any string).
// Environment variable references are '$name' or '${name}'.
// In the first form, 'name' is the longest combination of letters, digits and underscore.
// A combination \$ is interpreted as a literal $, not an environment variable reference.
//----------------------------------------------------------------------------

std::string ts::ExpandEnvironment (const std::string& path)
{
    const size_t len (path.length());
    std::string expanded ("");
    expanded.reserve (2 * len);
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
            std::string varname;
            if (++index < len) {
                if (path[index] == '{') {
                    // '${name}' format
                    const size_t last = path.find ('}', index);
                    if (last == std::string::npos) {
                        varname = path.substr (index + 1);
                        index = len;
                    }
                    else {
                        varname = path.substr (index + 1, last - index - 1);
                        index = last + 1;
                    }
                }
                else {
                    // '$name' format
                    const size_t last = path.find_first_not_of ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", index);
                    if (last == std::string::npos) {
                        varname = path.substr (index);
                        index = len;
                    }
                    else {
                        varname = path.substr (index, last - index);
                        index = last;
                    }
                }
            }
            // Second, replace environment variable
            expanded += GetEnvironment (varname);
        }
    }
    return expanded;
}


//----------------------------------------------------------------------------
// Add a "name=value" string to a container
//----------------------------------------------------------------------------

namespace {
    void AddNameValue(ts::Environment& env, const std::string& s)
    {
        const size_t pos = s.find("=");
        if (pos == std::string::npos) {
            // No "=", empty value
            env.insert(std::make_pair(s, ""));
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
    Guard lock(_environmentMutex);
    env.clear();

#if defined(__windows)

    const ::LPTCH strings = ::GetEnvironmentStrings();
    if (strings != 0) {
        size_t len;
        for (char* p = strings; (len = ::strlen(p)) != 0; p += len + 1) {
            AddNameValue(env, std::string(p, len));
        }
        ::FreeEnvironmentStrings(strings);
    }

#else

    for (char** p = ::environ; *p != 0; ++p) {
        AddNameValue(env, *p);
    }

#endif
}
