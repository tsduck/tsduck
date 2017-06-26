//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "tsCondition.h"
#include "tsTime.h"
#include "tsFormat.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Condition::Condition() :
    _created(false),
#if defined(__windows)
    _handle(INVALID_HANDLE_VALUE)
#else
    _cond(PTHREAD_COND_INITIALIZER)
#endif
{
#if defined(__windows)

    // Windows implementation.
    if ((_handle = ::CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        throw ConditionError(::GetLastError());
        return;
    }

#else

    // POSIX pthread implementation
    int error;
    ::pthread_condattr_t attr;

    if ((error = ::pthread_condattr_init(&attr)) != 0) {
        throw ConditionError("cond attr init", error);
        return;
    }
#if !defined(__mac)
    // The clock attribute is not implemented in MacOS, just keep the default clock.
    else if ((error = ::pthread_condattr_setclock(&attr, CLOCK_REALTIME)) != 0) {
        throw ConditionError("cond attr set clock", error);
        return;
    }
#endif
    else if ((error = ::pthread_cond_init(&_cond, &attr)) != 0) {
        throw ConditionError("cond init", error);
        return;
    }
    else if ((error = ::pthread_condattr_destroy(&attr)) != 0) {
        throw ConditionError("cond attr destroy", error);
        return;
    }

#endif

    _created = true;
}


//----------------------------------------------------------------------------
// Constructor/destructor
//----------------------------------------------------------------------------

ts::Condition::~Condition()
{
    if (_created) {
#if defined(__windows)
        ::CloseHandle(_handle);
#else
        ::pthread_cond_destroy(&_cond);
#endif
        _created = false;
    }
}


//----------------------------------------------------------------------------
// Signal the condition.
//----------------------------------------------------------------------------

void ts::Condition::signal()
{
    if (!_created) {
        return;
    }

#if defined (__windows)
    if (::SetEvent(_handle) == 0) {
        throw ConditionError (::GetLastError ());
    }
#else
    int error;
    if ((error = ::pthread_cond_signal(&_cond)) != 0) {
        throw ConditionError("cond signal", error);
    }
#endif
}


//----------------------------------------------------------------------------
// Wait for the condition to be signaled (or timeout expires).
//----------------------------------------------------------------------------

bool ts::Condition::wait (Mutex& mutex, MilliSecond timeout, bool& signaled)
{
    signaled = false;

    if (!_created) {
        return false;
    }

#if defined(__windows)

    if (!mutex.release ()) {
        return false;
    }

    // Wait for the event
    bool success = false;
    switch (::WaitForSingleObject(_handle, ::DWORD(std::min(timeout, MilliSecond(INFINITE))))) {
        case WAIT_OBJECT_0:
            // Success
            signaled = true;
            success = true;
            break;
        case WAIT_TIMEOUT:
            // Successful call but not signaled
            success = true;
            break;
        default:
            // Error
            throw ConditionError(::GetLastError());
    }

    // Re-acquire the mutex
    return mutex.acquire () && success;

#else

    // Convert infinite timeout into non-timed call.
    if (timeout == Infinite) {
        return ::pthread_cond_wait(&_cond, &mutex._mutex) == 0;
    }

    // Get current time + timeout using the real-time clock.
    ::timespec time;
    Time::UnixRealTimeClock(time, timeout);

    // Timed wait:
    int error;
    if ((error = ::pthread_cond_timedwait(&_cond, &mutex._mutex, &time)) == 0) {
        // Condition successfully signaled
        signaled = true;
        return true;
    }
    else if (error == ETIMEDOUT) {
        // Successful call but not acquired.
        return true;
    }
    else {
        // Error
        throw ConditionError("cond timed wait", error);
    }

#endif
}
