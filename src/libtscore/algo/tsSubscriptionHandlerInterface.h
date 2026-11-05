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
    //! All classes which subscribe to events in a subclass of SubscriptionBase must
    //! implement SubscriptionHandlerInterface. This interface does not declare any
    //! common method. Each subclass of SubscriptionBase decides which specific
    //! method should be called.
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL SubscriptionHandlerInterface
    {
        TS_NOCOPY_INTERFACE(SubscriptionHandlerInterface);
    private:
        // The base of the interface class keeps a set of all objects it is a subscriber of. The idea is
        // that this object can automatically deregister itself from all subscriptions in the destructor.
        bool                        _destructing = false;
        std::mutex                  _mutex {};
        std::set<SubscriptionBase*> _subscriptions {};

        // The class SubscriptionBase is allowed to call register/deregister.
        friend class SubscriptionBase;
        void registerSubscription(SubscriptionBase*);
        void deregisterSubscription(SubscriptionBase*);
    };
}
