//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface for handling user Ctrl-C interrupts.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! An interface to be implemented by a class to handle user Ctrl-C interrupt.
    //! @ingroup system
    //!
    class TSDUCKDLL InterruptHandler
    {
        TS_INTERFACE(InterruptHandler);
    public:
        //!
        //! This hook is invoked during the requested interrupt, in the context of a dedicated thread.
        //!
        virtual void handleInterrupt() = 0;
    };
}
