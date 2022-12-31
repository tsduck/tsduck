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
//!  Interface class for mutex objects.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Interface class for mutex objects.
    //! @ingroup thread
    //!
    //! A mutex is a general mutual exclusion mechanism.
    //! This interface class defines the generic interface of a mutex.
    //! Various concrete mutex classes can be defined based on distinct
    //! lower-level synchronization mechanisms.
    //!
    //! For each successful call to acquire(), there must be one
    //! call to release().
    //!
    //! This interface class does not define whether the actual
    //! mutex should be @e recursive or not. A mutex is defined
    //! as recursive if nested pairs of acquire() / release() are
    //! allowed on the mutex. If the implementation is a recursive
    //! mutex, there must be exactly one release() for each nested
    //! invocation of acquire().
    //!
    //! The concrete class ts::NullMutex is an empty mutex implementation
    //! which does nothing and can be used wherever a MutexInterface is
    //! required but no actual synchronization is necessary (non-threaded
    //! applications for instances).
    //!
    class TSDUCKDLL MutexInterface
    {
    public:

        //!
        //! Acquire the mutex with a timeout.
        //!
        //! The actual semantic of this operation depends on the mutex
        //! concrete class. Typically, the concrete class attempts to
        //! acquire a low-level synchronization mechanism and blocks
        //! until the mutex is granted or a timeout occurs.
        //!
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @return True on success and false on error or timeout. When false is
        //! returned, it is not possible to determine whether this was an error
        //! or a timeout.
        //!
        virtual bool acquire(MilliSecond timeout = Infinite) = 0;

        //!
        //! Release the mutex.
        //!
        //! @return true on success and false on error.
        //!
        virtual bool release() = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~MutexInterface();
    };
}
