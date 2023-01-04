//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsFileUtils.h"
#include "tsStaticInstance.h"
#include "tsMutex.h"
#include "tsGuardMutex.h"
#include "tsTime.h"
#include "tsArgs.h"

#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
    #include "tsBeforeStandardHeaders.h"
    #include <intrin.h>
    #include <io.h>
    #include <psapi.h>
    #include "tsAfterStandardHeaders.h"
#elif defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <dlfcn.h>
    #include "tsAfterStandardHeaders.h"
#elif defined(TS_MAC)
    #include "tsBeforeStandardHeaders.h"
    #include <sys/resource.h>
    #include <mach/mach.h>
    #include <mach/message.h>
    #include <mach/kern_return.h>
    #include <mach/task_info.h>
    #include <libproc.h>
    #include <dlfcn.h>
    #include "tsAfterStandardHeaders.h"
    extern char **environ; // not defined in public headers
#elif defined(TS_BSD)
    #include "tsSysCtl.h"
    #include "tsBeforeStandardHeaders.h"
    #include <sys/user.h>
    #include <sys/resource.h>
    #include <kvm.h>
    #include <signal.h>
    #include <dlfcn.h>
    #if defined(TS_FREEBSD)
        #include <libprocstat.h>
    #elif defined(TS_DRAGONFLYBSD)
        #include <sys/kinfo.h>
    #endif
    #include "tsAfterStandardHeaders.h"
    extern char **environ; // not defined in public headers
#endif

// Required link libraries under Windows.
#if defined(TS_WINDOWS) && defined(TS_MSC)
    #pragma comment(lib, "psapi.lib")  // GetProcessMemoryInfo
#endif

// External calls to environment variables are not reentrant. Use a global mutex.
TS_STATIC_INSTANCE(ts::Mutex, (), EnvironmentMutex)


//----------------------------------------------------------------------------
// Return the name of the current application executable file.
//----------------------------------------------------------------------------

ts::UString ts::ExecutableFile()
{
    UString path;

#if defined(TS_WINDOWS)

    // Window implementation.
    std::array<::WCHAR, 2048> name;
    ::DWORD length = ::GetModuleFileNameW(nullptr, name.data(), ::DWORD(name.size()));
    path = UString(name, length);

#elif defined(TS_LINUX)

    // Linux implementation.
    // /proc/self/exe is a symbolic link to the executable.
    path = ResolveSymbolicLinks(u"/proc/self/exe");

#elif defined(TS_MAC)

    // MacOS implementation.
    // The function proc_pidpath is documented as "private" and "subject to change".
    // Another option is _NSGetExecutablePath (not tested here yet).
    int length = 0;
    char name[PROC_PIDPATHINFO_MAXSIZE];
    if ((length = ::proc_pidpath(getpid(), name, sizeof(name))) < 0) {
        throw ts::Exception(u"proc_pidpath error", errno);
    }
    else {
        assert(length <= int(sizeof(name)));
        path.assignFromUTF8(name, length);
    }

#elif defined(TS_FREEBSD) || defined(TS_DRAGONFLYBSD)

    // FreeBSD and DragonFlyBSD implementation.
    // We use the sysctl() MIB and the OID for the current executable is:
    path = SysCtrlString({CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1}); // -1 means current process

#elif defined(TS_NETBSD)

    // NetBSD implementation.
    // We use the sysctl() MIB and the OID for the current executable is:
    path = SysCtrlString({CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME}); // -1 means current process

#elif defined(TS_OPENBSD)

    // OpenBSD implementation.
    // OpenBSD is the only OS without supported interface to get the current executable path,
    // giving invalid so-called "security reasons" for that. So, we try to guess it from the
    // original argv[0]. This is much less secure than having a supported interface. This is
    // why their "security reasons" are particularly stupid IMHO.

    ByteBlock argv_data(SysCtrlBytes({CTL_KERN, KERN_PROC_ARGS, ::getpid(), KERN_PROC_ARGV}));
    if (argv_data.size() < sizeof(char*)) {
        return UString();
    }
    char** argv = reinterpret_cast<char**>(argv_data.data());
    char* exe = argv[0];
    if (exe == nullptr) {
        return UString();
    }
    if (::strchr(exe, '/') != nullptr) {
        // A path is provided, resolve it.
        char* path8 = ::realpath(exe, nullptr);
        if (path8 != nullptr) {
            path.assignFromUTF8(path8);
            ::free(path8);
        }
    }
    else {
        // A simple command name is provided, find it in the PATH.
        path = SearchExecutableFile(UString::FromUTF8(exe));
    }

#else
#error "ts::ExecutableFile not implemented on this system"
#endif

    return path.empty() ? path : AbsoluteFilePath(path);
}


