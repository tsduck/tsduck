//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Fork a process and create a pipe to its standard input.
//
//----------------------------------------------------------------------------

#include "tsForkPipe.h"
#include "tsNullReport.h"
#include "tsMemoryUtils.h"
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;

// Index of pipe file descriptors on UNIX.
#define PIPE_READFD  0
#define PIPE_WRITEFD 1
#define PIPE_COUNT   2


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::ForkPipe::ForkPipe() :
    _in_mode(STDIN_PIPE),
    _out_mode(KEEP_BOTH),
    _is_open(false),
    _wait_mode(ASYNCHRONOUS),
    _in_pipe(false),
    _out_pipe(false),
    _use_pipe(false),
    _ignore_abort(false),
    _broken_pipe(false),
    _eof(false),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE),
    _process(INVALID_HANDLE_VALUE)
#else
    _fpid(0),
    _fd(-1)
#endif
{
    // We will handle broken-pipe errors, don't kill us for that.
    IgnorePipeSignal();
}


ts::ForkPipe::~ForkPipe()
{
    close(NULLREP);
}


//----------------------------------------------------------------------------
// Create the process, open the pipe.
//----------------------------------------------------------------------------

bool ts::ForkPipe::open(const UString& command, WaitMode wait_mode, size_t buffer_size, Report& report, OutputMode out_mode, InputMode in_mode)
{
    if (_is_open) {
        report.error(u"pipe is already open");
        return false;
    }

    // Characterize the use of the pipe.
    _in_pipe = in_mode == STDIN_PIPE;
    _out_pipe = out_mode == STDOUT_PIPE || out_mode == STDOUTERR_PIPE;
    _use_pipe = _in_pipe || _out_pipe;

    // We cannot use a pipe if we plan to exit immediately.
    if (wait_mode == EXIT_PROCESS && _use_pipe) {
        report.error(u"cannot use a pipe with exit-process option");
        return false;
    }

    // We can't use the pipe on both sides.
    if (_in_pipe && _out_pipe) {
        report.error(u"cannot use a pipe on both side at the same time");
        return false;
    }

    _in_mode = in_mode;
    _out_mode = out_mode;
    _broken_pipe = false;
    _wait_mode = wait_mode;
    _eof = !_out_pipe;

    report.debug(u"creating process \"%s\"", {command});

#if defined(TS_WINDOWS)

    _handle = INVALID_HANDLE_VALUE;
    _process = INVALID_HANDLE_VALUE;
    ::HANDLE read_handle = INVALID_HANDLE_VALUE;
    ::HANDLE write_handle = INVALID_HANDLE_VALUE;
    ::HANDLE null_handle = INVALID_HANDLE_VALUE;

    // Create a pipe
    if (_use_pipe) {
        ::DWORD bufsize = buffer_size == 0 ? 0 : ::DWORD(std::max<size_t>(32768, buffer_size));
        ::SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = 0;
        sa.bInheritHandle = TRUE;
        if (::CreatePipe(&read_handle, &write_handle, &sa, bufsize) == 0) {
            report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
            return false;
        }

        // CreatePipe can only inherit none or both handles. Since we need the
        // one handle to be inherited by the child process, we said "inherit".
        // Now, make sure that our end of the pipe is not inherited.
        ::SetHandleInformation(_in_pipe ? write_handle : read_handle, HANDLE_FLAG_INHERIT, 0);
    }

    // Our standard handles.
    const ::HANDLE in_handle  = ::GetStdHandle(STD_INPUT_HANDLE);
    const ::HANDLE out_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    const ::HANDLE err_handle = ::GetStdHandle(STD_ERROR_HANDLE);

    // Process startup info specifies standard handles.
    // Make sure our handles can be inherited when necessary.
    ::STARTUPINFOW si;
    TS_ZERO(si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    switch (_in_mode) {
        case STDIN_PIPE: {
            si.hStdInput = read_handle;
            break;
        }
        case STDIN_PARENT: {
            ::SetHandleInformation(in_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdInput = in_handle;
            break;
        }
        case STDIN_NONE: {
            // Open the null device for reading.
            null_handle = ::CreateFileA("NUL:", GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
            if (null_handle == INVALID_HANDLE_VALUE) {
                report.error(u"error opening NUL: %s", {ErrorCodeMessage()});
                if (_use_pipe) {
                    ::CloseHandle(read_handle);
                    ::CloseHandle(write_handle);
                }
                return false;
            }
            // Set the null device as standard input.
            ::SetHandleInformation(null_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdInput = null_handle;
            break;
        }
        default: {
            // Invalid enum value.
            if (_use_pipe) {
                ::CloseHandle(read_handle);
                ::CloseHandle(write_handle);
            }
            return false;
        }
    }

    switch (out_mode) {
        case KEEP_BOTH: {
            ::SetHandleInformation(out_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            ::SetHandleInformation(err_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = out_handle;
            si.hStdError = err_handle;
            break;
        }
        case STDOUT_ONLY: {
            ::SetHandleInformation(out_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = si.hStdError = out_handle;
            break;
        }
        case STDERR_ONLY: {
            ::SetHandleInformation(err_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdOutput = si.hStdError = err_handle;
            break;
        }
        case STDOUT_PIPE: {
            ::SetHandleInformation(err_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            si.hStdError = err_handle;
            si.hStdOutput = write_handle;
            break;
        }
        case STDOUTERR_PIPE: {
            si.hStdOutput = si.hStdError = write_handle;
            break;
        }
        default: {
            // Invalid enum value.
            if (_use_pipe) {
                ::CloseHandle(read_handle);
                ::CloseHandle(write_handle);
            }
            return false;
        }
    }

    // ::CreateProcess may modify the user-supplied command line (ugly!)
    UString cmd(command);
    ::WCHAR* cmdp = const_cast<::WCHAR*>(cmd.wc_str());

    // Create the process
    ::PROCESS_INFORMATION pi;
    if (::CreateProcessW(NULL, cmdp, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == 0) {
        report.error(u"error creating process: %s", {ErrorCodeMessage()});
        if (_use_pipe) {
            ::CloseHandle(read_handle);
            ::CloseHandle(write_handle);
        }
        return false;
    }

    // Close unused handles
    switch (_wait_mode) {
        case ASYNCHRONOUS: {
            // Process handle is useless, we won't use it.
            _process = INVALID_HANDLE_VALUE;
            ::CloseHandle(pi.hProcess);
            break;
        }
        case SYNCHRONOUS: {
            // Keep process handle to wait for it.
            _process = pi.hProcess;
            break;
        }
        case EXIT_PROCESS: {
            // Exit parent process.
            ::exit(EXIT_SUCCESS);
            break;
        }
        default: {
            // Should not get there.
            assert(false);
            break;
        }
    }
    ::CloseHandle(pi.hThread);

    // Keep our end-point of pipe for data transmission.
    // Close the other end-point of pipe.
    if (_in_pipe) {
        _handle = write_handle;
        ::CloseHandle(read_handle);
    }
    else if (_out_pipe) {
        _handle = read_handle;
        ::CloseHandle(write_handle);
    }

    // Close other no longer used handles.
    if (null_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(null_handle);
    }

#else // UNIX

    // Create a pipe
    int filedes[PIPE_COUNT];
    if (_use_pipe && ::pipe(filedes) < 0) {
        report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
        return false;
    }

    // Create the forked process
    if (_wait_mode == EXIT_PROCESS) {
        // Don't fork, the parent process will directly call exec().
        _fpid = 0;
    }
    else if ((_fpid = ::fork()) < 0) {
        report.error(u"fork error: %s", {ErrorCodeMessage()});
        if (_use_pipe) {
            ::close(filedes[PIPE_READFD]);
            ::close(filedes[PIPE_WRITEFD]);
        }
        return false;
    }

    if (_fpid != 0) {
        // In the context of the parent process.
        if (_in_pipe) {
            // Keep the writing end-point of pipe for data transmission.
            // Close the reading end-point of pipe.
            _fd = filedes[PIPE_WRITEFD];
            ::close(filedes[PIPE_READFD]);
        }
        else if (_out_pipe) {
            // Do the opposite.
            _fd = filedes[PIPE_READFD];
            ::close(filedes[PIPE_WRITEFD]);
        }
    }
    else {
        // In the context of the created process (or application if EXIT_PROCESS mode).
        // In the first case, abort on error. In the latter, report error and return to caller.
        int error = 0;
        const char* message = nullptr;

        // Setup stdin.
        switch (in_mode) {
            case STDIN_NONE: {
                // Open the null device as stdin and redirect it to standard input.
                const int infd = ::open("/dev/null", O_RDONLY);
                if (infd < 0) {
                    error = errno;
                    message = "error opening /dev/null in forked process";
                }
                else if (::dup2(infd, STDIN_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stdin in forked process";
                }
                else {
                    // Original file descriptor is no longer needed.
                    ::close(infd);
                }
                break;
            }
            case STDIN_PIPE: {
                // Close the writing end-point of the pipe.
                ::close(filedes[PIPE_WRITEFD]);
                // Redirect the reading end-point of the pipe to standard input
                if (::dup2(filedes[PIPE_READFD], STDIN_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stdin in forked process";
                }
                // Close the now extraneous file descriptor.
                ::close(filedes[PIPE_READFD]);
                break;
            }
            default: {
                // Nothing to do.
                break;
            }
        }

        // Setup stdout and stderr.
        switch (out_mode) {
            case STDOUT_ONLY: {
                // Use stdout as stderr as well.
                if (::dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stderr to stdout";
                }
                break;
            }
            case STDERR_ONLY: {
                // Use stderr as stdout as well.
                if (::dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stdout to stderr";
                }
                break;
            }
            case STDOUT_PIPE:
            case STDOUTERR_PIPE: {
                // Close reading end-point of the pipe.
                ::close(filedes[PIPE_READFD]);
                // Redirect stdout to the write end-point of the pipe.
                if (::dup2(filedes[PIPE_WRITEFD], STDOUT_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stdout to pipe";
                }
                // Same for stderr if requested.
                if (out_mode == STDOUTERR_PIPE && ::dup2(filedes[PIPE_WRITEFD], STDERR_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stderr to pipe";
                }
                // Close the now extraneous file descriptor.
                ::close(filedes[PIPE_WRITEFD]);
                break;
            }
            default: {
                // Nothing to do.
                break;
            }
        }

        // Execute the command if there was no prior error.
        if (message == nullptr) {
            ::execl("/bin/sh", "/bin/sh", "-c", command.toUTF8().c_str(), nullptr);
            // Should not return, so this is an error if we get there.
            error = errno;
            message = "exec error";
        }

        // At this point, there was an error.
        if (_wait_mode == EXIT_PROCESS) {
            // No process was created, so return to the caller.
            report.error(u"%s: %s", {message, ErrorCodeMessage(error)});
            return false;
        }
        else {
            // In a created process, the application is still running elsewhere.
            errno = error;
            ::perror(message);
            ::exit(EXIT_FAILURE);
            assert(false); // should never get there
        }
    }

#endif

    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Close the pipe. Optionally wait for process termination.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::close(Report& report)
{
    // Silent error is already closed
    if (!_is_open) {
        return false;
    }

    bool result = true;

#if defined(TS_WINDOWS)

    // Close the pipe handle
    if (_use_pipe) {
        report.debug(u"closing pipe handle");
        ::CloseHandle(_handle);
    }

    // Wait for termination of child process
    if (_wait_mode == SYNCHRONOUS && ::WaitForSingleObject(_process, INFINITE) != WAIT_OBJECT_0) {
        report.error(u"error waiting for process termination: %s", {ErrorCodeMessage()});
        result = false;
    }

    if (_process != INVALID_HANDLE_VALUE) {
        report.debug(u"closing process handle");
        ::CloseHandle(_process);
    }

#else // UNIX

    // Close the pipe file descriptor
    if (_use_pipe) {
        ::close(_fd);
    }

    // Wait for termination of forked process
    assert(_fpid != 0);
    if (_wait_mode == SYNCHRONOUS && ::waitpid(_fpid, nullptr, 0) < 0) {
        report.error(u"error waiting for process termination: %s", {ErrorCodeMessage()});
        result = false;
    }

#endif

    _is_open = false;
    return result;
}


//----------------------------------------------------------------------------
// Abort any currenly input/output operation in the pipe.
//----------------------------------------------------------------------------

void ts::ForkPipe::abortPipeReadWrite()
{
    if (_is_open) {
        // Mark broken pipe, read or write.
        _broken_pipe = _eof = true;

        // Close pipe handle, ignore errors.
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
#else // UNIX
        ::close(_fd);
        _fd = -1;
#endif
    }
}


//----------------------------------------------------------------------------
// Write data to the pipe (received at process' standard input).
//----------------------------------------------------------------------------

bool ts::ForkPipe::write(const void* addr, size_t size, Report& report)
{
    if (!_is_open) {
        report.error(u"pipe is not open");
        return false;
    }
    if (!_in_pipe) {
        report.error(u"process was created without input pipe");
        return false;
    }

    // If pipe already broken, return
    if (_broken_pipe) {
        return _ignore_abort;
    }

    bool error = false;
    ErrorCode error_code = SYS_SUCCESS;

#if defined(TS_WINDOWS)

    const char* data = reinterpret_cast<const char*>(addr);
    ::DWORD remain = ::DWORD(size);
    ::DWORD outsize = 0;

    while (remain > 0 && !error) {
        if (::WriteFile(_handle, data, remain, &outsize, NULL) != 0) {
            // Normal case, some data were written
            assert(outsize <= remain);
            data += outsize;
            remain -= std::max(remain, outsize);
        }
        else {
            // Write error
            error_code = LastErrorCode();
            error = true;
            // MSDN documentation on WriteFile says ERROR_BROKEN_PIPE,
            // experience says ERROR_NO_DATA.
            _broken_pipe = error_code == ERROR_BROKEN_PIPE || error_code == ERROR_NO_DATA;
        }
    }

#else // UNIX

    const char *data = reinterpret_cast<const char*>(addr);
    size_t remain = size;

    while (remain > 0 && !error) {
        ssize_t outsize = ::write(_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            assert(size_t(outsize) <= remain);
            data += outsize;
            remain -= std::max(remain, size_t(outsize));
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            error_code = LastErrorCode();
            error = true;
            _broken_pipe = error_code == EPIPE;
        }
    }
#endif

    if (!error) {
        return true;
    }
    else if (!_broken_pipe) {
        // Always report non-pipe error (message + error status).
        report.error(u"error writing to pipe: %s", {ErrorCodeMessage(error_code)});
        return false;
    }
    else if (_ignore_abort) {
        // Broken pipe but must be ignored. Report a verbose message
        // the first time to inform that data will continue to be
        // processed but will be ignored by the forked process.
        report.verbose(u"broken pipe, stopping transmission to forked process");
        // Not an error (ignored)
        return true;
    }
    else {
        // Broken pipe. Do not report a message, but report as error
        return false;
    }
}


//----------------------------------------------------------------------------
// Read data from the pipe (sent from process' standard output or error).
//----------------------------------------------------------------------------

bool ts::ForkPipe::read(void* addr, size_t max_size, size_t unit_size, size_t& ret_size, ts::Report& report)
{
    ret_size = 0;

    if (!_is_open) {
        report.error(u"pipe is not open");
        return false;
    }
    if (!_out_pipe) {
        report.error(u"process was created without output pipe");
        return false;
    }
    if (_eof) {
        // Already at end of file. Do not report error.
        return false;
    }
    if (max_size == 0) {
        // Trivial case, successfully read zero bytes.
        return true;
    }
    if (unit_size > 0 && max_size < unit_size) {
        report.error(u"internal error, buffer (%'d bytes) is smaller than unit size (%'d bytes)", {max_size, unit_size});
        return false;
    }
    if (unit_size > 0) {
        // Round down buffer size to a multiple of unit size.
        max_size = RoundDown(max_size, unit_size);
    }

    ErrorCode error_code = SYS_SUCCESS;

#if defined (TS_WINDOWS)

    char* data = reinterpret_cast<char*>(addr);
    ::DWORD remain = ::DWORD(max_size);

    for (;;) {
        ::DWORD insize = 0;
        if (::ReadFile(_handle, data, remain, &insize, NULL) != 0) {
            // Normal case, some data were read.
            assert(insize <= remain);
            insize = std::max(::DWORD(0), insize);  // just in case we got a negative value
            ret_size += insize;
            data += insize;
            remain -= std::min(remain, insize);
            // Exit when we read an integral number of "units" or the buffer is full.
            if (unit_size == 0 || remain == 0 || ret_size % unit_size == 0) {
                break;
            }
            // Need to read only the end of a "unit".
            remain = std::min(remain, ::DWORD(unit_size - ret_size % unit_size));
        }
        else if ((error_code = LastErrorCode()) == ERROR_HANDLE_EOF || error_code == ERROR_BROKEN_PIPE) {
            // End of file, not a real "error".
            _eof = true;
            break;
        }
        else {
            // This is a real error
            report.error(u"error reading from pipe: %s", {ErrorCodeMessage(error_code)});
            return false;
        }
    }

#else // UNIX

    char *data = reinterpret_cast<char*>(addr);
    size_t remain = max_size;

    for (;;) {
        const ssize_t insize = ::read(_fd, data, remain);
        if (insize == 0) {
            // End of file.
            _eof = true;
            break;
        }
        else if (insize > 0) {
            // Normal case, some data were read.
            assert(size_t(insize) <= remain);
            ret_size += size_t(insize);
            data += insize;
            remain -= std::min(remain, size_t(insize));
            // Exit when we read an integral number of "units" or the buffer is full.
            if (unit_size == 0 || remain == 0 || ret_size % unit_size == 0) {
                break;
            }
            // Need to read only the end of a "unit".
            remain = std::min(remain, unit_size - ret_size % unit_size);
        }
        else if ((error_code = LastErrorCode()) != EINTR) {
            // Actual error (not an interrupt)
            report.error(u"error reading from pipe: %s", {ErrorCodeMessage(error_code)});
            return false;
        }
    }
#endif

    // At end of file, truncate to unit size (drop trailing partial unit if any).
    if (_eof && unit_size > 0) {
        ret_size = RoundDown(ret_size, unit_size);
    }

    // Not an error yet if we already read some data.
    return ret_size > 0;
}
