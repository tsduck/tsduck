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
//!  Declare the ts::Condition class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsException.h"
#include "tsMutex.h"

namespace ts {

    //!
    //! Implementation of the @e synchronization @e condition design pattern.
    //! @ingroup thread
    //!
    //! A @e condition is a general synchronization mechanism which
    //! is associated with a @e mutex.
    //!
    //! Typical usage:
    //! A set of shared data is protected using a mutex (ts::Mutex).
    //! When some expected modification is performed, the modifier thread
    //! @e signals the condition. When other threads wait for the modification
    //! to be performed, they acquire the mutex and @e wait for the condition.
    //!
    //! The implementation of this class is operating system dependent,
    //! just like ts::Thread and ts::Mutex.
    //!
    class TSDUCKDLL Condition
    {
        TS_NOCOPY(Condition);
    public:
        //!
        //! Fatal low-level condition/threading error.
        //!
        TS_DECLARE_EXCEPTION(ConditionError);

        //!
        //! Default constructor.
        //!
        //! @throw ts::Condition::ConditionError In case of operating system error,
        //! when the underlying system objects could not be created.
        //!
        Condition();

        //!
        //! Destructor.
        //!
        virtual ~Condition();

        //!
        //! Signal the condition.
        //!
        //! If more than one thread wait for the condition, at least one
        //! is awaken. It is then the responsibility of the awaken threads
        //! to check that the expected situation actually exists.
        //!
        //! @throw ts::Condition::ConditionError In case of operating system error.
        //!
        void signal();

        //!
        //! Wait for the condition to be signaled with a timeout,
        //!
        //! The calling thread must have acquired the mutex first.
        //! The mutex is automatically released while waiting and then automatically
        //! re-acquired before returning with a successful status.
        //!
        //! @param [in,out] mutex The mutex to automatically release/acquire.
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @param [out] signaled Set to true if the condition was successfully
        //! signaled and the timeout did not expired.
        //! @return True on success and false on error. When the timeout expires,
        //! this is not an error: true is returned and the mutex is automatically
        //! re-acquired. When returning false, the state of the mutex is undefined
        //! (acquired vs. released).
        //!
        bool wait(Mutex& mutex, MilliSecond timeout, bool& signaled);

        //!
        //! Wait for the condition to be signaled with a timeout and loose error reporting.
        //!
        //! The calling thread must have acquired the mutex first.
        //! The mutex is automatically released while waiting and then automatically
        //! re-acquired before returning with a successful status.
        //!
        //! @param [in,out] mutex The mutex to automatically release/acquire.
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @return True on success and false on error or timeout. When false is
        //! returned, it is not possible to determine whether this was an error
        //! or a timeout and the state of the mutex is undefined
        //! (acquired vs. released).
        //!
        bool wait(Mutex& mutex, MilliSecond timeout)
        {
            bool signaled;
            return wait(mutex, timeout, signaled) && signaled;
        }

    private:
        bool _created;
#if defined(TS_WINDOWS)
        ::HANDLE _handle; // Event handle
#else
        ::pthread_cond_t _cond;
#endif
    };
}
