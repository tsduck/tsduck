//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Allocate POSIX real-time signal numbers (Linux-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"

#include "tsBeforeStandardHeaders.h"
#include <signal.h>
#include "tsAfterStandardHeaders.h"

namespace ts {
    //!
    //! Allocate POSIX real-time signal numbers (Linux-specific).
    //! @ingroup unix
    //!
    class TSDUCKDLL SignalAllocator
    {
        // This class is a singleton. Use static Instance() method.
        TS_DECLARE_SINGLETON(SignalAllocator);

    public:
        //!
        //! Allocate a new signal number.
        //! @return A POSIX real-time signal number or -1 if none available.
        //!
        int allocate();

        //!
        //! Release a signal number.
        //! @param [in] sig A POSIX real-time signal number.
        //!
        void release(int sig);

    private:
        // Private members:
        const int         _signal_min = SIGRTMIN;
        const int         _signal_max = SIGRTMAX;
        std::mutex        _mutex {};
        std::vector<bool> _signals {};
    };
}
