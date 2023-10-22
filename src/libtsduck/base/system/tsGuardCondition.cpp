//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGuardCondition.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::GuardCondition::GuardCondition(Mutex& mutex, Condition& condition, MilliSecond timeout) :
    _mutex(mutex),
    _condition(condition)
{
    _is_locked = mutex.acquire(timeout);

    if (timeout == Infinite && !_is_locked) {
        throw GuardConditionError(u"failed to acquire mutex");
    }
}

//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::GuardCondition::~GuardCondition()
{
    if (_is_locked) {
        _mutex.release();
        _is_locked = false;
    }
}

//----------------------------------------------------------------------------
// Signal the condition.
// The mutex must have been locked.
//----------------------------------------------------------------------------

void ts::GuardCondition::signal()
{
    if (!_is_locked) {
        throw GuardConditionError(u"GuardCondition: signal condition while mutex not locked");
    }
    else {
        _condition.signal();
    }
}

//----------------------------------------------------------------------------
// Wait for the condition or timeout.
// The mutex must have been locked.
//----------------------------------------------------------------------------

bool ts::GuardCondition::waitCondition(MilliSecond timeout)
{
    if (!_is_locked) {
        throw GuardConditionError(u"GuardCondition: wait condition while mutex not locked");
    }
    else {
        return _condition.wait(_mutex, timeout);
    }
}
