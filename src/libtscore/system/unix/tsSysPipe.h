//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtscore unix
//!  Encapsulate a UNIX pipe.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Encapsulate a UNIX pipe.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL SysPipe: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(SysPipe);
    public:
        static constexpr size_t PIPE_COUNT   = 2;  //!< Number of file descriptors in a pipe.
        static constexpr size_t PIPE_READFD  = 0;  //!< Index of pipe read file descriptor.
        static constexpr size_t PIPE_WRITEFD = 1;  //!< Index of pipe write file descriptor.

        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit SysPipe(Report* report, Object* owner = nullptr) : ReporterBase(report, owner) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit SysPipe(ReporterBase* delegate, Object* owner = nullptr) : ReporterBase(delegate, owner) {}

        //!
        //! Virtual destructor.
        //! The file descriptor which were not "fetched" are closed.
        //!
        virtual ~SysPipe() override;

        //!
        //! Create the pipe and open the two file descriptors.
        //! @return True on success, false on error.
        //!
        bool create();

        //!
        //! Close the pipe file descriptors which are not "fetched".
        //! @param [in] silent If true, do not report errors.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! Get the read file descriptor of the pipe.
        //! The file descriptor remains in this object and will be closed when this object is closed.
        //! @return The read file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType getRead() { return _fd[PIPE_READFD]; }

        //!
        //! Get the write file descriptor of the pipe.
        //! The file descriptor remains in this object and will be closed when this object is closed.
        //! @return The write file descriptor of the pipe or SYS_HANDLE_INVALID if there is none.
        //!
        SysHandleType getWrite() { return _fd[PIPE_WRITEFD]; }

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
        int _fd[PIPE_COUNT] {SYS_HANDLE_INVALID, SYS_HANDLE_INVALID};
    };
}
