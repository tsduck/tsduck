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

//!
//! Default separator in CSV (comma-separated values) format.
//! CSV files are suitable for analysis using tools such as Microsoft Excel.
//!
#define TS_DEFAULT_CSV_SEPARATOR u","

namespace ts {
    //!
    //! Separator character in search paths.
    //!
#if defined(DOXYGEN)
    const char SearchPathSeparator = platform-specific (':', ';'); // for doc only
#elif defined(TS_WINDOWS)
    const UChar SearchPathSeparator = u';';
#elif defined(TS_UNIX)
    const UChar SearchPathSeparator = u':';
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Integer type for operating system error codes.
    //!
#if defined(DOXYGEN)
    typedef platform_specific SysErrorCode;
#elif defined(TS_WINDOWS)
    typedef ::DWORD SysErrorCode;
#else
    typedef int SysErrorCode;
#endif

    //!
    //! A SysErrorCode value indicating success.
    //! It is not guaranteed that this value is the @e only success value.
    //! Operating system calls which complete successfully may also return
    //! other values.
    //!
#if defined(DOXYGEN)
    const SysErrorCode SYS_SUCCESS = platform_specific;
#elif defined(TS_WINDOWS)
    const SysErrorCode SYS_SUCCESS = ERROR_SUCCESS;
#elif defined(TS_UNIX)
    const SysErrorCode SYS_SUCCESS = 0;
#else
    #error "Unsupported operating system"
#endif

    //!
    //! A SysErrorCode value indicating a generic data error.
    //! This value can be used to initialize an error code to some generic
    //! error code indicating that a data is not yet available or an
    //! operation is not yet performed.
    //!
#if defined(DOXYGEN)
    const SysErrorCode SYS_DATA_ERROR = platform_specific;
#elif defined(TS_WINDOWS)
    const SysErrorCode SYS_DATA_ERROR = ERROR_INVALID_DATA;
#elif defined(TS_UNIX)
    const SysErrorCode SYS_DATA_ERROR = EINVAL;
#else
    #error "Unsupported operating system"
#endif

    //!
    //! Get the error code of the last operating system call.
    //! The validity of the returned value may depends on specific conditions.
    //! @return The error code of the last operating system call.
    //!
    TSDUCKDLL inline SysErrorCode LastSysErrorCode()
    {
#if defined(TS_WINDOWS)
        return ::GetLastError();
#elif defined(TS_UNIX)
        return errno;
#else
        #error "Unsupported operating system"
#endif
    }

    //!
    //! Format an error code into a string.
    //!
    //! @param [in] code An error code from the operating system.
    //! Typically a result from LastSysErrorCode().
    //! @return A string describing the error.
    //!
    TSDUCKDLL UString SysErrorCodeMessage(SysErrorCode code = LastSysErrorCode());

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
    TSDUCKDLL UString ExecutableFile();

    //!
    //! Get the name of the executable or shared library file containing the caller code.
    //! @return The full path of the file or empty in case of error or if not supported.
    //!
    TSDUCKDLL UString CallerLibraryFile();

    //!
    //! Suspend the current thread for the specified period.
    //! @param [in] delay Number of milliseconds to sleep the current thread.
    //!
    TSDUCKDLL void SleepThread(MilliSecond delay);

    //!
    //! Integer type for process identifier
    //!
#if defined(DOXYGEN)
    typedef platform_specific ProcessId;
#elif defined(TS_WINDOWS)
    typedef ::DWORD ProcessId;
#elif defined(TS_UNIX)
    typedef pid_t ProcessId;
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Get the current process id.
    //! @return Identification of the current process.
    //!
    TSDUCKDLL ProcessId CurrentProcessId();

    //!
    //! Check if the current user is privileged (root on UNIX, an administrator on Windows).
    //! @return True if the current user is privileged.
    //!
    TSDUCKDLL bool IsPrivilegedUser();

    //!
    //! Check if an environment variable exists.
    //! @param [in] varname Environment variable name.
    //! @return True if the specified environment variable exists, false otherwise.
    //!
    TSDUCKDLL bool EnvironmentExists(const UString& varname);

    //!
    //! Get the value of an environment variable.
    //! @param [in] varname Environment variable name.
    //! @param [in] defvalue Default value if the specified environment variable does not exist.
    //! @return The value of the specified environment variable it it exists, @a defvalue otherwise.
    //!
    TSDUCKDLL UString GetEnvironment(const UString& varname, const UString& defvalue = UString());

