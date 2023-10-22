//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Expiration handler interface for general-purpose timeout watchdog.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class WatchDog;

    //!
    //! Expiration handler interface for general-purpose timeout watchdog.
    //! @ingroup thread
    //!
    class TSDUCKDLL WatchDogHandlerInterface
    {
        TS_INTERFACE(WatchDogHandlerInterface);
    public:
        //!
        //! Handle the expiration of a timeout.
        //! The handler is executed in the context of an internal thread of the watchdog.
        //! @param [in,out] watchdog The watchdog which triggered the timeout.
        //!
        virtual void handleWatchDogTimeout(WatchDog& watchdog) = 0;
    };
}
