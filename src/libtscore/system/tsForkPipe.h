//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Fork a process and create optional pipes to its standard input/output.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNonBlockingDevice.h"
#include "tsStreamInterface.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Fork a process and create optional pipes to its standard input/output.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL ForkPipe: public NonBlockingDevice, public StreamInterface
    {
        TS_NOBUILD_NOCOPY(ForkPipe);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the file is initially set in non-blocking mode.
        //!
        explicit ForkPipe(Report* report, bool non_blocking = false);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the file is initially set in non-blocking mode.
        //!
        explicit ForkPipe(ReporterBase* delegate, bool non_blocking = false);

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
        enum InputMode {
            STDIN_PARENT,  //!< Keep same stdin as current (parent) process.
            STDIN_PIPE,    //!< Use a pipe as stdin.
            STDIN_NONE,    //!< No standard input (the null device in fact).
        };

        //!
        //! How to merge standard output and standard error in the created process.
        //!
        enum OutputMode {
            KEEP_BOTH,       //!< Keep same stdout and stderr as current (parent) process.
            STDOUT_ONLY,     //!< Merge stderr into current stdout.
            STDERR_ONLY,     //!< Merge stdout into current stderr.
            STDOUT_PIPE,     //!< Use a pipe to receive stdout, keep same stderr as current (parent) process.
            STDOUTERR_PIPE,  //!< Use a pipe to receive a merge of stdout and stderr.
        };

        //!
        //! Create the process, open the optional pipes.
        //! @param [in] command The command to execute.
        //! @param [in] wait_mode How to wait for process termination in close().
        //! @param [in] buffer_size The pipes buffer size in bytes. Used on Windows only. Zero means default.
        //! @param [in] out_mode How to handle stdout and stderr.
        //! @param [in] in_mode How to handle stdin.
        //! @return True on success, false on error. Do not return on success when @a wait_mode is EXIT_PROCESS.
        //!
        bool open(const UString& command, WaitMode wait_mode, size_t buffer_size, OutputMode out_mode, InputMode in_mode);

        //!
        //! Close the pipe.
        //! Optionally wait for process termination if @a wait_mode was SYNCHRONOUS on open().
        //! @param [in] silent If true, do not report errors. This is typically useful when the object is in some error
        //! condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool close(bool silent = false);

        //!
        //! Check if the process is running and the pipe is open (when used).
        //! @return True if the process is running and the pipe is open.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Check if the pipe was broken.
        //! @return True if was broken (unexpected process termination for instance).
        //!
        bool isBroken() const { return _broken_pipe; }

        //!
        //! Check if synchronous mode is active (ie. will wait for process termination).
        //! @return True if synchronous mode is active.
        //!
        bool isSynchronous() const { return _wait_mode == SYNCHRONOUS; }

        //!
        //! Get the created process id.
        //! @return The process id or SYS_PROCESS_ID_INVALID in case of error.
        //!
        SysProcessIdType getProcessId() const { return _fpid; }

        //!
        //! Get the created process handle.
        //! The concept of process handle exists on Windows only.
        //! @return The process id or SYS_HANDLE_INVALID in case of error or when process handles are not supported.
        //!
        SysHandleType getProcessHandle() const { return _process; }

        //!
        //! Set "ignore abort".
        //! @param [in] on If true and the process aborts, do not report error when writing data.
        //! when writing data.
        //!
        void setIgnoreAbort(bool on) { _ignore_abort = on; }

        //!
        //! Get "ignore abort".
        //! @return True if, when the process aborts, do not report error when writing data.
        //!
        bool getIgnoreAbort() const { return _ignore_abort; }

        //!
        //! Abort any currenly input/output operation in the pipe.
        //! The pipe is left in a broken state and can be only closed.
        //!
        void abortPipeReadWrite();

        //!
        //! This static method launches a command, without pipe, optionally without waiting for the completion of the command process.
        //! @param [in] command The command to execute.
        //! @param [in,out] report Where to report errors.
        //! @param [in] out_mode How to handle stdout and stderr. Must be KEEP_BOTH (default), STDOUT_ONLY or STDERR_ONLY. Output modes using pipes are forbidden.
        //! @param [in] in_mode How to handle stdin. Must be STDIN_PARENT (default) or STDIN_NONE. Input modes using pipes are forbidden.
        //! @param [in] wait_mode How to wait for the command process. Must be ASYNCHRONOUS (default) or SYNCHRONOUS.
        //! @return True on success, false on error.
        //!
        static bool Launch(const UString& command,
                           Report& report,
                           OutputMode out_mode = KEEP_BOTH,
                           InputMode in_mode = STDIN_PARENT,
                           WaitMode wait_mode = ASYNCHRONOUS);

        //!
        //! This static method launches a command and gets its output as text.
        //! @param [in] output The output of the command.
        //! @param [in] command The command to execute.
        //! @param [in,out] report Where to report errors.
        //! @param [in] include_stderr If false, the standard error of the command is the same as the parent process.
        //! If true, the standard error is merged with the standard output in @a output.
        //! @return True on success, false on error.
        //!
        static bool GetOutput(UString& output, const UString& command, Report& report, bool include_stderr = false);

        // Implementation of NonBlockingDevice.
        virtual SysHandleType getReadHandle() const override;
        virtual SysHandleType getWriteHandle() const override;

        // Implementation of StreamInterface.
        virtual bool readStream(void* addr, size_t size, const AbortInterface* abort = nullptr) override;
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, IOSB* iosb = nullptr) override;
        virtual bool asyncCompletedStream(IOSB* iosb) override;
        virtual bool isReadStream() override;
        virtual bool isWriteStream() override;
        virtual bool endOfStream() override;

    protected:
        // Inherited methods.
        virtual bool allowSetNonBlocking() const override;

    private:
        InputMode        _in_mode = STDIN_NONE;           // Input mode for the created process.
        OutputMode       _out_mode = KEEP_BOTH;           // Output mode for the created process.
        volatile bool    _is_open = false;                // Open and running.
        WaitMode         _wait_mode = ASYNCHRONOUS;       // How to wait for child process termination in close().
        bool             _in_pipe = false;                // The process uses an input pipe.
        bool             _out_pipe = false;               // The process uses an output pipe.
        bool             _ignore_abort = false;           // Ignore early termination of child process.
        volatile bool    _broken_pipe = false;            // Output pipe is broken, do not attempt to write.
        volatile bool    _eof = false;                    // Got end of file on input pipe.
        SysHandleType    _read_hfd = SYS_HANDLE_INVALID;  // File descriptor to read (output of process).
        SysHandleType    _write_hfd = SYS_HANDLE_INVALID; // File descriptor to write (input of process).
        SysProcessIdType _fpid = SYS_PROCESS_ID_INVALID;  // Forked process id.
        SysHandleType    _process = SYS_HANDLE_INVALID;   // Handle to child process (Windows only).
    };
}