//----------------------------------------------------------------------------
//! Get the name of the executable or shared library containing the caller.
//----------------------------------------------------------------------------

ts::UString ts::CallerLibraryFile()
{
#if defined(TSDUCK_STATIC)

    // In case of static build, there is no shared library.
    // All code is in the main executable.
    return ExecutableFile();

#elif defined(TS_MSC)

    // Window implementation.
    // Get return address of current function (in caller code).
    void* const ret = _ReturnAddress();
    // Get the module (DLL) into which this address can be found.
    ::HMODULE handle = nullptr;
    if (::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, ::LPCWSTR(ret), &handle) == 0) {
        return UString();
    }
    else {
        std::array<::WCHAR, 2048> name;
        ::DWORD length = ::GetModuleFileNameW(handle, name.data(), ::DWORD(name.size()));
        return UString(name, length);
    }

#elif defined(TS_GCC) || defined(TS_LLVM)

    // GCC and LLVM/clang implementation.
    // Get return address of current function (in caller code).
    void* const ret = __builtin_return_address(0);
    // Get the shared library into which this address can be found.
    ::Dl_info info;
    TS_ZERO(info);
    if (ret != nullptr && ::dladdr(ret, &info) != 0 && info.dli_fname != nullptr) {
        return UString::FromUTF8(info.dli_fname);
    }
    else {
        return UString();
    }

#else
    #error "ts::CallerLibraryFile not implemented on this system"
#endif
}


//----------------------------------------------------------------------------
// Suspend the current thread for the specified period
//----------------------------------------------------------------------------

