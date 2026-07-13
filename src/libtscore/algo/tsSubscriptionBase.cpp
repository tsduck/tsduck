//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSubscriptionBase.h"


//----------------------------------------------------------------------------
// Constructors & destructor
//----------------------------------------------------------------------------

ts::SubscriptionBase::~SubscriptionBase()
{
    // Tell all registered subscribers to deregister from this object before destroying it.
    callSubscribers<SubscriptionHandlerInterface>([this](SubscriptionHandlerInterface* subs) {
        subs->deregisterSocket(this);
    });
    _subscribers.clear();
}


//----------------------------------------------------------------------------
// Add/remove a subscriber to open/close events.
//----------------------------------------------------------------------------

void ts::SubscriptionBase::addSubscription(SubscriptionHandlerInterface* handler)
{
    if (handler != nullptr) {
        // Make sure we can call the handler.
        _subscribers.insert(handler);

        // Make sure the handler can cancel its subscription when destructed.
        handler->registerSocket(this);
    }
}

void ts::SubscriptionBase::cancelSubscription(SubscriptionHandlerInterface* handler)
{
    if (handler != nullptr) {
        // No longer call the handler.
        _subscribers.erase(handler);

        // Make sure the handler no longer cancel its subscription when destructed.
        handler->deregisterSocket(this);
    }
}
