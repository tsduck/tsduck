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

#include "tsWatchDog.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::WatchDog::WatchDog(ts::WatchDogHandlerInterface* handler, ts::MilliSecond timeout, int id, ts::Report& log) :
    _log(log),
    _watchDogId(id),
    _terminate(false),
    _mutex(),
    _condition(),
    _handler(handler),
    _timeout(timeout == 0 ? Infinite : timeout),
    _active(false),
    _started(false)
{
}

ts::WatchDog::~WatchDog()
{
    // Terminate the thread and wait for actual thread termination.
    // Does nothing if the thread has not been started.
    _terminate = true;
    _condition.signal();
    waitForTermination();
}


//----------------------------------------------------------------------------
// Replace the watchdog handler.
//----------------------------------------------------------------------------

void ts::WatchDog::setWatchDogHandler(WatchDogHandlerInterface* h)
{
    GuardMutex lock(_mutex);
    _handler = h;
}


//----------------------------------------------------------------------------
// Activate the watchdog. Must be called with mutex held.
//----------------------------------------------------------------------------

void ts::WatchDog::activate(GuardCondition& lock)
{
    if (_started) {
        // Watchdog thread already started, signal the condition.
        lock.signal();
    }
    else {
        // Start the watchdog thread.
        _started = true;
        Thread::start();
    }
}


//----------------------------------------------------------------------------
// Set a new timeout value.
//----------------------------------------------------------------------------

void ts::WatchDog::setTimeout(MilliSecond timeout, bool autoStart)
{
    GuardCondition lock(_mutex, _condition);
    _timeout = timeout == 0 ? Infinite : timeout;
    _active = autoStart;
    if (autoStart) {
        activate(lock);
    }
}


//----------------------------------------------------------------------------
// Restart the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::restart()
{
    GuardCondition lock(_mutex, _condition);
    _active = true;
    activate(lock);
}


//----------------------------------------------------------------------------
// Suspend the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::suspend()
{
    GuardCondition lock(_mutex, _condition);
    _active = false;
    // Signal the condition if the thread is started.
    // No need to activate the thread if not started.
    lock.signal();
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::WatchDog::main()
{
    _log.debug(u"Watchdog thread started, id %d", {_watchDogId});

    while (!_terminate) {
        bool expired = false;
        WatchDogHandlerInterface* h = nullptr;

        // Wait for the condition to be signaled. Get protected data while under mutex protection.
        {
            GuardCondition lock(_mutex, _condition);
            expired = !lock.waitCondition(_active ? _timeout : Infinite);
            h = _handler;
        }

        // Handle the expiration. No longer under mutex protection to avoid deadlocks in handler.
        if (!_terminate && expired && h != nullptr) {
            _log.debug(u"Watchdog expired, id %d", {_watchDogId});
            h->handleWatchDogTimeout(*this);
        }
    }

    _log.debug(u"Watchdog thread completed, id %d", {_watchDogId});
}