void ts::SleepThread(MilliSecond delay)
{
    if (delay > 0) {
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
ts::UString ts::SysErrorCodeMessage(ts::SysErrorCode code)
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
    const ::HANDLE proc = ::GetCurrentProcess();

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

    // Definition of data available from /proc/<pid>/stat
    // See man page for proc(5) for more details.
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
    const int expected = 37;
    const int count = fscanf(fp,
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
    unsigned long jps = ::sysconf(_SC_CLK_TCK);   // jiffies per second
    unsigned long jiffies = ps.utime + ps.stime; // CPU time in jiffies
    metrics.cpu_time = (MilliSecond(jiffies) * 1000) / jps;

#elif defined(TS_MAC)

    // macOS implementation.

    // Get the virtual memory size using task_info (mach kernel).
    ::mach_task_basic_info_data_t taskinfo;
    TS_ZERO(taskinfo);
    ::mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    const ::kern_return_t status1 = ::task_info(::mach_task_self(), MACH_TASK_BASIC_INFO, ::task_info_t(&taskinfo), &count);
    if (status1 != KERN_SUCCESS) {
        throw ts::Exception(u"task_info error", errno);
    }
    metrics.vmem_size = taskinfo.virtual_size;

#elif defined(TS_FREEBSD)

    // FreeBSD implementation.

    // Get the virtual memory size using procstat_getprocs() on current process.
    ::procstat* pstat = ::procstat_open_sysctl();
    if (pstat == nullptr) {
        throw ts::Exception(u"procstat_open_sysctl error", errno);
    }

    unsigned int kproc_count = 0;
    ::kinfo_proc* kproc = ::procstat_getprocs(pstat, KERN_PROC_PID, ::getpid(), &kproc_count);
    if (kproc == nullptr || kproc_count == 0) {
        throw ts::Exception(u"procstat_getprocs error", errno);
    }
    metrics.vmem_size = kproc->ki_size;

    ::procstat_freeprocs(pstat, kproc);
    ::procstat_close(pstat);

#elif defined(TS_OPENBSD)

    // OpenBSD implementation.

    // Use the kvm library to get the process virtual size.
    ::kvm_t* kvm = ::kvm_open(nullptr, nullptr, nullptr, KVM_NO_FILES, "kvm_open");
    if (kvm == nullptr) {
        throw ts::Exception(u"kvm_open error", errno);
    }

    int count = 0;
    ::kinfo_proc* kinfo = ::kvm_getprocs(kvm, KERN_PROC_PID, ::getpid(), sizeof(::kinfo_proc), &count);
    if (kinfo == nullptr || count == 0) {
        throw ts::Exception(u"kvm_getprocs error", errno);
    }

    // The virtual memory size is text size + data size + stack size.
    // Cannot use p_vm_map_size, it is always zero.
    const long pagesize = ::sysconf(_SC_PAGESIZE);
    metrics.vmem_size = kinfo->p_vm_tsize * pagesize + kinfo->p_vm_dsize * pagesize + kinfo->p_vm_ssize * pagesize;

    ::kvm_close(kvm);

#elif defined(TS_DRAGONFLYBSD)

    // DragonFlyBSD implementation.

    // Similar to OpenBSD but some symbols have different names and kvm_getprocs() has no way
    // to describe the current size of struct kinfo_proc. Moreover, /dev/null must be passed as
    // execfile and corefile. Otherwise, a permission denied error is returned on /dev/mem.
    ::kvm_t* kvm = ::kvm_open("/dev/null", "/dev/null", nullptr, O_RDONLY, "kvm_open");
    if (kvm == nullptr) {
        throw ts::Exception(u"kvm_open error", errno);
    }

    int count = 0;
    ::kinfo_proc* kinfo = ::kvm_getprocs(kvm, KERN_PROC_PID, ::getpid(), &count);
    if (kinfo == nullptr || count == 0) {
        throw ts::Exception(u"kvm_getprocs error", errno);
    }

    // The virtual memory size is directly in kp_vm_map_size, in bytes.
    metrics.vmem_size = kinfo->kp_vm_map_size;

    ::kvm_close(kvm);

#elif defined(TS_NETBSD)

    // NetBSD implementation.

    // Similar to OpenBSD but use struct kinfo_proc2 and kvm_getproc2().
    ::kvm_t* kvm = ::kvm_open(nullptr, nullptr, nullptr, KVM_NO_FILES, "kvm_open");
    if (kvm == nullptr) {
        throw ts::Exception(u"kvm_open error", errno);
    }

    int count = 0;
    ::kinfo_proc2* kinfo = ::kvm_getproc2(kvm, KERN_PROC_PID, ::getpid(), sizeof(::kinfo_proc2), &count);
    if (kinfo == nullptr || count == 0) {
        throw ts::Exception(u"kvm_getprocs error", errno);
    }

    // The virtual memory size is text size + data size + stack size.
    const long pagesize = ::sysconf(_SC_PAGESIZE);
    metrics.vmem_size = kinfo->p_vm_tsize * pagesize + kinfo->p_vm_dsize * pagesize + kinfo->p_vm_ssize * pagesize;

    ::kvm_close(kvm);

#else
    #error "ts::GetProcessMetrics not implemented on this system"
#endif

#if defined(TS_MAC) || defined(TS_BSD)

    // On BSD systems, get CPU time using getrusage().
    ::rusage usage;
    const int status2 = ::getrusage(RUSAGE_SELF, &usage);
    if (status2 < 0) {
        throw ts::Exception(u"getrusage error", errno);
    }

    // Add system time and user time, in milliseconds.
    metrics.cpu_time =
        MilliSecond(usage.ru_stime.tv_sec) * MilliSecPerSec +
        MilliSecond(usage.ru_stime.tv_usec) / MicroSecPerMilliSec +
        MilliSecond(usage.ru_utime.tv_sec) * MilliSecPerSec +
        MilliSecond(usage.ru_utime.tv_usec) / MicroSecPerMilliSec;

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
    GuardMutex lock(EnvironmentMutex::Instance());

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
    GuardMutex lock(EnvironmentMutex::Instance());

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
    GuardMutex lock(EnvironmentMutex::Instance());

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
    GuardMutex lock(EnvironmentMutex::Instance());

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
    GuardMutex lock(EnvironmentMutex::Instance());
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
        for (const auto& it : lines) {
            AddNameValue(env, it, false);
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
