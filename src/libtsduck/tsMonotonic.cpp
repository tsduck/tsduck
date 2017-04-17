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
//
//  Basic monotonic clock & timer class
//
//----------------------------------------------------------------------------

#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"



//----------------------------------------------------------------------------
// System-specific initialization
//----------------------------------------------------------------------------

void ts::Monotonic::init() throw (MonotonicError)
{
#if defined (__windows)

    // Windows implementation

    if ((_handle = ::CreateWaitableTimer (NULL, FALSE, NULL)) == NULL) {
        throw MonotonicError (::GetLastError ());
    }

#else

    // POSIX implementation

    _jps = sysconf (_SC_CLK_TCK); // jiffies per second
    if (_jps <= 0) {
        throw MonotonicError ("system error: cannot get clock tick");
    }

#endif
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Monotonic::~Monotonic()
{
#if defined (__windows)
    ::CloseHandle (_handle);
#endif
}


//----------------------------------------------------------------------------
// Get system time value
//----------------------------------------------------------------------------

void ts::Monotonic::getSystemTime() throw (MonotonicError)
{
#if defined (__windows)

    // Windows implementation

    // On Win32, a the FILETIME structure is binary-compatible with a 64-bit integer.
    union {
        ::FILETIME ft;
        int64_t i;
    } result;
    ::GetSystemTimeAsFileTime(&result.ft);
    _value = result.i;

#else

    // POSIX implementation

    _value = Time::UnixRealTimeClockNanoSeconds();

#endif
}


//----------------------------------------------------------------------------
// Wait until the time of the monotonic clock.
//----------------------------------------------------------------------------

void ts::Monotonic::wait() throw(MonotonicError)
{
#if defined(__windows)

    // Windows implementation

    ::LARGE_INTEGER due_time;
    due_time.QuadPart = _value;
    if (::SetWaitableTimer(_handle, &due_time, 0, NULL, NULL, FALSE) == 0) {
        throw MonotonicError(::GetLastError());
    }
    if (::WaitForSingleObject(_handle, INFINITE) != WAIT_OBJECT_0) {
        throw MonotonicError(::GetLastError());
    }

#elif defined(__mac)

    // MacOS implementation.
    // There is no clock_nanosleep on MacOS. We need to use a relative nanosleep which will be less precise.

    for (;;) {
        // Number of nanoseconds to wait for.
        const NanoSecond nano = _value - Time::UnixRealTimeClockNanoSeconds();

        // Exit when due time is over.
        if (nano <= 0) {
            break;
        }

        // Wait that number of nanoseconds.
        ::timespec tspec;
        tspec.tv_sec = time_t(nano / NanoSecPerSec);
        tspec.tv_nsec = long(nano % NanoSecPerSec);
        if (::nanosleep(&tspec, NULL) < 0 && errno != EINTR) {
            // Actual error, not interrupted by a signal
            throw MonotonicError("nanosleep error", errno);
        }
    }
    
#else

    // POSIX implementation

    // Compute due time
    ::timespec due;
    due.tv_sec = time_t(_value / NanoSecPerSec);
    due.tv_nsec = long(_value % NanoSecPerSec);

    // Loop on clock_nanosleep, ignoring signals
    int status;
    while ((status = ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &due, NULL)) != 0) {
        if (status != EINTR) {
            // Actual error, not interrupted by a signal
            throw MonotonicError("clock_nanosleep error", errno);
        }
    }

#endif
}


//----------------------------------------------------------------------------
// This static method requests a minimum resolution, in nano-seconds, for the
// timers. Return the guaranteed value (can be equal to or greater than the
// requested value.
//----------------------------------------------------------------------------

ts::NanoSecond ts::Monotonic::SetPrecision(const NanoSecond& requested) throw (MonotonicError)
{
#if defined (__windows)

    // Windows implementation

    // Timer precisions use milliseconds on Windows. Convert requested value in ms.
    ::UINT good = std::max (::UINT (1), ::UINT (requested / 1000000));

    // Try requested value
    if (::timeBeginPeriod (good) == TIMERR_NOERROR) {
        return std::max (requested, 1000000 * NanoSecond (good));
    }

    // Requested value failed. Try doubling the value repeatedly.
    // If timer value excesses one second, there must be a problem.
    ::UINT fail = good;
    do {
        if (good >= 1000) { // 1000 ms = 1 s
            throw MonotonicError ("cannot get system timer precision");
        }
        good = 2 * good;
    } while (::timeBeginPeriod (good) != TIMERR_NOERROR);

    // Now, repeatedly try to divide between 'fail' and 'good'. At most 10 tries.
    for (size_t count = 10; count > 0 && good > fail + 1; --count) {
        ::UINT val = fail + (good - fail) / 2;
        if (::timeBeginPeriod (val) == TIMERR_NOERROR) {
            ::timeEndPeriod (good);
            good = val;
        }
        else {
            fail = val;
        }
    }

    // Return last good value in nanoseconds
    return 1000000 * NanoSecond (good);

#else

    // POSIX implementation

    // The timer precision cannot be changed. Simply get the smallest delay.
    Monotonic m; // constructor initializes _jps
    return std::max (requested, NanoSecond (NanoSecPerSec / m._jps));

#endif
}
