//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsThreadAttributes.h"


//----------------------------------------------------------------------------
// Default operating system priorities
//----------------------------------------------------------------------------

volatile bool ts::ThreadAttributes::_priority_initialized = false;
int ts::ThreadAttributes::_minimum_priority = 0;
int ts::ThreadAttributes::_low_priority = 0;
int ts::ThreadAttributes::_normal_priority = 0;
int ts::ThreadAttributes::_high_priority = 0;
int ts::ThreadAttributes::_maximum_priority = 0;


//----------------------------------------------------------------------------
// This static method initializes the operating system priority range.
//----------------------------------------------------------------------------

void ts::ThreadAttributes::InitializePriorities()
{
#if defined(TS_WINDOWS)

    // Windows priority mapping: see Win32Priority() below
    _minimum_priority = 0;  // THREAD_PRIORITY_IDLE
    _low_priority = 2;      // THREAD_PRIORITY_BELOW_NORMAL
    _normal_priority = 3;   // THREAD_PRIORITY_NORMAL
    _high_priority = 4;     // THREAD_PRIORITY_ABOVE_NORMAL;
    _maximum_priority = 6;  // THREAD_PRIORITY_TIME_CRITICAL

#else

    // POSIX pthread implementation.
    // First, get the scheduling policy for the current process.
    const int schedPolicy = PthreadSchedulingPolicy();
    // On error, simply let one single priority: zero
    if (schedPolicy >= 0) {
        // Get the system min and max priorities for this scheduling policy
        const int prioMin = ::sched_get_priority_min(schedPolicy);
        const int prioMax = ::sched_get_priority_max(schedPolicy);
        // On error, keep default values
        _minimum_priority = prioMin >= 0 ? prioMin : 0;
        _maximum_priority = prioMax >= 0 ? std::max(_minimum_priority, prioMax) : _minimum_priority;
        _normal_priority = (_minimum_priority + _maximum_priority) / 2;
        _low_priority = (_minimum_priority + _normal_priority) / 2;
        _high_priority = (_normal_priority + _maximum_priority) / 2;
    }

#endif
    _priority_initialized = true;
}


//----------------------------------------------------------------------------
// Get one of the predefined priorities.
//----------------------------------------------------------------------------

int ts::ThreadAttributes::GetPriority(const int& staticPriority)
{
    if (!_priority_initialized) {
        InitializePriorities();
    }
    return staticPriority;
}


//----------------------------------------------------------------------------
// This static method is used by the implementation of ts::Thread on Unix
// to obtain the scheduling policy to use for this process.
//----------------------------------------------------------------------------

#if defined(TS_UNIX)
int ts::ThreadAttributes::PthreadSchedulingPolicy()
{
    // Get the scheduling policy of the current process.
#if defined(TS_MAC) || defined(TS_OPENBSD)
    // On macOS and OpenBSD, there is no sched_getscheduler, use hard-coded SCHED_OTHER.
    // This is far from ideal, can we do better?
    return SCHED_OTHER;
#else
    // Get the scheduling policy of the current thread. Return SCHED_OTHER on error.
    const int pol = ::sched_getscheduler(0);
    return pol >= 0 ? pol : SCHED_OTHER;
#endif
}
#endif


//----------------------------------------------------------------------------
// This static method is used by the implementation of ts::Thread on Windows
// to obtain the actual Win32 priority value.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
int ts::ThreadAttributes::Win32Priority(int priority)
{
    switch (priority) {
        case 1:
            return THREAD_PRIORITY_LOWEST;
        case 2:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case 3:
            return THREAD_PRIORITY_NORMAL;
        case 4:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        case 5:
            return THREAD_PRIORITY_HIGHEST;
        default:
            if (priority <= 0) {
                return THREAD_PRIORITY_IDLE;
            }
            else {
                return THREAD_PRIORITY_TIME_CRITICAL;
            }
    }
}
#endif


//----------------------------------------------------------------------------
// Default constructor (all attributes have their default values)
//----------------------------------------------------------------------------

ts::ThreadAttributes::ThreadAttributes()
{
    if (!_priority_initialized) {
        InitializePriorities();
    }
    _priority = _normal_priority;
}


//----------------------------------------------------------------------------
// Set the priority for the thread.
//----------------------------------------------------------------------------

ts::ThreadAttributes& ts::ThreadAttributes::setPriority(int priority)
{
    // Force within allowed range. Note that the static values where already
    // initialized, no later than the constructor.
    _priority = std::max(_minimum_priority, std::min(_maximum_priority, priority));
    return *this;
}
