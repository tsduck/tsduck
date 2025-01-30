//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    TSCOREDLL inline int LastSysErrorCode()
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
    TSCOREDLL inline std::string SysErrorCodeMessage(int code = LastSysErrorCode(), const std::error_category& category = std::system_category())
    {
        return std::error_code(code, category).message();
    }

    //!
    //! Portable type for ioctl() request parameter.
    //!
    #if defined(DOXYGEN)
        using ioctl_request_t = platform-dependent;
    #elif defined(TS_WINDOWS)
        // Second parameter of ::DeviceIoControl().
        using ioctl_request_t = ::DWORD;
    #else
        // Extract the type of the second parameter of ::ioctl().
        // It is "unsigned long" on most Linux systems but "int" on Alpine Linux.
        template<typename T>
        T request_param_type(int (*ioctl_syscall)(int, T, ...));
        using ioctl_request_t = decltype(request_param_type(&::ioctl));
    #endif

    //!
    //! Get the name of the current application executable file.
    //! @return The full path of the executable file which is run in the current process.
    //!
    TSCOREDLL fs::path ExecutableFile();

    //!
    //! Get the name of the executable or shared library file containing the caller code.
    //! @return The full path of the file or empty in case of error or if not supported.
    //!
    TSCOREDLL fs::path CallerLibraryFile();

    //!
    //! Check if the current user is privileged (root on UNIX, an administrator on Windows).
    //! @return True if the current user is privileged.
    //!
    TSCOREDLL bool IsPrivilegedUser();

    //!
    //! Get the CPU time of the process in milliseconds.
    //! @return The CPU time of the process in milliseconds.
    //! @throw ts::Exception on error.
    //!
    TSCOREDLL cn::milliseconds GetProcessCpuTime();

    //!
    //! Get the virtual memory size of the process in bytes.
    //! @return The virtual memory size of the process in bytes.
    //! @throw ts::Exception on error.
    //!
    TSCOREDLL size_t GetProcessVirtualSize();

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
    TSCOREDLL void IgnorePipeSignal();

    // Implementation of SetTimersPrecision() using nanoseconds.
    //! @cond nodoxygen
    TSCOREDLL cn::nanoseconds::rep _SetTimersPrecisionNanoSecond(cn::nanoseconds::rep precision);
    //! @endcond

    //!
    //! Request a minimum resolution for the system timers.
    //! @param [in,out] precision On input, specify the requested minimum resolution in any std::chrono::duration units.
    //! On output, return the obtained guaranteed minimum resolution. The guaranteed precision value can be equal to or
    //! greater than the requested value. The default system resolution is 20 ms on Win32, which can be too long for applications.
    //!
    template <class Rep, class Period>
    void SetTimersPrecision(cn::duration<Rep,Period>& precision)
    {
        const cn::nanoseconds::rep ns_in = cn::duration_cast<cn::nanoseconds>(precision).count();
        precision = cn::duration_cast<cn::duration<Rep,Period>>(cn::nanoseconds(_SetTimersPrecisionNanoSecond(ns_in)));
    }

    //!
    //! Check if the standard input is a terminal.
    //! @return True if the standard input is a terminal.
    //!
    TSCOREDLL bool StdInIsTerminal();

    //!
    //! Check if the standard output is a terminal.
    //! @return True if the standard output is a terminal.
    //!
    TSCOREDLL bool StdOutIsTerminal();

    //!
    //! Check if the standard error is a terminal.
    //! @return True if the standard error is a terminal.
    //!
    TSCOREDLL bool StdErrIsTerminal();

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
    TSCOREDLL bool SetBinaryModeStdin(Report& report = CERR);

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
    TSCOREDLL bool SetBinaryModeStdout(Report& report = CERR);

    //!
    //! Get the name of a class from the @c type_index of a class.
    //! The result may be not portable.
    //! @param [in] index The @c type_index of a class.
    //! @return An implementation-specific name of the class.
    //!
    TSCOREDLL UString ClassName(const std::type_index index);

    //!
    //! Get the name of a class from the @c type_info of an object.
    //! The result may be not portable.
    //! @param [in] info The @c type_info of an object.
    //! @return An implementation-specific name of the object class.
    //!
    inline TSCOREDLL UString ClassName(const std::type_info& info)
    {
        return ClassName(std::type_index(info));
    }
}
