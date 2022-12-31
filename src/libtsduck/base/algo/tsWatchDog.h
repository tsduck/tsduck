//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  General-purpose timeout watchdog.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWatchDogHandlerInterface.h"
#include "tsNullReport.h"
#include "tsThread.h"
#include "tsReport.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuardCondition.h"

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
        Report&                   _log;         // For debug messages.
        volatile int              _watchDogId;  // Application-defined watchdog identifier.
        volatile bool             _terminate;   // Terminate the thread.
        Mutex                     _mutex;       // Mutex to protect the following fields.
        Condition                 _condition;   // Condition to signal when something changed.
        WatchDogHandlerInterface* _handler;     // Handler for expiration.
        MilliSecond               _timeout;     // Expiration timeout, 0 means no timeout.
        bool                      _active;      // The watchdog is active.
        bool                      _started;     // The thread is started.

        // Activate the watchdog. Must be called with mutex held.
        void activate(GuardCondition& lock);

        // Implementation of Thread.
        virtual void main() override;
    };
}
