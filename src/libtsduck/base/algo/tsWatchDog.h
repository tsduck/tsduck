//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose timeout watchdog.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWatchDogHandlerInterface.h"
#include "tsNullReport.h"
#include "tsThread.h"
#include "tsReport.h"

namespace ts {
    //!
    //! General-purpose timeout watchdog.
    //! @ingroup thread
    //!
    //! The watchdog is initially suspended. A call to restart() reinitializes the timer
    //! and starts the watchdog. After the timeout, the watchdog automatically triggers
    //! a "next input". To avoid this, restart() or suspend() must be called before
    //! the timeout expires.
    //!
    //! A WatchDog instance contains an internal thread which is started in the constructor
    //! and terminated in the destructor.
    //!
    class TSDUCKDLL WatchDog : private Thread
    {
        TS_NOCOPY(WatchDog);
    public:
        //!
        //! Constructor.
        //! @param [in] handler Initial handler to call at expiration of the watchdog timeout.
        //! @param [in] timeout Initial watchdog timeout in milliseconds. Zero means no timeout.
        //! @param [in] id Application-defined watchdog id to assign.
        //! This value is chosen and set by the application.
        //! It can be retrieved later if a handler is used by several watchdogs.
        //! The id is not interpreted by the watchdog, it is only stored for the application.
        //! @param [in,out] log Log report.
        //!
        WatchDog(WatchDogHandlerInterface* handler = nullptr, MilliSecond timeout = 0, int id = 0, Report& log = NULLREP);

        //!
        //! Destructor.
        //!
        virtual ~WatchDog() override;

        //!
        //! Set a new timeout value.
        //! @param [in] timeout New watchdog timeout in milliseconds. Zero means no timeout.
        //! @param [in] autoStart If true and @a timeout is non zero, the timeout is
        //! automatically started. Otherwise, it is suspended. The previous timeout,
        //! if active, is automatically canceled.
        //!
        void setTimeout(MilliSecond timeout, bool autoStart = false);

        //!
        //! Restart the watchdog, the previous timeout is canceled.
        //!
        void restart();

        //!
        //! Suspend the watchdog, the previous timeout is canceled.
        //!
        void suspend();

        //!
        //! Replace the watchdog handler.
        //! @param [in] h The new handler. Can be zero.
        //!
        void setWatchDogHandler(WatchDogHandlerInterface* h);

        //!
        //! Set some arbitrary "watchdog id" value.
        //! @param [in] id Application-defined watchdog id to assign.
        //!
        void setWatchDogId(int id) { _watchDogId = id; }

        //!
        //! Get the "watchdog id" value, as previously stored by the application.
        //! @return The application-defined watchdog id.
        //!
        int watchDogId() const { return _watchDogId; }

    private:
        Report&                   _log;                // For debug messages.
        volatile int              _watchDogId = 0;     // Application-defined watchdog identifier.
        volatile bool             _terminate = false;  // Terminate the thread.
        std::mutex                _mutex {};           // Mutex to protect the following fields.
        std::condition_variable   _condition {};       // Condition to signal when something changed.
        WatchDogHandlerInterface* _handler = nullptr;  // Handler for expiration.
        MilliSecond               _timeout = 0;        // Expiration timeout, 0 means no timeout.
        bool                      _active = false;     // The watchdog is active.
        bool                      _started = false;    // The thread is started.

        // Activate the watchdog. Must be called with mutex held.
        void activate();

        // Implementation of Thread.
        virtual void main() override;
    };
}
