//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Define which kind of support is provided for reactors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPUtils.h"

//
// Depending on the operating system, we use distinct forms of kernel event queues.
// This is not simply an internal detail of the Reactor class. It influences the
// way "reactive classes" will interact with a Reactor regarding I/O's.
//
#if !defined(DOXYGEN)
    #if defined(TS_LINUX)
        // Use epoll(), a Linux specific feature.
        // In the future, we may consider io_uring, although it may be overkill for small servers.
        #define TS_USE_EPOLL 1
    #elif defined(TS_MAC) || defined(TS_BSD)
        // Use kqueue(), as found on macOS and all BSD systems.
        #define TS_USE_KQUEUE 1
    #elif defined(TS_WINDOWS)
        // Use I/O completion ports, a Windows feature.
        #define TS_USE_IOCP 1
    #else
        #error "Reactor is not supported on this operating system"
    #endif
#endif

#if defined(TS_USE_EPOLL) || defined(TS_USE_KQUEUE) || defined(DOXYGEN)
    //!
    //! This macro is defined when the Reactor uses a non-blocking I/O model.
    //!
    //! Reactive classes which manage I/O shall repeatedly attempt I/O operations as long
    //! as they succeed. When they fail with a "would block" status, the reactive class
    //! shall request the Reactor to be notified when the I/O becomes possible.
    //!
    //! The macros TS_USE_NON_BLOCKING_IO and TS_USE_ASYNCHRONOUS_IO should be used only
    //! when contitional compilation is required for syntactic reasons. A reactive I/O
    //! class should use "if constexpr" structures using the static methods UseNonBlockingIO()
    //! and UseAsynchronousIO().
    //!
    //! @see TS_USE_ASYNCHRONOUS_IO
    //! @see ts::ReactorSupport::UseNonBlockingIO()
    //! @see ts::ReactorSupport::UseAsynchronousIO()
    //! @see ts::NonBlockingDevice
    //!
    #define TS_USE_NON_BLOCKING_IO 1
#endif

#if defined(TS_USE_IOCP) || defined(DOXYGEN)
    //!
    //! This macro is defined when the Reactor uses an asynchronous I/O model.
    //!
    //! Reactive classes which manage I/O shall start I/O operations and, if the operation
    //! completes with a "pending" status, the reactive class shall request the Reactor to
    //! be notified when the I/O completes. In the meantime, the reactive class shall ensure
    //! that the I/O buffers remain valid, as they are used in the background by the I/O.
    //!
    //! The macros TS_USE_NON_BLOCKING_IO and TS_USE_ASYNCHRONOUS_IO should be used only
    //! when contitional compilation is required for syntactic reasons. A reactive I/O
    //! class should use "if constexpr" structures using the static methods UseNonBlockingIO()
    //! and UseAsynchronousIO().
    //!
    //! @see TS_USE_NON_BLOCKING_IO
    //! @see ts::ReactorSupport::UseNonBlockingIO()
    //! @see ts::ReactorSupport::UseAsynchronousIO()
    //! @see ts::NonBlockingDevice
    //!
    #define TS_USE_ASYNCHRONOUS_IO 1
#endif

namespace ts {
    //!
    //! Define which kind of implementation and support are provided for reactors.
    //! @ingroup libtscore reactor
    //!
    //! This class contains static methods only, some of them being consteval (compilation-level checks).
    //!
    class TSCOREDLL ReactorSupport
    {
    public:
        //!
        //! This static function returns whether the Reactor uses an asynchronous I/O model.
        //! @return True when asynchronous I/O are used, false when non-blocking I/O are used.
        //!
        //! Reactive classes which manage I/O shall start I/O operations and, if the operation
        //! completes with a "pending" status, the reactive class shall request the Reactor to
        //! be notified when the I/O completes. In the meantime, the reactive class shall ensure
        //! that the I/O buffers remain valid, as they are used in the background by the I/O.
        //!
        //! This method is typically used in "if constexpr" structures, which are preferred to
        //! conditional compilation using the macro TS_USE_ASYNCHRONOUS_IO.
        //!
        //! Example:
        //! @code
        //! if constexpr (ReactorSupport::UseAsynchronousIO()) {
        //!     ....
        //! }
        //! @endcode
        //! @see UseNonBlockingIO()
        //! @see NonBlockingDevice
        //!
        static consteval bool UseAsynchronousIO()
        {
#if defined(TS_USE_ASYNCHRONOUS_IO) && !defined(TS_USE_NON_BLOCKING_IO)
            return true;
#elif defined(TS_USE_NON_BLOCKING_IO) && !defined(TS_USE_ASYNCHRONOUS_IO)
            return false;
#else
    #error "invalid asynchronous vs. non-blocking configuration"
#endif
        }

        //!
        //! This static function returns whether the Reactor uses a non-blocking I/O model.
        //! @return True when non-blocking I/O are used, false when asynchronous I/O are used.
        //!
        //! Reactive classes which manage I/O shall repeatedly attempt I/O operations as long
        //! as they succeed. When they fail with a "would block" status, the reactive class
        //! shall request the Reactor to be notified when the I/O becomes possible.
        //!
        //! This method is typically used in "if constexpr" structures, which are preferred to
        //! conditional compilation using the macro TS_USE_NON_BLOCKING_IO.
        //!
        //! Example:
        //! @code
        //! if constexpr (ReactorSupport::UseNonBlockingIO()) {
        //!     ....
        //! }
        //! @endcode
        //! @see UseAsynchronousIO()
        //! @see NonBlockingDevice
        //!
        static consteval bool UseNonBlockingIO() { return !UseAsynchronousIO(); }

        //!
        //! Check if a given file descriptor or handle is supported by the reactor.
        //!
        //! On some operating systems, some types of devices do not support asynchronous or non-blocking I/O
        //! and cannot be used in a reactor. The most annoying case is regular files on Linux. Using a regular
        //! file with epoll on Linux ends with EPERM because regular files are considered as "always ready".
        //! On macOS and FreeBSD, regular files are also "always ready" but they can be used by kqueue, which
        //! means that the code can be generic, which is not the case on Linux.
        //!
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return True if @a sock supports asynchronous or non-blocking I/O on this operating system, false otherwise.
        //!
        static bool SupportedDevice(SysSocketType sock);
    };
}
