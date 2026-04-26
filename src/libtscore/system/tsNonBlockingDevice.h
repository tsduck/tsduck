//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for devices which can work in non-blocking mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsIPUtils.h"

namespace ts {
    //!
    //! Base class for devices, files, or sockets which can work in non-blocking mode.
    //! @ingroup libtscore system
    //!
    //! The methods from this class should not be used by applications. They should be
    //! used only by "reactive classes", which work in combination with an event dispatcher.
    //!
    //! The exact meaning of "non-blocking" depends on the type of device and the
    //! operating system. This is why this class shall be used by specialized classes
    //! which exactly know what they are doing.
    //!
    //! Differences in semantics:
    //! 
    //! - On UNIX systems (Linux, macOS, BSD), non-blocking means that a read or write
    //!   operation fails if it cannot be immediately served. The corresponding error is
    //!   EAGAIN.
    //! - On Windows systems, non-blocking means using "overlapped" I/O. For anyone with a
    //!   basic system culture, this means "asynchronous" I/O.
    //!
    //! Differences in usage:
    //!
    //! - Execution logic: On UNIX systems, an event dispatcher notifies the applications when an
    //!   I/O may be "possible". When notified, the application shall repeatedly read or write
    //!   all possible data, until an EAGAIN error indicates that no more I/O is immediately possible.
    //!   On Windows systems, the application starts an I/O and control immediately returns.
    //!   The I/O data exchange continues in the background. The event dispatcher notifies the
    //!   application when the I/O completes.
    //! 
    //! - Data buffer usage: On UNIX, an I/O is either immediate or failed. On Windows, the
    //!   I/O is in progress as long as it is not completed. This means that, on Windows,
    //!   I/O buffers which are used for read or write must be available during the asynchronous
    //!   phase of the I/O, because the data can come in or go out at any time. On UNIX systems,
    //!   on the contrary, no buffer is used while waiting for some I/O to become possible.
    //!
    //!
    class TSCOREDLL NonBlockingDevice : public ReporterBase
    {
    private:
        NonBlockingDevice() = delete;
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit NonBlockingDevice(Report* report, bool non_blocking = false) : ReporterBase(report), _is_non_blocking(non_blocking) {}

        //!
        //! Destructor.
        //!
        virtual ~NonBlockingDevice() override;

        //!
        //! Set the device in non-blocking mode.
        //!
        //! Important: Usually, this method must be called before opening the device, whatever it means.
        //! Otherwise it is ignored and the device blocking mode is unchanged.
        //!
        //! @param [in] non_blocking It true, the device is set in non-blocking mode.
        //! @return True on success, false if the device is already open and the
        //! non-blocking mode is unchanged.
        //!
        bool setNonBlocking(bool non_blocking);

        //!
        //! Check if the device is in non-blocking mode.
        //! @return True if the device is in non-blocking mode, false otherwise.
        //! @see setNonBlocking()
        //!
        bool isNonBlocking() const { return _is_non_blocking; }

        //!
        //! This structure indicates the status of a non-blocking I/O.
        //! You must be a former OpenVMS expert to understand the name...
        //! 
        //! The subclasses which implement non-blocking I/O should use an IOSB* parameter for any I/O operation
        //! which can be non-blocking. Typically, if the instance of the subclass is in blocking mode (the default),
        //! this parameter must be null. When the instance is in non-blocking mode, this parameter must not be null.
        //! @see checkNonBlocking()
        //!
        class TSCOREDLL IOSB
        {
        public:
            //!
            //! This boolean indicates if the I/O is pending.
            //! - UNIX systems: The I/O would block and was not started. Retry later.
            //! - Windows systems: The I/O is started and will complete later.
            //!   Use the field @a overlap to synchronize on the I/O completion.
            //!
            bool pending = false;
#if defined(TS_WINDOWS) || defined(DOXYGEN)
            //!
            //! OVERLAPPED structure for an asynchronous I/O on Windows.
            //!
            OVERLAPPED overlap {};
#endif
        };

    protected:
        //!
        //! Check the blocking mode of a device.
        //! Called by subclass methods which are explicitly called in blocking or non-blocking mode.
        //! @param [in] non_blocking The required non-blocking mode.
        //! @param [in] opname Name of the operation, for the error message.
        //! @return True on success, false on error.
        //!
        bool checkNonBlocking(bool non_blocking, const UChar* opname);

        //!
        //! Check the blocking mode of a device.
        //! Called by subclass methods which are explicitly called in blocking or non-blocking mode.
        //! @param [in,out] iosb Address of an IOSB structure. If non-null, we are in non-blocking mode.
        //! When null, we are in blocking mode. When non-null, @a pending is reset to false and @a overlap
        //! is zeroed.
        //! @param [in] opname Name of the operation, for the error message.
        //! @return True on success, false on error.
        //!
        bool checkNonBlocking(IOSB* iosb, const UChar* opname);

        //!
        //! Check that the non-blocking mode can be set.
        //! Must be implemented by subclasses which do not support setting the non-blocking
        //! in certain states, such as after being opened. The default implementation always
        //! allows setting the non-blocking mode.
        //! @return True if setting the non-blocking mode is allowed, false otherwise.
        //!
        virtual bool allowSetNonBlocking() const;

        //!
        //! Convenience method to set a system file descriptor or socket handle in non-blocking mode.
        //! @param [in] fd System file descriptor (UNIX) or socket handle (Windows). On Windows,
        //! non-socket devices shall be opened with flag FILE_FLAG_OVERLAPPED instead of using this method.
        //! @param [in] non_blocking It true, the device is set in non-blocking mode.
        //! @return True on success, false on error.
        //!
        bool setSystemNonBlocking(SysSocketType fd, bool non_blocking);

    private:
        bool _is_non_blocking;
    };
}
