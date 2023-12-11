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
//!  Various system utilities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsCerrReport.h"

#if defined(TS_UNIX)
    #include "tsBeforeStandardHeaders.h"
    #include <cerrno>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <pthread.h>
    #include <sched.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Get the error code of the last operating system call.
    //! The validity of the returned value may depends on specific conditions.
    //!
    //! Portability of error code representation: On UNIX, error codes are just @c int. On Windows, error
    //! codes are @c DWORD, which is compatible with @c int. In C++11, @c std::error_core uses @c int for
    //! error codes. Therefore, because of this new C++11 feature, we just use @c int.
    //!
    //! Windows note: According to Windows documentation, socket functions should call WSAGetLastError()
    //! instead of GetLastError() to retrieve the error code. This is an oddity from the old 16-bit
    //! Windows API. On Win32, various sources confirm that WSAGetLastError() just call GetLastError().
    //! Thus, in this application, we do not make the difference.
    //!
    //! @return The error code of the last operating system call.
    //!
    TSDUCKDLL inline int LastSysErrorCode()
    {
#if defined(TS_WINDOWS)
        return ::GetLastError();
#else
        return errno;
#endif
    }

    //!
    //! Format a system error code into a string.
    //! @param [in] code An error code from the operating system.
    //! Typically a result from @c errno (Unix) or @c GetLastError() (Windows).
    //! @param [in] category Error category, system by default.
    //! @return A string describing the error.
    //!
    TSDUCKDLL inline std::string SysErrorCodeMessage(int code = LastSysErrorCode(), const std::error_category& category = std::system_category())
    {
        return std::error_code(code, category).message();
    }

    //!
    //! Portable type for ioctl() request parameter.
    //!
    #if defined(DOXYGEN)
        typedef platform-dependent ioctl_request_t;
    #elif defined(TS_WINDOWS)
        // Second parameter of ::DeviceIoControl().
        typedef ::DWORD ioctl_request_t;
    #else
        // Extract the type of the second parameter of ::ioctl().
        // It is "unsigned long" on most Linux systems but "int" on Alpine Linux.
        template<typename T>
        T request_param_type(int (*ioctl_syscall)(int, T, ...));
        typedef decltype(request_param_type(&::ioctl)) ioctl_request_t;
    #endif

    //!
    //! Get the name of the current application executable file.
    //! @return The full path of the executable file which is run in the current process.
    //!
    TSDUCKDLL fs::path ExecutableFile();

    //!
    //! Get the name of the executable or shared library file containing the caller code.
    //! @return The full path of the file or empty in case of error or if not supported.
    //!
    TSDUCKDLL fs::path CallerLibraryFile();

    //!
    //! Suspend the current thread for the specified period.
    //! Before enforcing C++11, this function used to be implemented in a system-dependent manner.
    //! Now, it is just an encapsulation of std::this_thread::sleep_for().
    //! @param [in] delay Number of milliseconds to sleep the current thread.
    //!
    TSDUCKDLL inline void SleepThread(MilliSecond delay)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::chrono::milliseconds::rep(delay)));
    }

    //!
    //! Check if the current user is privileged (root on UNIX, an administrator on Windows).
    //! @return True if the current user is privileged.
    //!
    TSDUCKDLL bool IsPrivilegedUser();

    //!
    //! Get the CPU time of the process in milliseconds.
    //! @return The CPU time of the process in milliseconds.
    //! @throw ts::Exception on error.
    //!
    TSDUCKDLL MilliSecond GetProcessCpuTime();

    //!
    //! Get the virtual memory size of the process in bytes.
    //! @return The virtual memory size of the process in bytes.
    //! @throw ts::Exception on error.
    //!
    TSDUCKDLL size_t GetProcessVirtualSize();

    //!
    //! Ensure that writing to a broken pipe does not kill the current process.
    //!
    //! On UNIX systems, writing to a <i>broken pipe</i>, i.e. a pipe with
    //! no process reading from it, kills the current process. This may not
    //! be what you want. This functions prevents this.
    //!
    //! <strong>UNIX Systems:</strong> This function ignores SIGPIPE.
    //! Writing to a broken pipe will now return an error instead of killing
    //! the process.
    //!
    //! <strong>Windows systems:</strong> This function does nothing (because
    //! there is no need to do anything).
    //!
    TSDUCKDLL void IgnorePipeSignal();

    //!
    //! Check if the standard input is a terminal.
    //! @return True if the standard input is a terminal.
    //!
    TSDUCKDLL bool StdInIsTerminal();

    //!
    //! Check if the standard output is a terminal.
    //! @return True if the standard output is a terminal.
    //!
    TSDUCKDLL bool StdOutIsTerminal();

    //!
    //! Check if the standard error is a terminal.
    //! @return True if the standard error is a terminal.
    //!
    TSDUCKDLL bool StdErrIsTerminal();

    //!
    //! Put the standard input stream in binary mode.
    //!
    //! On UNIX systems, this does not make any difference.
    //! On Windows systems, however, in a stream which is not open in
    //! binary mode, there is automatic translation between LF and CR-LF.
    //! The standard input is open in text mode (non-binary).
    //! This function forces it into binary mode.
    //!
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //! If @a report is a subclass of ts::Args, terminate the application on error.
    //! @see SetBinaryModeStdout()
    //!
    TSDUCKDLL bool SetBinaryModeStdin(Report& report = CERR);

    //!
    //! Put the standard output stream in binary mode.
    //!
    //! On UNIX systems, this does not make any difference.
    //! On Windows systems, however, in a stream which is not open in
    //! binary mode, there is automatic translation between LF and CR-LF.
    //! The standard output is open in text mode (non-binary).
    //! This function forces it into binary mode.
    //!
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //! If @a report is a subclass of ts::Args, terminate the application on error.
    //! @see SetBinaryModeStdout()
    //!
    TSDUCKDLL bool SetBinaryModeStdout(Report& report = CERR);

    //!
    //! Get the name of a class from the @c type_info of an object.
    //! The result may be not portable.
    //! @param [in] info The @c type_info of an object.
    //! @return An implementation-specific name of the object class.
    //!
    TSDUCKDLL UString ClassName(const std::type_info& info);
}
