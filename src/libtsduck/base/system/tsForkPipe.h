//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Fork a process and create a pipe to its standard input.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractOutputStream.h"
#include "tsAbstractReadStreamInterface.h"
#include "tsAbstractWriteStreamInterface.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Fork a process and create an optional pipe to its standard input.
    //! @ingroup system
    //!
    //! This class can be used as any output stream when the output is a pipe.
    //!
    class TSDUCKDLL ForkPipe:
        public AbstractOutputStream,
        public AbstractReadStreamInterface,
        public AbstractWriteStreamInterface
    {
        TS_NOCOPY(ForkPipe);
    public:
        //!
        //! Default constructor.
        //!
        ForkPipe();

        //!
        //! Destructor.
        //!
        virtual ~ForkPipe() override;

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
        //! Abort any currenly input/output operation in the pipe.
        //! The pipe is left in a broken state and can be only closed.
        //!
        void abortPipeReadWrite();

        //!
        //! This static method asynchronously launches a command, without pipe, without waiting for the completion of the command process.
        //! @param [in] command The command to execute.
        //! @param [in,out] report Where to report errors.
        //! @param [in] out_mode How to handle stdout and stderr. Keep both by default.
        //! Output modes using pipes are forbidden.
        //! @param [in] in_mode How to handle stdin. Keep the parent input by default.
        //! Input modes using pipes are forbidden.
        //! @return True on success, false on error.
        //!
        static bool Launch(const UString& command, Report& report, OutputMode out_mode = KEEP_BOTH, InputMode in_mode = STDIN_PARENT);

        // Implementation of AbstractReadStreamInterface
        virtual bool endOfStream() override;
        virtual bool readStreamPartial(void* addr, size_t max_size, size_t& ret_size, Report& report) override;

        // Implementation of AbstractWriteStreamInterface
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, Report& report) override;

    protected:
        // Implementation of AbstractOutputStream
        virtual bool writeStreamBuffer(const void* addr, size_t size) override;

    private:
        InputMode     _in_mode {STDIN_PIPE};     // Input mode for the created process.
        OutputMode    _out_mode {KEEP_BOTH};     // Output mode for the created process.
        volatile bool _is_open = false;          // Open and running.
        WaitMode      _wait_mode {ASYNCHRONOUS}; // How to wait for child process termination in close().
        bool          _in_pipe = false;          // The process uses an input pipe.
        bool          _out_pipe = false;         // The process uses an output pipe.
        bool          _use_pipe = false;         // The process uses a pipe, somehow.
        bool          _ignore_abort = false;     // Ignore early termination of child process.
        volatile bool _broken_pipe = false;      // Pipe is broken, do not attempt to write.
        volatile bool _eof = false;              // Got end of file on input pipe.
#if defined(TS_WINDOWS)
        ::HANDLE      _handle {INVALID_HANDLE_VALUE};  // Pipe output handle.
        ::HANDLE      _process {INVALID_HANDLE_VALUE}; // Handle to child process.
#else
        ::pid_t       _fpid = 0;                 // Forked process id (UNIX PID, not MPEG PID!)
        int           _fd {-1};                  // Pipe output file descriptor.
#endif
    };
}
