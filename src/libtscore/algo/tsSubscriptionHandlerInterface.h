//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for notification of subscription events.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class SubscriptionBase;

    //!
    //! Interface class for notification of subscription events.
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL SubscriptionHandlerInterface
    {
        TS_INTERFACE(SubscriptionHandlerInterface);
    private:
        // The class SubscriptionBase is allowed to call register/deregister.
        friend class SubscriptionBase;
        void registerSocket(SubscriptionBase*);
        void deregisterSocket(SubscriptionBase*);

        bool _destructing = false;
        std::set<SubscriptionBase*> _subscriptions {};
    };
}
