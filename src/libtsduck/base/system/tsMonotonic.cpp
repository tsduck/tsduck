//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMonotonic.h"
#include "tsFileUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <mmsystem.h>
    #include "tsAfterStandardHeaders.h"
#endif

// Required link libraries under Windows.
#if defined(TS_WINDOWS) && defined(TS_MSC)
    #pragma comment(lib, "winmm.lib") // timeBeginPeriod
#endif


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Monotonic::Monotonic(bool systemTime)
{
#if defined(TS_WINDOWS)
    if ((_handle = ::CreateWaitableTimer(nullptr, false, nullptr)) == nullptr) {
        throw MonotonicError(::GetLastError());
    }
#endif

    if (systemTime) {
        getSystemTime();
    }
}

ts::Monotonic::Monotonic(const Monotonic& t) : Monotonic(false)
{
    _value = t._value;
}

ts::Monotonic::~Monotonic()
{
#if defined(TS_WINDOWS)
    ::CloseHandle(_handle);
#endif
}


//----------------------------------------------------------------------------
// Get system time value
//----------------------------------------------------------------------------

void ts::Monotonic::getSystemTime()
{
#if defined(TS_WINDOWS)

    // On Win32, a the FILETIME structure is binary-compatible with a 64-bit integer.
    union {
        ::FILETIME ft;
        int64_t i;
    } result;
    ::GetSystemTimeAsFileTime(&result.ft);
    _value = result.i;

#elif defined(TS_MAC) || defined(TS_OPENBSD)

    // On OpenBSD and macOS, there is no clock_nanosleep. We use a relative nanosleep.
    // And nanosleep is always based on CLOCK_REALTIME.
    _value = Time::UnixClockNanoSeconds(CLOCK_REALTIME);

#elif defined(TS_UNIX)

    // Use clock_nanosleep. We can choose the clock.
    // The most appropriate one here is CLOCK_MONOTONIC.
    _value = Time::UnixClockNanoSeconds(CLOCK_MONOTONIC);

#else
    #error "Unimplemented operating system"
#endif
}


//----------------------------------------------------------------------------
// Wait until the time of the monotonic clock.
//----------------------------------------------------------------------------

void ts::Monotonic::wait()
{
#if defined(TS_WINDOWS)

    // Windows implementation

    ::LARGE_INTEGER due_time;
    due_time.QuadPart = _value;
    if (::SetWaitableTimer(_handle, &due_time, 0, nullptr, nullptr, false) == 0) {
        throw MonotonicError(::GetLastError());
    }
    if (::WaitForSingleObject(_handle, INFINITE) != WAIT_OBJECT_0) {
        throw MonotonicError(::GetLastError());
    }

#elif defined(TS_MAC) || defined(TS_OPENBSD)

    // OpenBSD and macOS implementation.
    // No support for clock_nanosleep. Use a relative nanosleep which is less precise.

    for (;;) {
        // Number of nanoseconds to wait for.
        const NanoSecond nano = _value - Time::UnixClockNanoSeconds(CLOCK_REALTIME);

        // Exit when due time is over.
        if (nano <= 0) {
            break;
        }

        // Wait that number of nanoseconds.
        ::timespec tspec;
        tspec.tv_sec = time_t(nano / NanoSecPerSec);
        tspec.tv_nsec = long(nano % NanoSecPerSec);
        if (::nanosleep(&tspec, nullptr) < 0 && errno != EINTR) {
            // Actual error, not interrupted by a signal
            throw MonotonicError(u"nanosleep error", errno);
        }
    }

#elif defined(TS_UNIX)

    // UNIX implementation with clock_nanosleep support.

    // Compute due time.
    ::timespec due;
    due.tv_sec = time_t(_value / NanoSecPerSec);
    due.tv_nsec = long(_value % NanoSecPerSec);

    // Loop on clock_nanosleep, ignoring signals
    int status;
    while ((status = ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &due, nullptr)) != 0) {
        if (status != EINTR) {
            // Actual error, not interrupted by a signal
            throw MonotonicError(u"clock_nanosleep error", errno);
        }
    }

#else
    #error "Unimplemented operating system"
#endif
}


//----------------------------------------------------------------------------
// This static method requests a minimum resolution, in nano-seconds, for the
// timers. Return the guaranteed value (can be equal to or greater than the
// requested value.
//----------------------------------------------------------------------------

ts::NanoSecond ts::Monotonic::SetPrecision(const NanoSecond& requested)
{
#if defined(TS_WINDOWS)

    // Windows implementation

    // Timer precisions use milliseconds on Windows. Convert requested value in ms.
    ::UINT good = std::max(::UINT(1), ::UINT(requested / 1000000));

    // Try requested value
    if (::timeBeginPeriod(good) == TIMERR_NOERROR) {
        return std::max(requested, 1000000 * NanoSecond(good));
    }

    // Requested value failed. Try doubling the value repeatedly.
    // If timer value excesses one second, there must be a problem.
    ::UINT fail = good;
    do {
        if (good >= 1000) { // 1000 ms = 1 s
            throw MonotonicError(u"cannot get system timer precision");
        }
        good = 2 * good;
    } while (::timeBeginPeriod(good) != TIMERR_NOERROR);

    // Now, repeatedly try to divide between 'fail' and 'good'. At most 10 tries.
    for (size_t count = 10; count > 0 && good > fail + 1; --count) {
        ::UINT val = fail + (good - fail) / 2;
        if (::timeBeginPeriod(val) == TIMERR_NOERROR) {
            ::timeEndPeriod(good);
            good = val;
        }
        else {
            fail = val;
        }
    }

    // Return last good value in nanoseconds
    return 1000000 * NanoSecond(good);

#elif defined(TS_UNIX)

    // POSIX implementation

    // The timer precision cannot be changed. Simply get the smallest delay.
    unsigned long jps = sysconf(_SC_CLK_TCK); // jiffies per second
    if (jps <= 0) {
        throw MonotonicError(u"system error: cannot get clock tick");
    }
    return std::max(requested, NanoSecond(NanoSecPerSec / jps));

#else
    #error "Unimplemented operating system"
#endif
}
