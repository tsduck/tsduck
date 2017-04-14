//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//! @file tsSysUtils.h
//!
//!  Various system utilities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsStringUtils.h"
#include "tsException.h"
#include "tsCerrReport.h"

// Executable file suffix.
#if defined(__windows)
    #define TS_EXECUTABLE_SUFFIX ".exe"
#else
    #define TS_EXECUTABLE_SUFFIX ""
#endif

namespace ts {

    // Directory path and search path separator
#if defined(__windows)
    const char PathSeparator = '\\';
    const char SearchPathSeparator = ';';
#else
    const char PathSeparator = '/';
    const char SearchPathSeparator = ':';
#endif

    //!
    //! Return a "vernacular" version of a file path.
    //!
    //! @param [in] path A file path.
    //! @return A copy of @a path where all '/' and '\' have been
    //! translated into the local directory separator.
    //!
    TSDUCKDLL std::string VernacularFilePath(const std::string& path);

    // Return the directory name of a file path (eg. "dir/foo.bar" => "dir")
    TSDUCKDLL std::string DirectoryName(const std::string& path);

    // Return the base name of a file path (eg. "dir/foo.bar" => "foo.bar")
    // Remove specified suffix if present.
    TSDUCKDLL std::string BaseName(const std::string& path, const std::string& suffix = std::string());

    // Return the suffix of a file path (eg. "dir/foo.bar" => ".bar")
    TSDUCKDLL std::string PathSuffix(const std::string& path);

    // If the file path does not contain a suffix, add the specified one.
    // Otherwise, return the name unchanged.
    TSDUCKDLL std::string AddPathSuffix(const std::string& path, const std::string& suffix);

    // Return the prefix of a file path (eg. "dir/foo.bar" => "dir/foo")
    TSDUCKDLL std::string PathPrefix(const std::string& path);

    //!
    //! Get the current user's home directory.
    //!
    //! @return The full path of the current user's home directory.
    //! @throw ts::Exception In case of operating system error.
    //!
    TSDUCKDLL std::string UserHomeDirectory() throw(ts::Exception);

    // Return the name of the current application executable file.
    TSDUCKDLL std::string ExecutableFile() throw(ts::Exception);

    // Return the current hostname
    TSDUCKDLL std::string HostName() throw(ts::Exception);

    // Suspend the current thread for the specified period
    TSDUCKDLL void SleepThread(MilliSecond delay) throw(ts::Exception);

    // Get system memory page size
    TSDUCKDLL size_t MemoryPageSize() throw(ts::Exception);

    // Definition of a process id
#if defined(__windows)
    typedef ::DWORD ProcessId;
#else
    typedef pid_t ProcessId;
#endif

    // Get current process id
    TSDUCKDLL ProcessId CurrentProcessId() throw(ts::Exception);

    //!
    //! Create a directory
    //!
    //! @param [in] path A directory path.
    //! @return A system-specific error code (SYS_SUCCESS on success).
    //!
    TSDUCKDLL ErrorCode CreateDirectory(const std::string& path);

    // Return the name of a directory for temporary files.
    TSDUCKDLL std::string TempDirectory();

    // Return the name of a unique temporary file name.
    TSDUCKDLL std::string TempFile(const std::string& suffix = ".tmp");

    //!
    //! Get the size in bytes of a file.
    //!
    //! @param [in] path A file path.
    //! @return Size in bytes of the file or -1 in case of error.
    //!
    TSDUCKDLL int64_t GetFileSize(const std::string& path);

    // Get the UTC time of last modification of a file. Return Time::Epoch in case of error.
    TSDUCKDLL Time GetFileModificationTimeLocal(const std::string& path);
    TSDUCKDLL Time GetFileModificationTimeUTC(const std::string& path);

    // Check if a file or directory exists
    TSDUCKDLL bool FileExists(const std::string& path);

    // Check if a path exists and is a directory
    TSDUCKDLL bool IsDirectory(const std::string& path);

    //!
    //! Delete a file or directory.
    //!
    //! If the specified path is a directory, it must be empty.
    //! Otherwise, an error is returned.
    //!
    //! @param [in] path A file or directory path.
    //! @return A system-specific error code (SYS_SUCCESS on success).
    //!
    TSDUCKDLL ErrorCode DeleteFile(const std::string& path);

    // Truncate a file to the specified size. Return an error code.
    TSDUCKDLL ErrorCode TruncateFile(const std::string& path, uint64_t size);

    // Rename / move a file. Return an error code.
    // Not guaranteed to work across volumes or file systems.
    TSDUCKDLL ErrorCode RenameFile(const std::string& old_path, const std::string& new_path);

