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
#include "tsObject.h"
#include "tsIPUtils.h"
#include "tsSysUtils.h"

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
    //! There are two distinct I/O models:
    //!
    //! - Non-blocking I/O (UNIX).
    //! - Asynchronous I/O (Windows).
    //!
    //! Differences in semantics:
    //!
    //! - On UNIX systems (Linux, macOS, BSD), non-blocking means that a read or write
    //!   operation fails if it cannot be immediately served. The corresponding error is
    //!   EAGAIN.
    //!
    //! - On Windows systems, non-blocking means using "overlapped" I/O. For anyone with a
    //!   basic system culture, this means "asynchronous" I/O. An asynchronous I/O operation
    //!   can either immediately succeed or fail with error ERROR_IO_PENDING, meaning that the
    //!   operation executes in the background.
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
    //! Important differences in canceling I/O and closing file descriptors or handles:
    //!
    //! - On UNIX, a non-blocking I/O is either immediate or failed. No I/O is ever "in progress".
    //!   Closing a file descriptor is possible at any time without restriction.
    //!
    //! - On Windows, a pending I/O can be canceled either explicitly or as the result of closing
    //!   the device handle. In that case, the I/O completion is notified later with an I/O error status.
    //!   This means that the I/O data buffer and IOSB (see below) shall remain valid as long as this
    //!   corresponding I/O completion has not been received. It is of the utmost importance that the
    //!   application keeps track of all pending asynchronous I/O and always waits for the reception of
    //!   all corresponding I/O completions before releasing the memory for the data buffers, including
    //!   when closing the device handle.
    //!
    class TSCOREDLL NonBlockingDevice : public ReporterBase
    {
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit NonBlockingDevice(Report* report = nullptr, bool non_blocking = false) : ReporterBase(report), _is_non_blocking(non_blocking) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //!
        explicit NonBlockingDevice(ReporterBase* delegate, bool non_blocking = false) : ReporterBase(delegate), _is_non_blocking(non_blocking) {}

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
        //! This static method checks if a system error code means "I/O in progress" (asynchronous I/O) or "I/O would block" (non-blocking I/O).
        //! @param [in] error_code System error code.
        //! @return True if @a error_code is an in-progress/would-block one.
        //!
        static bool IsPendingStatus(int error_code);

        //!
        //! This structure indicates the status of a non-blocking I/O.
        //! You must be a former OpenVMS expert to understand the name...
        //!
        //! The subclasses which implement non-blocking I/O should use an IOSB* parameter for any I/O operation
        //! which can be non-blocking. Typically, if the instance of the subclass is in blocking mode (the default),
        //! this parameter must be null. When the instance is in non-blocking mode, this parameter must not be null.
        //!
        //! As a general rule, when an I/O method accepts an IOSB* parameter and the I/O is pending (meaning not possible
        //! now with non-blocking I/O or in progress with asynchronous I/O), the I/O method sets the @a pending field tp
        //! true and returns a success status. The success returned status means that the I/O was successfully started and
        //! the @a pending field set to true means that the I/O is not completed yet.
        //!
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

            //!
            //! Current error code for the I/O.
            //! The value may evolve during the I/O, until completion.
            //!
            int error_code = SYS_SUCCESS;

            //!
            //! When @a pending is true, indicate how many bytes were successfully sent before the I/O would block.
            //! Apply to non-blocking I/O only, not asynchronous I/O.
            //!
            size_t sent_size = 0;

            //!
            //! Pointer to application data.
            //! This is typically set before starting the I/O at application level.
            //!
            std::shared_ptr<Object> app_data {};

            //!
            //! Pointer to reactive I/O data.
            //! This is typically set before starting the I/O at "reactive I/O" level.
            //!
            std::shared_ptr<Object> react_data {};

#if defined(TS_WINDOWS) || defined(DOXYGEN)
            // Memory marker to identify an overlap structure which belongs to an IOSB structure.
            // When an asynchronous I/O returns the address of an OVERLAPPED structure, check if
            // memory before the OVERLAPPED contains some magic marker.
            //! @cond nodoxygen
            volatile uint32_t iosb_marker = iosb_marker_value;
            static constexpr uint32_t iosb_marker_value = 0x494F5342; // "IOSB"
            //! @endcond

            //!
            //! OVERLAPPED structure for an asynchronous I/O (Windows-specific).
            //!
            ::OVERLAPPED overlap {};

            //!
            //! Pointer to asynchronous I/O data (Windows-specific).
            //! This is typically set before starting the asynchronous I/O at socket level.
            //!
            std::shared_ptr<Object> async_data {};

            //!
            //! Check if an OVERLAPPED structure is part of an IOSB.
            //! @param [in] overlap Address of an OVERLAPPED structure.
            //! @return Address of the IOSB which contains the OVERLAPPED or nullptr if @a overlap is not part of an IOSB.
            //!
            static IOSB* ParentIOSB(::OVERLAPPED* overlap);

            //! @cond nodoxygen
            IOSB();
            ~IOSB();
            //! @endcond
#else
            //! @cond nodoxygen
            IOSB() = default;
            //! @endcond
#endif
            //!
            //! Reset the IOSB for a new I/O operation.
            //!
            void reset();
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
        //! Low-level method to set a system file or socket descriptor in non-blocking mode.
        //! @param [in] fd System file or socket descriptor.
        //! @param [in] non_blocking It true, the device is set in non-blocking mode.
        //! @return True on success, false on error.
        //!
        //! Summary: Do not use this method unless you exactly know what you are doing.
        //!
        //! UNIX: Depending on the way a file descriptor is created, it may be possible to specify the non-blocking mode from the
        //! beginning. Or it can be somehow inherited. However, this is not portable.
        //!
        //! Examples:
        //! - On Linux and FreeBSD, a socket can be directly created in non-blocking mode using the flag SOCK_NONBLOCK in the 'type'
        //!   parameter of the socket() system call. However, it does not work on macOS.
        //! - On macOS (and maybe FreeBSD), when a server socket is in non-blocking mode, all client session sockets which are
        //!   created by accept() are also in non-blocking mode. However, on Linux, they are in blocking mode.
        //!
        //! In all cases, it is possible to set a file descriptor in non-blocking mode at any time using the method setSystemNonBlocking().
        //! This method uses fcntl(F_SETFL) to alter the file descriptor's flags.
        //!
        //! Windows: The natural way of not being blocked on I/O on Windows is asynchronous I/O. To increase the general confusion,
        //! there is some form of non-blocking mode on Windows sockets, and only sockets, not other forms of file handles. This mode
        //! is activated using "ioctlsocket(fd, FIONBIO, &mode)". When this mode is active, socket I/O become similar to UNIX: they
        //! immediately either succeed or fail, but never block. However, there is no way to get notified when the I/O becomes possible.
        //! There is no equivalent to epoll (Linux) or kqueue (macOS and BSD). The Windows I/O Completion Ports can only work on
        //! asynchronous I/O, using OVERLAPPED structures. Because this form of non-blocking mode is mostly useless in practice, we do
        //! not use it and the method setSystemNonBlocking() does nothing on Windows.
        //!
        //! @see https://learn.microsoft.com/en-us/archive/blogs/csliu/io-concept-blockingnon-blocking-vs-syncasync
        //!
        bool setSystemNonBlocking(SysSocketType fd, bool non_blocking);

    private:
        bool _is_non_blocking;
    };
}
