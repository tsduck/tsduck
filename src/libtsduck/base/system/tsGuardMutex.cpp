//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGuardMutex.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::GuardMutex::GuardMutex(MutexInterface& mutex, MilliSecond timeout) :
    _mutex(mutex)
{
    _is_locked = mutex.acquire(timeout);

    if (timeout == Infinite && !_is_locked) {
        throw GuardMutexError(u"failed to acquire mutex");
    }
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::GuardMutex::~GuardMutex()
{
    if (_is_locked) {
        _is_locked = !_mutex.release();
        if (_is_locked) {
            // With C++11, destructors are no longer allowed to throw an exception.
            static const char err[] = "\n\n*** Fatal error: GuardMutex failed to release mutex in destructor, aborting...\n\n";
            static constexpr size_t err_size = sizeof(err) - 1;
            FatalError(err, err_size);
        }
    }
}

//----------------------------------------------------------------------------
// Force an early unlock of the mutex.
//----------------------------------------------------------------------------

bool ts::GuardMutex::unlock()
{
    if (_is_locked) {
        _is_locked = !_mutex.release();
        return !_is_locked;
    }
    else {
        // The mutex was not locked in the first place.
        return false;
    }
}