    //!
    //! Get the value of an environment variable containing a search path.
    //!
    //! The search path is analyzed and split into individual directory names.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [out] container A container of @c UString receiving the directory names.
    //! @param [in] name Environment variable name.
    //! @param [in] def Default value if the specified environment variable does not exist.
    //!
    template <class CONTAINER>
    inline void GetEnvironmentPath(CONTAINER& container, const UString& name, const UString& def = UString())
    {
        GetEnvironment(name, def).split(container, SearchPathSeparator, true, true);
    }

    //!
    //! Get the value of an environment variable containing a search path.
    //!
    //! The search path is analyzed and split into individual directory names.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c UString receiving the directory names.
    //! The directory names are appended to the container without erasing previous content.
    //! @param [in] name Environment variable name.
    //! @param [in] def Default value if the specified environment variable does not exist.
    //!
    template <class CONTAINER>
    inline void GetEnvironmentPathAppend(CONTAINER& container, const UString& name, const UString& def = UString())
    {
        GetEnvironment(name, def).splitAppend(container, SearchPathSeparator, true, true);
    }

    //!
    //! Set the value of an environment variable.
    //!
    //! If the variable previously existed, its value is overwritten.
    //! If it did not exist, it is created.
    //!
    //! @param [in] name Environment variable name.
    //! @param [in] value Environment variable value.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SetEnvironment(const UString& name, const UString& value);

    //!
    //! Set the value of an environment variable containing a search path.
    //!
    //! If the variable previously existed, its value is overwritten.
    //! If it did not exist, it is created.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [in] name Environment variable name.
    //! @param [in] container A container of @c UString containing directory names.
    //!
    template <class CONTAINER>
    inline void SetEnvironmentPath(const UString& name, const CONTAINER& container)
    {
        SetEnvironment(name, UString::Join(container, UString(1, SearchPathSeparator)));
    }

    //!
    //! Delete an environment variable.
    //!
    //! If the variable did not exist, do nothing, do not generate an error.
    //!
    //! @param [in] name Environment variable name.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteEnvironment(const UString& name);

    //!
    //! Expand environment variables inside a file path (or any string).
    //!
    //! Environment variable references '$name' or '${name}' are replaced
    //! by the corresponding values from the environment.
    //! In the first form, 'name' is the longest combination of letters, digits and underscore.
    //! A combination \\$ is interpreted as a literal $, not an environment variable reference.
    //!
    //! @param [in] path A path string containing references to environment variables.
    //! @return The expanded string.
    //!
    TSDUCKDLL UString ExpandEnvironment(const UString& path);

    //!
    //! Define a container type holding all environment variables.
    //!
    //! For each element in the container, the @e key is the name of an
    //! environment variable and the @e value is the corresponding value
    //! of this environment variable.
    //!
    typedef std::map<UString, UString> Environment;

    //!
    //! Get the content of the entire environment (all environment variables).
    //!
    //! @param [out] env An associative container which receives the content
    //! of the environment. Each @e key is the name of an environment variable
    //! and the corresponding @e value is the value of this environment variable.
    //!
    TSDUCKDLL void GetEnvironment(Environment& env);

    //!
    //! Load a text file containing environment variables.
    //! Each line shall be in the form "name = value".
    //! Empty line and line starting with '#' are ignored.
    //! Spaces are trimmed.
    //!
    //! @param [out] env An associative container which receives the content of the environment.
    //! Each @e key is the name of an environment variable and the corresponding @e value is
    //! the value of this environment variable.
    //! @param [in] fileName Name of the file to load.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool LoadEnvironment(Environment& env, const UString& fileName);

    //!
    //! This structure contains metrics about a process
    //!
    struct TSDUCKDLL ProcessMetrics
    {
        MilliSecond cpu_time;    //!< CPU time of the process in milliseconds.
        size_t      vmem_size;   //!< Virtual memory size in bytes.

        //!
        //! Default constructor.
        //!
        ProcessMetrics() : cpu_time(-1), vmem_size(0) {}
    };

    //!
    //! Get metrics for the current process
    //! @param [out] metrics Receive the current process metrics.
    //! @throw ts::Exception on error.
    //!
    TSDUCKDLL void GetProcessMetrics(ProcessMetrics& metrics);

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
    //! If @a report is a subclass of ts::Args, also terminate application.
    //! @return True on success, false on error.
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
    //! If @a report is a subclass of ts::Args, also terminate application.
    //! @return True on success, false on error.
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
