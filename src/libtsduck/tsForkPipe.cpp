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
//
//  Fork a process and create a pipe to its standard input.
//
//----------------------------------------------------------------------------

#include "tsForkPipe.h"
#include "tsNullReport.h"
#include "tsMemoryUtils.h"



//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::ForkPipe::ForkPipe() :
    _is_open (false),
    _synchronous (false),
    _ignore_abort (false),
    _broken_pipe (false)
{
    // We will handle broken-pipe errors, don't kill us for that.
    IgnorePipeSignal();
}


ts::ForkPipe::~ForkPipe()
{
    close (*NullReport::Instance());
}


//----------------------------------------------------------------------------
// Create the process, open the pipe.
// If synchronous is true, wait for process termination in close.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::open (const std::string& command, bool synchronous, size_t buffer_size, ReportInterface& report)
{
    if (_is_open) {
        report.error ("pipe is already open");
        return false;
    }

    _broken_pipe = false;
    _synchronous = synchronous;

    report.debug ("creating process \"" + command + "\"");

#if defined (__windows)

    // Create a pipe
    ::HANDLE read_handle;
    ::HANDLE write_handle;
    ::DWORD bufsize = buffer_size == 0 ? 0 : ::DWORD (std::max<size_t> (32768, buffer_size));
    ::SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = TRUE;
    if (::CreatePipe (&read_handle, &write_handle, &sa, bufsize) == 0) {
        report.error ("error creating pipe: " + ErrorCodeMessage());
        return false;
    }

    // CreatePipe can only inherit none or both handles. Since we need the
    // read handle to be inherited by the child process, we said "inherit".
    // Now, make sure that the write handle of the pipe is not inherited.
    ::SetHandleInformation (write_handle, HANDLE_FLAG_INHERIT, 0);

    // Make sure our output handles can be inherited
    ::SetHandleInformation (::GetStdHandle (STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    ::SetHandleInformation (::GetStdHandle (STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    // Process startup info specifies standard handles
    ::STARTUPINFO si;
    TS_ZERO (si);
    si.cb = sizeof(si);
    si.hStdInput = read_handle;
    si.hStdOutput = ::GetStdHandle (STD_OUTPUT_HANDLE);
    si.hStdError = ::GetStdHandle (STD_ERROR_HANDLE);
    si.dwFlags = STARTF_USESTDHANDLES;

    // ::CreateProcess may modify the user-supplied command line (ugly!)
    std::string cmd (command);
    char* cmdp = const_cast<char*> (cmd.c_str());

    // Create the process
    ::PROCESS_INFORMATION pi;
    if (::CreateProcess (NULL, cmdp, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == 0) {
        report.error ("error creating pipe: " + ErrorCodeMessage());
        return false;
    }

    // Close unused handles
    if (_synchronous) {
        _process = pi.hProcess;
    }
    else {
        _process = INVALID_HANDLE_VALUE;
        ::CloseHandle (pi.hProcess);
    }
    ::CloseHandle (pi.hThread);

    // Keep the writing end-point of pipe for data transmission.
    // Close the reading end-point of pipe.
    _handle = write_handle;
    ::CloseHandle (read_handle);

#else // UNIX

    // Create a pipe
    int filedes[2];
    if (::pipe (filedes) < 0) {
        report.error ("error creating pipe: " + ErrorCodeMessage());
        return false;
    }

    // Create the forked process
    if ((_fpid = ::fork()) < 0) {
        report.error ("fork error: " + ErrorCodeMessage());
        return false;
    }
    else if (_fpid == 0) {
        // In the context of the created process.
        // Close standard input.
        ::close (STDIN_FILENO);
        // Close the writing end-point of the pipe.
        ::close (filedes[1]);
        // Redirect the reading end-point of the pipe to standard input
        if (::dup2 (filedes[0], STDIN_FILENO) < 0) {
            ::perror ("error redirecting stdin in forked process");
            ::exit (EXIT_FAILURE);
        }
        // Close the now extraneous file descriptor.
        ::close (filedes[0]);
        // Execute the command. Should not return.
        ::execl ("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
        ::perror ("exec error");
        ::exit (EXIT_FAILURE);
        assert (false); // should never get there
    }

    // Keep the writing end-point of pipe for packet transmission.
    // Close the reading end-point of pipe.
    _fd = filedes[1];
    ::close (filedes[0]);

#endif

    _is_open = true;
    return true;
}


//----------------------------------------------------------------------------
// Close the pipe. Optionally wait for process termination.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::close (ReportInterface& report)
{
    // Silent error is already closed
    if (!_is_open) {
        return false;
    }

    bool result = true;

#if defined (__windows)

    // Close the pipe handle
    ::CloseHandle (_handle);

    // Wait for termination of child process
    if (_synchronous && ::WaitForSingleObject (_process, INFINITE) != WAIT_OBJECT_0) {
        report.error ("error waiting for process termination: " + ErrorCodeMessage());
        result = false;
    }

    if (_process != INVALID_HANDLE_VALUE) {
        ::CloseHandle (_process);
    }

#else // UNIX

    // Close the pipe file descriptor
    ::close (_fd);

    // Wait for termination of forked process
    assert (_fpid != 0);
    if (_synchronous && ::waitpid (_fpid, NULL, 0) < 0) {
        report.error ("error waiting for process termination: " + ErrorCodeMessage());
        result = false;
    }

#endif

    _is_open = false;
    return result;
}


//----------------------------------------------------------------------------
// Write data to the pipe (received at process' standard input).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ForkPipe::write (const void* addr, size_t size, ReportInterface& report)
{
    if (!_is_open) {
        report.error ("pipe is not open");
        return false;
    }

    // If pipe already broken, return
    if (_broken_pipe) {
        return _ignore_abort;
    }

    bool error = false;
    ErrorCode error_code = SYS_SUCCESS;

#if defined (__windows)

    const char* data = reinterpret_cast <const char*> (addr);
    ::DWORD remain = ::DWORD (size);
    ::DWORD outsize;

    while (remain > 0 && !error) {
        if (::WriteFile (_handle, data, remain, &outsize, NULL) != 0)  {
            // Normal case, some data were written
            assert (outsize <= remain);
            data += outsize;
            remain -= std::max (remain, outsize);
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

    const char *data = reinterpret_cast <const char*> (addr);
    size_t remain = size;

    while (remain > 0 && !error) {
        ssize_t outsize = ::write (_fd, data, remain);
        if (outsize > 0) {
            // Normal case, some data were written
            assert (size_t (outsize) <= remain);
            data += outsize;
            remain -= std::max (remain, size_t (outsize));
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
        report.error ("error writing to pipe: " + ErrorCodeMessage (error_code));
        return false;
    }
    else if (_ignore_abort) {
        // Broken pipe but must be ignored. Report a verbose message
        // the first time to inform that data will continue to be
        // processed but will be ignored by the forked process.
        report.verbose ("broken pipe, stopping transmission to forked process");
        // Not an error (ignored)
        return true;
    }
    else {
        // Broken pipe. Do not report a message, but report as error
        return false;
    }
}
