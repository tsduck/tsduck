//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Encapsulate a system pipe, UNIX or Windows.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsEnumUtils.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Encapsulate a system pipe, UNIX or Windows.
    //! @ingroup libtscore system
    //!
    //! A pipe has two file descriptors / system handles, one for the write end, one for the read end.
    //! This class is useful to create pipes, fetch "useful" file descriptors (e.g. to pass to a child
    //! process), and let the unused file descriptors be closed in the destructor. This is a safe way
    //! to never forget to close unused file descriptors, regardless of error patch.
    //!
    //! Once open, the file descriptors / system handles can be either "fetched" or "peeked".
    //! - When the application "fetches" a file descriptor, the responsibility of the file descriptor is
    //!   passed to the application. The SysPipe object no longer knows it and it won't be closed by the
    //!   SysPipe object. This is typically what should be done when an end of the pipe shall be transmitted
    //!   to a child process.
    //! - When the application "peeks" a file descriptor, its value is returned but it remains under the
    //!   responsibility of the SysPipe object and will be closed with the SysPipe object.
    //!
    //! Windows specificities
    //! ---------------------
    //! On Windows, asynchronous I/O are not supported in anonymous pipes (as created by CreatePipe).
    //! - https://learn.microsoft.com/en-us/windows/win32/ipc/anonymous-pipe-operations
    //! - https://stackoverflow.com/a/419736
    //! Overlapped I/O can be used on named pipes only.
    //! - https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-overlapped-i-o
    //! - https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client
    //! 
    class TSCOREDLL SysPipe: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(SysPipe);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit SysPipe(Report* report) : ReporterBase(report) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit SysPipe(ReporterBase* delegate) : ReporterBase(delegate) {}

        //!
        //! Virtual destructor.
        //! The file descriptor which were not "fetched" are closed.
        //!
        virtual ~SysPipe() override;

        //!
        //! Pipe creation flags.
        //! 
        enum Flags {
            NONE          = 0x0000,  //!< No creation flag, use all default.
            READ_ASYNC    = 0x0001,  //!< Set the read end in non-blocking / asynchronous mode.
            WRITE_ASYNC   = 0x0002,  //!< Set the write end in non-blocking / asynchronous mode.
            READ_CLOEXEC  = 0x0004,  //!< Set the flag "close on exe" on the read end (UNIX only).
            WRITE_CLOEXEC = 0x0008,  //!< Set the flag "close on exe" on the write end (UNIX only).
            READ_INHERIT  = 0x0010,  //!< The read end must be inherited in children processes (Windows only).
            WRITE_INHERIT = 0x0020,  //!< The write end must be inherited in children processes (Windows only).
        };

        //!
        //! Create the pipe and open the two file descriptors.
        //! @param [in] flags Creation flags.
        //! @param [in] buffer_size The pipe buffer size in bytes, on Windows. Zero means default. Ignored on UNIX.
        //! @return True on success, false on error.
        //!
        bool create(Flags flags = NONE, size_t buffer_size = 0);

        //!
        //! Close the pipe file descriptors which are not "fetched".
        //! @param [in] silent If true, do not report errors.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! "Peek" the read file descriptor of the pipe.
        //! The file descriptor remains in this object and will be closed when this object is closed.
        //! @return The read file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType peekRead() { return _read_hfd; }

        //!
        //! "Peek" the write file descriptor of the pipe.
        //! The file descriptor remains in this object and will be closed when this object is closed.
        //! @return The write file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType peekWrite() { return _write_hfd; }

        //!
        //! "Fetch" the read file descriptor of the pipe.
        //! The file descriptor is then removed from this object and will not be closed when this object is closed.
        //! This is typically used when we need the file descriptor to keep it open for later usage.
        //! When there is no read file descriptor in this object, an error is reported.
        //! @return The read file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType fetchRead();

        //!
        //! "Fetch" the write file descriptor of the pipe.
        //! The file descriptor is then removed from this object and will not be closed when this object is closed.
        //! This is typically used when we need the file descriptor to keep it open for later usage.
        //! When there is no write file descriptor in this object, an error is reported.
        //! @return The write file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType fetchWrite();

    private:
        SysHandleType _read_hfd = SYS_HANDLE_INVALID;
        SysHandleType _write_hfd = SYS_HANDLE_INVALID;

        static constexpr size_t PIPE_COUNT = 2;    // Number of file descriptors in a pipe.
        static constexpr size_t PIPE_READFD = 0;   // Index of pipe read file descriptor, UNIX order.
        static constexpr size_t PIPE_WRITEFD = 1;  // Index of pipe write file descriptor, UNIX order.
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::SysPipe::Flags);
