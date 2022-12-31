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

#include "tsGuardMutex.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::GuardMutex::GuardMutex(MutexInterface& mutex, MilliSecond timeout) :
    _mutex(mutex),
    _is_locked(false)
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
            static const size_t err_size = sizeof(err) - 1;
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
