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
//!
//!  @file
//!  Automatic guard class for mutex (ts::MutexInterface).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMutexInterface.h"
#include "tsException.h"

namespace ts {
    //!
    //! Automatic guard class for mutex (ts::MutexInterface).
    //! @ingroup thread
    //!
    //! This class implements the @e guard design pattern for mutex,
    //! as defined by ts::MutexInterface.
    //!
    //! The common pitfall in the usage of resources which must be explicitly
    //! released after having been acquired is the absence of release.
    //! This can be an omission in the code (no invocation of @c release() at all)
    //! or a premature exit from the code sequence between the acquire and
    //! release operations (exception or @c return statement).
    //!
    //! Example:
    //! @code
    //! Mutex mutex;
    //! ...
    //! mutex.acquire();
    //! ...
    //! // some exception occurs here and release() is never invoked.
    //! ...
    //! mutex.release();
    //! @endcode
    //!
    //! The @e guard design pattern mitigates this risk. Each critical
    //! sequence is a code block. Within the code block, an ancillary
    //! object, the @e guard, is created and refers to the common mutex
    //! object. The constructor of the guard automatically acquires the
    //! mutex and its destructor automatically releases the mutex.
    //!
    //! There is no need for an explicit release of the mutex.
    //! If there is an exception or a premature exit of the code block,
    //! the C++ language guarantees that the destructor of the guard
    //! will be invoked and that the mutex will be released in all cases.
    //!
    //! Example:
    //! @code
    //! Mutex mutex;
    //! ...
    //! {
    //!     MutexGuard guard (mutex); // mutex acquired
    //!     ...
    //!     // some exception occurs here, no problem, don't worry
    //!     ...
    //! } // guard's destructor invoked, mutex always released
    //! @endcode
    //!
    class TSDUCKDLL GuardMutex
    {
        TS_NOBUILD_NOCOPY(GuardMutex);
    public:
        //!
        //! Fatal low-level mutex guard error.
        //!
        TS_DECLARE_EXCEPTION(GuardMutexError);

        //!
        //! Constructor, automatically acquire the mutex with a timeout.
        //!
        //! With a non-infinite timeout, there is no guarantee that the mutex is
        //! locked after construction. The user has to invoke isLocked() to check
        //! that the mutex was actually acquired before the timeout expired.
        //!
        //! @param [in,out] mutex A reference on the mutex object to acquire.
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @exception ts::GuardMutex::GuardMutexError Thrown whenever an error occurs
        //! during the acquisition of the mutex. Exceeding the timeout is not
        //! error, the object is successfully constructed but isLocked() will
        //! return false.
        //!
        GuardMutex(MutexInterface& mutex, MilliSecond timeout = Infinite);

        //!
        //! Destructor, automatically release the mutex.
        //!
        //! @exception ts::GuardMutex::GuardMutexError Thrown whenever an error occurs
        //! during the release of the mutex, i.e. when
        //! ts::MutexInterface::release() returns false.
        //!
        virtual ~GuardMutex();

        //!
        //! Check if the mutex was actually locked.
        //!
        //! This method is useful only with the object was constructed with a
        //! timeout. When the constructor without timeout was used, this method
        //! always return true.
        //!
        //! @return True if the mutex was successfully acquired and false if the timeout expired.
        //!
        bool isLocked() const {return _is_locked;}

        //!
        //! Force an early unlock of the mutex.
        //! @return True if the mutex has been successfully unlocked.
        //!
        bool unlock();

    private:
        MutexInterface& _mutex;
        bool _is_locked;
    };
}
