//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(MutexInterface);
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
    };
}
