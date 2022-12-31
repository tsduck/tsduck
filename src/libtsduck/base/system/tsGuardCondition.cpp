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

#include "tsGuardCondition.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::GuardCondition::GuardCondition(Mutex& mutex, Condition& condition, MilliSecond timeout) :
    _mutex(mutex),
    _condition(condition),
    _is_locked(false)
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
