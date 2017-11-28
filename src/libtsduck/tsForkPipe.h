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
//!  Fork a process and create a pipe to its standard input.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSysUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Fork a process and create a pipe to its standard input.
    //!
    class TSDUCKDLL ForkPipe
    {
    public:
        //!
        //! Default constructor.
        //!
        ForkPipe();

        //!
        //! Destructor.
        //!
        ~ForkPipe();

        //!
        //! Create the process, open the pipe.
        //! @param [in] command The command to execute.
        //! @param [in] synchronous If true, wait for process termination in close().
        //! @param [in] buffer_size The pipe buffer size in bytes. Used on Windows only. Zero means default.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(const UString& command, bool synchronous, size_t buffer_size, Report& report);

        //!
        //! Close the pipe.
        //! Optionally wait for process termination if @a synchronous was true on open().
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Check if the process is running and the pipe is open.
        //! @return True if the process is running and the pipe is open.
        //!
        bool isOpen() const
        {
            return _is_open;
        }

        //!
        //! Check if the pipe was broken.
        //! @return True if was broken (unexpected process termination for instance).
        //!
        bool isBroken() const
        {
            return _broken_pipe;
        }

        //!
        //! Check if synchronous mode is active (ie. will wait for process termination).
        //! @return True if synchronous mode is active.
        //!
        bool isSynchronous() const
        {
            return _synchronous;
        }

        //!
        //! Set "ignore abort".
        //! @param [in] on If true and the process aborts, do not report error
        //! when writing data.
        //!
        void setIgnoreAbort(bool on)
        {
            _ignore_abort = on;
        }

        //!
        //! Get "ignore abort".
        //! @return True if, when the process aborts, do not report error when writing data.
        //!
        bool getIgnoreAbort() const
        {
            return _ignore_abort;
        }

        //!
        //! Write data to the pipe (received at process' standard input).
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool write(const void* addr, size_t size, Report& report);

    private:
        bool     _is_open;       // Open and running.
        bool     _synchronous;   // Wait for child process termination in stop()
        bool     _ignore_abort;  // Ignore early termination of child process
        bool     _broken_pipe;   // Pipe is broken, do not attempt to write
#if defined(TS_WINDOWS)
        ::HANDLE _handle;        // Pipe output handle
        ::HANDLE _process;       // Handle to child process
#else
        ::pid_t  _fpid;          // Forked process id (UNIX PID, not MPEG PID!)
        int      _fd;            // Pipe output file descriptor
#endif
    };
}
