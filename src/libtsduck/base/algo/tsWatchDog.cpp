//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWatchDog.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::WatchDog::WatchDog(WatchDogHandlerInterface* handler, cn::milliseconds timeout, int id, Report& log) :
    _log(log),
    _watchDogId(id),
    _handler(handler),
    _timeout(timeout)
{
}

ts::WatchDog::~WatchDog()
{
    // Terminate the thread and wait for actual thread termination.
    // Does nothing if the thread has not been started.
    _terminate = true;
    _condition.notify_all();
    waitForTermination();
}


//----------------------------------------------------------------------------
// Replace the watchdog handler.
//----------------------------------------------------------------------------

void ts::WatchDog::setWatchDogHandler(WatchDogHandlerInterface* h)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _handler = h;
}


//----------------------------------------------------------------------------
// Activate the watchdog. Must be called with mutex held.
//----------------------------------------------------------------------------

void ts::WatchDog::activate()
{
    if (_started) {
        // Watchdog thread already started, signal the condition.
        _condition.notify_all();
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

void ts::WatchDog::setTimeout(cn::milliseconds timeout, bool autoStart)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _timeout = timeout;
    _active = autoStart;
    if (autoStart) {
        activate();
    }
}


//----------------------------------------------------------------------------
// Restart the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::restart()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _active = true;
    activate();
}


//----------------------------------------------------------------------------
// Suspend the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::suspend()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _active = false;
    // Signal the condition if the thread is started.
    // No need to activate the thread if not started.
    _condition.notify_all();
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::WatchDog::main()
{
    _log.debug(u"Watchdog thread started, id %d", _watchDogId);

    while (!_terminate) {
        bool expired = false;
        WatchDogHandlerInterface* h = nullptr;

        // Wait for the condition to be signaled. Get protected data while under mutex protection.
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (!_active || _timeout <= cn::milliseconds::zero()) {
                _condition.wait(lock);
            }
            else {
                expired = _condition.wait_for(lock, _timeout) == std::cv_status::timeout;
            }
            h = _handler;
        }

        // Handle the expiration. No longer under mutex protection to avoid deadlocks in handler.
        if (!_terminate && expired && h != nullptr) {
            _log.debug(u"Watchdog expired, id %d", _watchDogId);
            h->handleWatchDogTimeout(*this);
        }
    }

    _log.debug(u"Watchdog thread completed, id %d", _watchDogId);
}
