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
//!  @file
//!  Various system utilities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsStringUtils.h"
#include "tsException.h"
#include "tsCerrReport.h"

//!
//! Executable file suffix.
//!
#if defined(DOXYGEN)
    #define TS_EXECUTABLE_SUFFIX "platform_specific" (".exe", ""); // for doc only
#elif defined(TS_WINDOWS)
    #define TS_EXECUTABLE_SUFFIX ".exe"
#else
    #define TS_EXECUTABLE_SUFFIX ""
#endif

//!
//! Environment variable containing the command search path.
//!
#if defined(DOXYGEN)
    #define TS_COMMAND_PATH "platform_specific" ("PATH", "Path"); // for doc only
#elif defined(TS_WINDOWS)
    #define TS_COMMAND_PATH "Path"
#elif defined(TS_UNIX)
    #define TS_COMMAND_PATH "PATH"
#else
    #error "Unimplemented operating system"
#endif

//!
//! Name of the environment variable which contains a list of paths for plugins.
//!
#define TS_PLUGINS_PATH "TSPLUGINS_PATH"

namespace ts {

    //!
    //! Directory separator character in file paths.
    //!
#if defined(DOXYGEN)
    const char PathSeparator = platform_specific ('/', '\\'); // for doc only
#elif defined(TS_WINDOWS)
    const char PathSeparator = '\\';
#elif defined(TS_UNIX)
    const char PathSeparator = '/';
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Separator character in search paths.
    //!
#if defined(DOXYGEN)
    const char SearchPathSeparator = platform_specific (':', ';'); // for doc only
#elif defined(TS_WINDOWS)
    const char SearchPathSeparator = ';';
#elif defined(TS_UNIX)
    const char SearchPathSeparator = ':';
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Return a "vernacular" version of a file path.
    //!
    //! @param [in] path A file path.
    //! @return A copy of @a path where all '/' and '\' have been
    //! translated into the local directory separator.
    //!
    TSDUCKDLL std::string VernacularFilePath(const std::string& path);

    //!
    //! Return the directory name of a file path ("dir/foo.bar" => "dir").
    //!
    //! @param [in] path A file path.
    //! @return The directory name of @a path ("dir/foo.bar" => "dir").
    //!
    TSDUCKDLL std::string DirectoryName(const std::string& path);

    //!
    //! Return the base file name of a file path ("dir/foo.bar" => "foo.bar").
    //!
    //! @param [in] path A file path.
    //! @param [in] suffix An optional file suffix.
    //! If @a path ends in @a suffix, the suffix is removed.
    //! @return The base file name of @a path ("dir/foo.bar" => "foo.bar").
    //!
    TSDUCKDLL std::string BaseName(const std::string& path, const std::string& suffix = std::string());

    //!
    //! Return the suffix of a file path ("dir/foo.bar" => ".bar").
    //!
    //! @param [in] path A file path.
    //! @return The suffix of @a path ("dir/foo.bar" => ".bar").
    //!
    TSDUCKDLL std::string PathSuffix(const std::string& path);

    //!
    //! Conditionally add a suffix to a file path.
    //!
    //! If the file path does not contain a suffix, add the specified one.
    //! Otherwise, return the name unchanged.
    //!
    //! @param [in] path A file path.
    //! @param [in] suffix The suffix to conditionally add.
    //! @return The @a path with a suffix (for conditional suffix ".bar",
    //! "dir/foo" => "dir/foo.bar" and "dir/foo.too" => "dir/foo.too").
    //!
    TSDUCKDLL std::string AddPathSuffix(const std::string& path, const std::string& suffix);

    //!
    //! Return the prefix of a file path ("dir/foo.bar" => "dir/foo").
    //!
    //! @param [in] path A file path.
    //! @return The prefix of @a path ("dir/foo.bar" => "dir/foo").
    //!
    TSDUCKDLL std::string PathPrefix(const std::string& path);

    //!
    //! Get the current user's home directory.
    //!
    //! @return The full path of the current user's home directory.
    //! @throw ts::Exception In case of operating system error.
    //!
    TSDUCKDLL std::string UserHomeDirectory();

    //!
    //! Get the name of the current application executable file.
    //! @return The full path of the executable file which is run in the current process.
    //!
    TSDUCKDLL std::string ExecutableFile();

    //!
    //! Get the name of the system host.
    //! @return The name of the system host.
    //!
    TSDUCKDLL std::string HostName();

    //!
    //! Suspend the current thread for the specified period.
    //! @param [in] delay Number of milliseconds to sleep the current thread.
    //!
    TSDUCKDLL void SleepThread(MilliSecond delay);

