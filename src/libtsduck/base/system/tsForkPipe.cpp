//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsForkPipe.h"
#include "tsNullReport.h"
#include "tsMemory.h"
#include "tsSysUtils.h"
#include "tsIntegerUtils.h"

// Index of pipe file descriptors on UNIX.
#define PIPE_READFD  0
#define PIPE_WRITEFD 1
#define PIPE_COUNT   2

// Path to defaut basic shell on UNIX systems.
// Can be overridden on the command line, eg. make CXXFLAGS_EXTRA="-DTS_SHELL_PATH=/foo/bar/bin/sh"
#if !defined(TS_SHELL_PATH)
    #define TS_SHELL_PATH /bin/sh
#endif
#if !defined(TS_SHELL_STRING)
    #define TS_SHELL_STRING TS_STRINGIFY(TS_SHELL_PATH)
#endif


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::ForkPipe::ForkPipe()
{
    // We will handle broken-pipe errors, don't kill us for that.
    IgnorePipeSignal();
}

ts::ForkPipe::~ForkPipe()
{
    close(NULLREP);
}


//----------------------------------------------------------------------------
// Implementation of AbstractOutputStream
//----------------------------------------------------------------------------

bool ts::ForkPipe::writeStreamBuffer(const void* addr, size_t size)
{
    size_t outsize = 0;
    return writeStream(addr, size, outsize, NULLREP);
}


//----------------------------------------------------------------------------
// Implementation of AbstractReadStreamInterface
//----------------------------------------------------------------------------

