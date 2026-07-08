//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  All interface classes which are used as event handlers in a Reactor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEventId.h"
#include "tsNonBlockingDevice.h"

namespace ts {

    class Reactor;

    //!
    //! Interface class for Reactor handlers.
    //! All methods are empty by default. An application may implement the required ones only.
    //!
    class TSCOREDLL ReactorHandlerInterface
    {
        TS_INTERFACE(ReactorHandlerInterface);
    public:
        //!
        //! Handle a timer in a Reactor.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] id Id of the timer which expires.
        //!
        virtual void handleTimer(Reactor& reactor, EventId id);

        //!
        //! Handle a user-defined event in a Reactor.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] id Id of the event which was signaled.
        //!
        virtual void handleUserEvent(Reactor& reactor, EventId id);

        //!
        //! Handle a broadcast event in a Reactor.
        //! A broadcast event is sent to all currently registered events in the reactor.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] error_code Application-specific error code which was passed to Reactor::signalBroadcastEvent().
        //! @param [in] user_data The user-data shared pointer which was passed to Reactor::signalBroadcastEvent().
        //!
        virtual void handleBroadcastEvent(Reactor& reactor, int error_code, const ObjectPtr& user_data);

        //!
        //! Handle a read-ready event in a Reactor.
        //! This handler is only invoked in the non-blocking I/O model.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] id Id of the event which was signaled.
        //! @param [in] error_code System-specific error code, zero on success, SYS_ERROR in case of unknown error.
        //!
        virtual void handleReadReady(Reactor& reactor, EventId id, int error_code);

        //!
        //! Handle a write-ready event in a Reactor.
        //! This handler is only invoked in the non-blocking I/O model.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] id Id of the event which was signaled.
        //! @param [in] error_code System-specific error code, zero on success, SYS_ERROR in case of unknown error.
        //!
        virtual void handleWriteReady(Reactor& reactor, EventId id, int error_code);

        //!
        //! Handle an asynchronous I/O completion event in a Reactor.
        //! This handler is only invoked in the asynchronous I/O model.
        //! @param [in,out] reactor Reactor into which the handler is invoked.
        //! @param [in] id Id of the event which was signaled.
        //! @param [in,out] iosb IOSB structure which was used when the asynchronous I/O was started.
        //! A system-specific error code is in @a iosb, SYS_CANCELED if the I/O was canceled before completion.
        //! @param [in] io_size Size of the I/O in bytes.
        //!
        virtual void handleAsynchronousIO(Reactor& reactor, EventId id, NonBlockingDevice::IOSB& iosb, size_t io_size);
    };
}