    //!
    //! Get system memory page size.
    //! @return The system memory page size in bytes.
    //!
    TSDUCKDLL size_t MemoryPageSize();

    //!
    //! Integer type for process identifier
    //!
#if defined (DOXYGEN)
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
    //! Create a directory
    //! @param [in] path A directory path.
    //! @return A system-specific error code (SYS_SUCCESS on success).
    //!
    TSDUCKDLL ErrorCode CreateDirectory(const std::string& path);

    //!
    //! Return the name of a directory for temporary files.
    //! @return A system-dependent location where temporary files can be created.
    //!
    TSDUCKDLL std::string TempDirectory();

    //!
    //! Return the name of a unique temporary file.
    //! @param [in] suffix An optional suffix to add to the file name.
    //! @return A unique temporary file name.
    //!
    TSDUCKDLL std::string TempFile(const std::string& suffix = ".tmp");

    //!
    //! Get the size in bytes of a file.
    //!
    //! @param [in] path A file path.
    //! @return Size in bytes of the file or -1 in case of error.
    //!
    TSDUCKDLL int64_t GetFileSize(const std::string& path);

    //!
    //! Get the local time of the last modification of a file.
    //!
    //! @param [in] path A file path.
    //! @return Last modification time or Time::Epoch in case of error.
    //!
    TSDUCKDLL Time GetFileModificationTimeLocal(const std::string& path);

    //!
    //! Get the UTC time of the last modification of a file.
    //! @param [in] path A file path.
    //! @return Last modification time or Time::Epoch in case of error.
    //!
    TSDUCKDLL Time GetFileModificationTimeUTC(const std::string& path);

    //!
    //! Check if a file or directory exists
    //! @param [in] path A file path.
    //! @return True if a file or directory exists with that name, false otherwise.
    //!
    TSDUCKDLL bool FileExists(const std::string& path);

    //!
    //! Check if a path exists and is a directory.
    //! @param [in] path A directory path.
    //! @return True if a directory exists with that name, false otherwise.
    //!
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

    //!
    //! Truncate a file to the specified size.
    //!
    //! @param [in] path A file path.
    //! @param [in] size Size in bytes after which the file shall be truncated.
    //! @return A system-specific error code (SYS_SUCCESS on success).
    //!
    TSDUCKDLL ErrorCode TruncateFile(const std::string& path, uint64_t size);

    //!
    //! Rename / move a file or directory.
    //!
    //! If the path specifies a directory, all files in the directory
    //! are moved as well.
    //!
    //! This method is not guaranteed to work when the new and old names
    //! are on distinct volumes or file systems.
    //!
    //! @param [in] old_path The file path of an existing file or directory.
    //! @param [in] new_path The new name for the file or directory.
    //! @return A system-specific error code (SYS_SUCCESS on success).
    //!
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

    //!
    //! Search a configuration file.
    //! @param [in] fileName Name of the file to search.
    //! If @a fileName is not found and does not contain any directory part, search this file
    //! in the following places:
    //! - Directory of the current executable.
    //! - All directories in @c TSPLUGINS_PATH environment variable.
    //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
    //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
    //! @return The path to an existing file or an empty string if not found.
    //!
    TSDUCKDLL std::string SearchConfigurationFile(const std::string& fileName);

    //!
    //! Check if an environment variable exists.
    //! @param [in] varname Environment variable name.
    //! @return True if the specified environment variable exists, false otherwise.
    //!
    TSDUCKDLL bool EnvironmentExists(const std::string& varname);

    //!
    //! Get the value of an environment variable.
    //! @param [in] varname Environment variable name.
    //! @param [in] defvalue Default value if the specified environment variable does not exist.
    //! @return The value of the specified environment variable it it exists, @a defvalue otherwise.
    //!
    TSDUCKDLL std::string GetEnvironment(const std::string& varname, const std::string& defvalue = std::string());

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
    //! @param [in] def Default value if the specified environment variable does not exist.
    //!
    template <class CONTAINER>
    void GetEnvironmentPath(CONTAINER& container, const std::string& name, const std::string& def = std::string())
    {
        SplitString(container, GetEnvironment(name, def), SearchPathSeparator, true);
        if (container.size() == 1 && container.front().empty()) {
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
    TSDUCKDLL bool SetBinaryModeStdin(ReportInterface& report = CERR);

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
    TSDUCKDLL bool SetBinaryModeStdout(ReportInterface& report = CERR);
}

#include "tsSysUtilsTemplate.h"