    //!
    //! Get all files matching a specified wildcard pattern and append them into a container.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c std::string receiving the
    //! the names of all files matching the wildcard. The names are appended
    //! at the end of the existing content of the container.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of
    //! the wildcards is system-dependent.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    bool ExpandWildcardAndAppend(CONTAINER& container, const std::string& pattern);

    //!
    //! Get all files matching a specified wildcard pattern.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string receiving the
    //! the names of all files matching the wildcard.
    //! @param [in] pattern A file path pattern with wildcards. The syntax of
    //! the wildcards is system-dependent.
    //! @return True on success, false on error. Note that finding no file matching
    //! the pattern is not an error, it simply return no file name.
    //!
    template <class CONTAINER>
    bool ExpandWildcard(CONTAINER& container, const std::string& pattern)
    {
        container.clear();
        return ExpandWildcardAndAppend(container, pattern);
    }

    // Check if an environment variable exists
    TSDUCKDLL bool EnvironmentExists(const std::string& varname);

    // Get the value of an environment variable.
    // Return default value if does not exist.
    TSDUCKDLL std::string GetEnvironment(const std::string& varname, const std::string& defvalue = "");

    //!
    //! Get the value of an environment variable containing a search path.
    //!
    //! The search path is analyzed and split into individual directory names.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string receiving the
    //! directory names.
    //! @param [in] name Environment variable name.
    //! @param [in] def Default value if the specified environment
    //! variable does not exist.
    //!
    template <class CONTAINER>
    void GetEnvironmentPath(CONTAINER& container, const std::string& name, const std::string& def = "")
    {
        SplitString(container, GetEnvironment(name, def), SearchPathSeparator, true);
        if (container.size() == 1 && container[0].empty()) {
            // Path was actually empty
            container.clear();
        }
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
    TSDUCKDLL bool SetEnvironment(const std::string& name, const std::string& value);

    //!
    //! Delete an environment variable.
    //!
    //! If the variable did not exist, do nothing, do not generate an error.
    //!
    //! @param [in] name Environment variable name.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteEnvironment(const std::string& name);

    // Expand environment variables inside a file path (or any string).
    // Environment variable references are '$name' or '${name}'.
    // In the first form, 'name' is the longest combination of letters, digits and underscore.
    // A combination \$ is interpreted as a literal $, not an environment variable reference.
    TSDUCKDLL std::string ExpandEnvironment(const std::string& path);

    //!
    //! Define a container type holding all environment variables.
    //!
    //! For each element in the container, the @e key is the name of an
    //! environment variable and the @e value is the corresponding value 
    //! of this environment variable.
    //!
    typedef std::map<std::string, std::string> Environment;

    //!
    //! Get the content of the entire environment (all environment variables).
    //!
    //! @param [out] env An associative container which receives the content
    //! of the environment. Each @e key is the name of an environment variable
    //! and the corresponding @e value is the value of this environment variable.
    //!
    TSDUCKDLL void GetEnvironment(Environment& env);

    //!
    //! Format an error code into a string.
    //!
    //! @param [in] code An error code from the operating system.
    //! Typically a result from LastErrorCode().
    //! @return A string describing the error.
    //!
    TSDUCKDLL std::string ErrorCodeMessage(ErrorCode code = LastErrorCode());

    // This structure contains metrics about a process
    struct TSDUCKDLL ProcessMetrics
    {
        MilliSecond cpu_time;    // CPU time of the process
        size_t      vmem_size;   // Virtual memory size in bytes

        // Constructor.
        ProcessMetrics() : cpu_time(-1), vmem_size(0) {}
    };

    // Get metrics for the current process
    TSDUCKDLL void GetProcessMetrics(ProcessMetrics&) throw(ts::Exception);

    // Ignore SIGPIPE. On UNIX systems: writing to a broken pipe returns an
    // error instead of killing the process. On Windows systems: does nothing.
    TSDUCKDLL void IgnorePipeSignal();

    // Put standard input / output stream in binary mode.
    // On UNIX systems, this does not make any difference.
    // On Windows systems, however, in a stream which is not open in
    // binary mode, there is automatic translation between LF and CR-LF.
    // The standard input / output are open in text mode (non-binary).
    // These functions force them into binary mode.
    // Return true on success, false on error.
    // If report is a subclass or ts::Args, also terminate application.
    TSDUCKDLL bool SetBinaryModeStdin(ReportInterface& report = CERR);
    TSDUCKDLL bool SetBinaryModeStdout(ReportInterface& report = CERR);
}

#include "tsSysUtilsTemplate.h"
