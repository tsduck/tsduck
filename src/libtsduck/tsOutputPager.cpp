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

#include "tsOutputPager.h"
#include "tsSysUtils.h"
#include "tsMemoryUtils.h"


//----------------------------------------------------------------------------
// Get the pager command.
//----------------------------------------------------------------------------

namespace {
    //!
    //! Get the pager command
    //! @param [out] cmd Page command.
    //! @param [out] exe It true, @a cmd is an executable file, otherwise it is a shell command.
    //! @param [in,out] report Where to log "real errors" and debug messages.
    //! @param [in] envName Name of the optional environment variable containing the pager command name.
    //! @return True on success, false on error
    //!
    bool GetPagerCommand(ts::UString& cmd, bool& exe, ts::Report& report, const ts::UString& envName)
    {
        // Check if the PAGER variable contains something.
        if (!envName.empty()) {
            cmd = ts::GetEnvironment(envName);
            cmd.trim();
            if (!cmd.empty()) {
                report.debug(u"%s is \"%s\"", {envName, cmd});
                exe = false;
                return true;
            }
        }

        // Get the path search list.
        ts::UStringList dirs;
        ts::GetEnvironmentPath(dirs, TS_COMMAND_PATH);

        // Predefined list of commands:
        static const ts::UChar* const names[] = {
            u"less" TS_EXECUTABLE_SUFFIX,
            u"more" TS_EXECUTABLE_SUFFIX,
            0
        };

        // Search the predefined pager commands in the path.
        for (size_t i = 0; names[i] != 0; ++i) {
            for (ts::UStringList::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
                cmd = (*it + ts::PathSeparator) + names[i];
                if (ts::FileExists(cmd)) {
                    report.debug(u"pager executable is \"%s\"", {cmd});
                    exe = true;
                    return true;
                }
            }
        }

        // No pager command found.
#if defined(TS_WINDOWS)
        // On Windows, we can always use the embedded "more" command.
        cmd = u"more";
        exe = false;
        return true;
#else
        report.debug(u"no pager command found");
        return false;
#endif
    }
}


//----------------------------------------------------------------------------
// Send application output to a "pager" application such as "more" or "less".
//----------------------------------------------------------------------------

bool ts::OutputPager(Report& report, bool useStdout, bool useStderr, const UString& envName)
{
    // Check that all requested devices are terminals.
    if ((useStdout && !StdOutIsTerminal()) || (useStderr && !StdErrIsTerminal()) || (!useStdout && !useStderr)) {
        return false;
    }

    // Locate the pager command.
    UString pager;
    bool pagerIsExec = false;
    if (!GetPagerCommand(pager, pagerIsExec, report, envName)) {
        return false;
    }

#if defined (TS_WINDOWS)

    // Create a pipe
    ::HANDLE read_handle;
    ::HANDLE write_handle;
    ::SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = TRUE;
    if (::CreatePipe(&read_handle, &write_handle, &sa, 0) == 0) {
        report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
        return false;
    }

    // CreatePipe can only inherit none or both handles. Since we need the
    // read handle to be inherited by the child process, we said "inherit".
    // Now, make sure that the write handle of the pipe is not inherited.
    ::SetHandleInformation(write_handle, HANDLE_FLAG_INHERIT, 0);

    // Make sure our output handles can be inherited
    ::SetHandleInformation(::GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    ::SetHandleInformation(::GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    // Process startup info specifies standard handles
    ::STARTUPINFOW si;
    TS_ZERO(si);
    si.cb = sizeof(si);
    si.hStdInput = read_handle;
    si.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags = STARTF_USESTDHANDLES;

    // If the pager is a command to be decoded (ie not an executable file), build the full command name.
    if (!pagerIsExec) {
        pager.insert(0, u"cmd /q /d /c");
    }

    // Create the process
    ::PROCESS_INFORMATION pi;
    if (::CreateProcessW(NULL, const_cast<::WCHAR*>(pager.wc_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == 0) {
        report.error(u"error creating pager process: %s", {ErrorCodeMessage()});
        ::CloseHandle(read_handle);
        ::CloseHandle(write_handle);
        return false;
    }

    // Close unused process handles
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);

    // Close the reading end-point of pipe.
    ::CloseHandle(read_handle);

    // Use the writing end-point of pipe for stdout and/or stderr.
    if (useStdout && !::SetStdHandle(STD_OUTPUT_HANDLE, write_handle)) {
        report.error(u"error setting stdout: %s", {ErrorCodeMessage()});
    }
    if (useStderr && !::SetStdHandle(STD_ERROR_HANDLE, write_handle)) {
        report.error(u"error setting stderr: %s", {ErrorCodeMessage()});
    }

#else // UNIX

    // Create a pipe
    int filedes[2];
    if (::pipe(filedes) < 0) {
        report.error(u"error creating pipe: %s", {ErrorCodeMessage()});
        return false;
    }

    // Create the forked process
    if ((_fpid = ::fork()) < 0) {
        report.error(u"fork error: %s", {ErrorCodeMessage()});
        return false;
    }
    else if (_fpid == 0) {
        // In the context of the created process.
        // Close standard input.
        ::close(STDIN_FILENO);
        // Close the writing end-point of the pipe.
        ::close(filedes[1]);
        // Redirect the reading end-point of the pipe to standard input
        if (::dup2(filedes[0], STDIN_FILENO) < 0) {
            ::perror("error redirecting stdin in forked process");
            ::exit(EXIT_FAILURE);
        }
        // Close the now extraneous file descriptor.
        ::close(filedes[0]);
        // Execute the command. Should not return.
        const std::string cmd(pager.toUTF8());
        if (pagerIsExec) {
            ::execl(cmd.c_str(), cmd.c_str(), TS_NULL);
        }
        else {
            ::execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), TS_NULL);
        }
        ::perror("exec error");
        ::exit(EXIT_FAILURE);
        assert(false); // should never get there
    }

    // Close the reading end-point of pipe.
    ::close(filedes[0]);

    // Use the writing end-point of pipe for stdout and/or stderr.
    if (useStdout && ::dup2(filedes[1], STDOUT_FILENO) < 0) {
        report.error(u"error setting stdout: %s", {ErrorCodeMessage()});
    }
    if (useStderr && ::dup2(filedes[1], STDERR_FILENO) < 0) {
        report.error(u"error setting stderr: %s", {ErrorCodeMessage()});
    }

#endif

    // Successful creation.
    return true;
}
