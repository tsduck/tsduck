//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::WatchDog::WatchDog(ts::WatchDogHandlerInterface* handler, ts::MilliSecond timeout, int id, ts::Report& log) :
    _log(log),
    _watchDogId(id),
    _mutex(),
    _condition(),
    _handler(handler),
    _command(NONE),
    _timeout(timeout == 0 ? Infinite : timeout)
{
    // Start the thread.
    start();
}

ts::WatchDog::~WatchDog()
{
    // Terminate the thread and wait for actual thread termination.
    sendCommand(TERMINATE);
    waitForTermination();
}


//----------------------------------------------------------------------------
// Replace the watchdog handler.
//----------------------------------------------------------------------------

void ts::WatchDog::setWatchDogHandler(WatchDogHandlerInterface* h)
{
    Guard lock(_mutex);
    _handler = h;
}


//----------------------------------------------------------------------------
// Set a new timeout value.
//----------------------------------------------------------------------------

void ts::WatchDog::setTimeout(MilliSecond timeout, bool autoStart)
{
    GuardCondition lock(_mutex, _condition);

    // No need to shake the threads if there is nothing to do.
    if (_timeout != Infinite || (timeout != Infinite && timeout != 0)) {

        // Update the timeout value.
        _timeout = timeout == 0 ? Infinite : timeout;

        // Signal the thread to either restart or suspend.
        // Do not overwrite any TERMINATE command, this is the last one.
        if (_command != TERMINATE) {
            _command = autoStart ? RESTART : SUSPEND;
            lock.signal();
        }
    }
}


//----------------------------------------------------------------------------
// Send a command to the thread.
//----------------------------------------------------------------------------

void ts::WatchDog::sendCommand(Command cmd)
{
    GuardCondition lock(_mutex, _condition);

    // Do not overwrite any TERMINATE command, this is the last one.
    if (_command != TERMINATE) {
        _command = cmd;
        lock.signal();
    }
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::WatchDog::main()
{
    _log.debug(u"Watchdog thread started, id %d", {_watchDogId});

    MilliSecond currentTimeout = Infinite;

    // Loop on commands.
    GuardCondition lock(_mutex, _condition);
    for (;;) {
        // Release the mutex and wait until a command is received.
        // The mutex is automatically re-acquired.
        const bool expired = !lock.waitCondition(currentTimeout);

        // Exit thread on terminate command.
        if (_command == TERMINATE) {
            break;
        }

        // Handle the expiration.
        if (expired && _handler != 0) {
            _log.debug(u"Watchdog expired, id %d", {_watchDogId});
            _handler->handleWatchDogTimeout(*this);
        }

        // Compute new timeout.
        currentTimeout = _command == RESTART ? _timeout : Infinite;

        // Reset command.
        _command = NONE;
    }

    _log.debug(u"Watchdog thread completed, id %d", {_watchDogId});
}
