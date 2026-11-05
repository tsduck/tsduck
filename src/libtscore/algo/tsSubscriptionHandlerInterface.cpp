//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSubscriptionHandlerInterface.h"
#include "tsSubscriptionBase.h"


//----------------------------------------------------------------------------
// Register / deregister the subscription to an object.
//----------------------------------------------------------------------------

void ts::SubscriptionHandlerInterface::registerSubscription(SubscriptionBase* obj)
{
    if (!_destructing && obj != nullptr) {
        std::lock_guard<std::mutex> lock(*_mutex);
        _subscriptions.insert(obj);
    }
}

void ts::SubscriptionHandlerInterface::deregisterSubscription(SubscriptionBase* obj)
{
    // Avoid modifying _subscriptions during destructor.
    if (!_destructing) {
        std::lock_guard<std::mutex> lock(*_mutex);
        _subscriptions.erase(obj);
    }
}


//----------------------------------------------------------------------------
// The destructor removes all subscriptions.
//----------------------------------------------------------------------------

ts::SubscriptionHandlerInterface::~SubscriptionHandlerInterface()
{
    // Tell all subscribers to forget us.
    _destructing = true;
    for (auto obj : _subscriptions) {
        obj->cancelSubscription(this);
    }
    _subscriptions.clear();
}