bool ts::ForkPipe::endOfStream()
{
    return _eof;
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
        sa.bInheritHandle = true;
        if (::CreatePipe(&read_handle, &write_handle, &sa, bufsize) == 0) {
            report.error(u"error creating pipe: %s", {SysErrorCodeMessage()});
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
                report.error(u"error opening NUL: %s", {SysErrorCodeMessage()});
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
    if (::CreateProcessW(nullptr, cmdp, nullptr, nullptr, true, 0, nullptr, nullptr, &si, &pi) == 0) {
        report.error(u"error creating process: %s", {SysErrorCodeMessage()});
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
            std::exit(EXIT_SUCCESS);
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
        report.error(u"error creating pipe: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Create the forked process
    if (_wait_mode == EXIT_PROCESS) {
        // Don't fork, the parent process will directly call exec().
        _fpid = 0;
    }
    else if ((_fpid = ::fork()) < 0) {
        report.error(u"fork error: %s", {SysErrorCodeMessage()});
        if (_use_pipe) {
            ::close(filedes[PIPE_READFD]);
            ::close(filedes[PIPE_WRITEFD]);
        }
        return false;
    }

    // If we want to make the created process asynchronous, it shall not remain zombie until
    // someone waits for it, since noone will wait for it. To make a process non-zombie, the
    // created process must become a session leader (setsid) and then fork again and die.
    // Thus, the application process is a grand-child of the calling application and the
    // intermediate process no longer exists.
    if (_wait_mode == ASYNCHRONOUS) {
        if (_fpid != 0) {
            // In the parent process, wait for the intermediate child to die immediately.
            // Failing to do so, the intermediate process would remain zombie.
            ::waitpid(_fpid, nullptr, 0);
        }
        else {
            // In the intermediate process. First make it a session leader.
            ::setsid();
            // Then create the grand-child process.
            if (::fork() != 0) {
                // In the intermediate process, die immediately.
                std::exit(EXIT_SUCCESS);
            }
            // We are here in the grand-child process...
        }
    }

    if (_fpid != 0) {
        // In the context of the parent process.
        if (_in_pipe) {
            // Keep the writing end-point of pipe for data transmission.
            _fd = filedes[PIPE_WRITEFD];
            // But make it automatically closed on exec(). If the parent process
            // creates another child later, we do not want it to inherit this file
            // descriptor, assuming that fork() is always followed by exec().
            ::fcntl(_fd, F_SETFD, FD_CLOEXEC);
            // Close the reading end-point of pipe.
            ::close(filedes[PIPE_READFD]);
        }
        else if (_out_pipe) {
            // Do the opposite.
            _fd = filedes[PIPE_READFD];
            ::fcntl(_fd, F_SETFD, FD_CLOEXEC);
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
                // Redirect the reading end-point of the pipe to standard input
                if (::dup2(filedes[PIPE_READFD], STDIN_FILENO) < 0) {
                    error = errno;
                    message = "error redirecting stdin in forked process";
                }
                break;
            }
            case STDIN_PARENT:
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
                break;
            }
            case KEEP_BOTH:
            default: {
                // Nothing to do.
                break;
            }
        }

        // The original file descriptors of the pipe are now useless.
        // Either they were redirected to stdin/out/err or they are unused.
        if (_use_pipe) {
            ::close(filedes[PIPE_WRITEFD]);
            ::close(filedes[PIPE_READFD]);
        }

        // Execute the command if there was no prior error.
        if (message == nullptr) {
            ::execl(TS_SHELL_STRING, TS_SHELL_STRING, "-c", command.toUTF8().c_str(), nullptr);
            // Should not return, so this is an error if we get there.
            error = errno;
            message = "exec error";
        }

        // At this point, there was an error.
        if (_wait_mode == EXIT_PROCESS) {
            // No process was created, so return to the caller.
            report.error(u"%s: %s", {message, SysErrorCodeMessage(error)});
            return false;
        }
        else {
            // In a created process, the application is still running elsewhere.
            errno = error;
            ::perror(message);
            std::exit(EXIT_FAILURE);
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

    // Flush the output buffer, if any.
    if (_in_pipe) {
        flush(); // from std::basic_ostream
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
        report.error(u"error waiting for process termination: %s", {SysErrorCodeMessage()});
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
    if (_wait_mode == SYNCHRONOUS) {
        assert(_fpid != 0);
        if (::waitpid(_fpid, nullptr, 0) < 0) {
            report.error(u"error waiting for process termination: %s", {SysErrorCodeMessage()});
            result = false;
        }
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
        _broken_pipe = true;
        _eof = true;

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
// Implementation of AbstractWriteStreamInterface
//----------------------------------------------------------------------------

bool ts::ForkPipe::writeStream(const void* addr, size_t size, size_t& written_size, Report& report)
{
    written_size = 0;

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
    int errcode = 0;

#if defined(TS_WINDOWS)

    const char* data = reinterpret_cast<const char*>(addr);
    ::DWORD remain = ::DWORD(size);
    ::DWORD outsize = 0;

    while (remain > 0 && !error) {
        if (::WriteFile(_handle, data, remain, &outsize, nullptr) != 0) {
            // Normal case, some data were written
            assert(outsize <= remain);
            data += outsize;
            remain -= std::max(remain, outsize);
            written_size += size_t(outsize);
        }
        else {
            // Write error
            errcode = ::GetLastError();
            error = true;
            // MSDN documentation on WriteFile says ERROR_BROKEN_PIPE,
            // experience says ERROR_NO_DATA.
            _broken_pipe = errcode == ERROR_BROKEN_PIPE || errcode == ERROR_NO_DATA;
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
            written_size += size_t(outsize);
        }
        else if ((errcode = errno) != EINTR) {
            // Actual error (not an interrupt)
            error = true;
            _broken_pipe = errcode == EPIPE;
        }
    }
#endif

    if (!error) {
        return true;
    }
    else if (!_broken_pipe) {
        // Always report non-pipe error (message + error status).
        report.error(u"error writing to pipe: %s", {SysErrorCodeMessage(errcode)});
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
// Implementation of AbstractReadStreamInterface
//----------------------------------------------------------------------------

bool ts::ForkPipe::readStreamPartial(void *addr, size_t max_size, size_t& ret_size, Report &report)
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

#if defined(TS_WINDOWS)

    ::DWORD errcode = ERROR_SUCCESS;
    ::DWORD insize = 0;
    if (::ReadFile(_handle, addr, ::DWORD(max_size), &insize, nullptr) != 0) {
        // Normal case, some data were read.
        assert(insize <= ::DWORD(max_size));
        insize = std::max(::DWORD(0), insize);  // just in case we got a negative value
        ret_size = size_t(insize);
        return true;
    }
    else if ((errcode = ::GetLastError()) == ERROR_HANDLE_EOF || errcode == ERROR_BROKEN_PIPE) {
        // End of file, not a real "error".
        _eof = true;
        return false;
    }
    else {
        // This is a real error.
        report.error(u"error reading from pipe: %s", {SysErrorCodeMessage(errcode)});
        return false;
    }

#else // UNIX

    for (;;) {
        const ssize_t insize = ::read(_fd, addr, max_size);
        if (insize == 0) {
            // End of file.
            _eof = true;
            return false;
        }
        else if (insize > 0) {
            // Normal case, some data were read.
            assert(size_t(insize) <= max_size);
            ret_size = size_t(insize);
            return true;
        }
        else if (errno != EINTR) {
            // Actual error (not an interrupt)
            report.error(u"error reading from pipe: %s", {SysErrorCodeMessage()});
            return false;
        }
    }
#endif
}


//----------------------------------------------------------------------------
// This static method asynchronously launches a command, without pipe,
// without waiting for the completion of the command process.
//----------------------------------------------------------------------------

bool ts::ForkPipe::Launch(const ts::UString& command, ts::Report& report, ts::ForkPipe::OutputMode out_mode, ts::ForkPipe::InputMode in_mode)
{
    // Reject input and output mode involving pipes.
    if (in_mode == STDIN_PIPE) {
        report.error(u"internal error, invalid input mode in ForkPipe::Launch");
        return false;
    }
    if (out_mode == STDOUT_PIPE || out_mode == STDOUTERR_PIPE) {
        report.error(u"internal error, invalid output mode in ForkPipe::Launch");
        return false;
    }

    // Run the command asynchronously.
    ForkPipe exe;
    if (exe.open(command, ASYNCHRONOUS, 0, report, out_mode, in_mode)) {
        // Process was created asynchronously, close ForkPipe object now.
        return exe.close(report);
    }
    else {
        report.error(u"cannot execute command: %s", {command});
        return false;
    }
}
