//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Empty mutex implementation.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Empty mutex implementation.
    //! @ingroup thread
    //!
    //! The class ts::NullMutex is an empty mutex implementation which does nothing
    //! and can be used wherever no actual synchronization is necessary (non-threaded
    //! applications for instances).
    //!
    class TSDUCKDLL NullMutex
    {
    public:
        //!
        //! Acquire the mutex, does nothing but successfully!
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex. Ignored.
        //! @return Always true.
        //!
        bool acquire(MilliSecond timeout = Infinite) { return true; }

        //!
        //! Release the mutex, does nothing but successfully!
        //! @return Always true.
        //!
        bool release() { return true; }
    };
}
