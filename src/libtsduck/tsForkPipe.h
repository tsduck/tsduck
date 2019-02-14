//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsAbstractOutputStream.h"
#include "tsSysUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Fork a process and create an optional pipe to its standard input.
    //! @ingroup system
    //!
    //! This class can be used as any output stream when the output is a pipe.
    //!
    class TSDUCKDLL ForkPipe: public AbstractOutputStream
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
        //! How to wait for the created process when close() is invoked.
        //! No pipe can be used with EXIT_PROCESS because there would be
        //! nobody on the other end of the pipe.
        //!
        enum WaitMode {
            ASYNCHRONOUS,  //!< Don't wait, close() will return immediately.
            SYNCHRONOUS,   //!< Wait for process completion during close().
            EXIT_PROCESS,  //!< Exit parent process during open(). UNIX: call exec(), Windows: call exit() @e after process creation.
        };

        //!
        //! How to standard input in the created process.
        //!
        //! The pipe can be used either on input or output, but not both.
        //! So, STDIN_PIPE is also forbidden with output mode is either
        //! STDOUT_PIPE or STDOUTERR_PIPE.
        //!
        enum InputMode {
            STDIN_PARENT,  //!< Keep same stdin as current (parent) process.
            STDIN_PIPE,    //!< Use the pipe as stdin.
            STDIN_NONE,    //!< No standard input (the null device in fact).
        };

        //!
        //! How to merge standard output and standard error in the created process.
        //!
        enum OutputMode {
            KEEP_BOTH,       //!< Keep same stdout and stderr as current (parent) process.
            STDOUT_ONLY,     //!< Merge stderr into current stdout.
            STDERR_ONLY,     //!< Merge stdout into current stderr.
            STDOUT_PIPE,     //!< Use the pipe to receive stdout, keep same stderr as current (parent) process.
            STDOUTERR_PIPE,  //!< Use the pipe to receive a merge of stdout and stderr.
        };

        //!
        //! Create the process, open the optional pipe.
        //! @param [in] command The command to execute.
        //! @param [in] wait_mode How to wait for process termination in close().
        //! @param [in] buffer_size The pipe buffer size in bytes. Used on Windows only. Zero means default.
        //! @param [in,out] report Where to report errors.
        //! @param [in] out_mode How to handle stdout and stderr.
        //! @param [in] in_mode How to handle stdin. Use the pipe by default.
        //! When set to KEEP_STDIN, no pipe is created.
        //! @return True on success, false on error.
        //! Do not return on success when @a wait_mode is EXIT_PROCESS.
        //!
        bool open(const UString& command, WaitMode wait_mode, size_t buffer_size, Report& report, OutputMode out_mode, InputMode in_mode);

        //!
        //! Close the pipe.
        //! Optionally wait for process termination if @a wait_mode was SYNCHRONOUS on open().
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Check if the process is running and the pipe is open (when used).
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
            return _wait_mode == SYNCHRONOUS;
        }

        //!
        //! Set "ignore abort".
        //! @param [in] on If true and the process aborts, do not report error when writing data.
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

        //!
        //! Read data from the pipe (sent from process' standard output or error).
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [in] unit_size If not zero, make sure that the input size is always
        //! a multiple of @a unit_size. If the initial read ends in the middle of a @e unit,
        //! read again and again, up to the end of the current unit or end of file.
        //! @param [out] ret_size Returned input size in bytes.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool read(void* addr, size_t max_size, size_t unit_size, size_t& ret_size, Report& report);

        //!
        //! Abort any currenly input/output operation in the pipe.
        //! The pipe is left in a broken state and can be only closed.
        //!
        void abortPipeReadWrite();

        //!
        //! Check if the input pipe is at end of file.
        //! @return True if the input pipe is at end of file.
        //!
        bool eof() const { return _eof; }

    protected:
        // Implementation of AbstractOutputStream
        virtual bool writeStreamBuffer(const void* addr, size_t size) override;

    private:
        InputMode     _in_mode;       // Input mode for the created process.
        OutputMode    _out_mode;      // Output mode for the created process.
        volatile bool _is_open;       // Open and running.
        WaitMode      _wait_mode;     // How to wait for child process termination in close().
        bool          _in_pipe;       // The process uses an input pipe.
        bool          _out_pipe;      // The process uses an output pipe.
        bool          _use_pipe;      // The process uses a pipe, somehow.
        bool          _ignore_abort;  // Ignore early termination of child process.
        volatile bool _broken_pipe;   // Pipe is broken, do not attempt to write.
        volatile bool _eof;           // Got end of file on input pipe.
#if defined(TS_WINDOWS)
        ::HANDLE      _handle;        // Pipe output handle.
        ::HANDLE      _process;       // Handle to child process.
#else
        ::pid_t       _fpid;          // Forked process id (UNIX PID, not MPEG PID!)
        int           _fd;            // Pipe output file descriptor.
#endif

        // Inaccessible operations.
        ForkPipe(const ForkPipe&) = delete;
        ForkPipe& operator=(const ForkPipe&) = delete;
    };
}
