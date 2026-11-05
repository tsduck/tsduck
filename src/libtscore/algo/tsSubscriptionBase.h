//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for subscription to events.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSubscriptionHandlerInterface.h"

namespace ts {
    //!
    //! Base class for subscription to events.
    //! Used as base class in Socket and other socket wrappers which intercept socket events.
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL SubscriptionBase
    {
        TS_NOCOPY(SubscriptionBase);
    public:
        //!
        //! Default constructor.
        //!
        SubscriptionBase() = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~SubscriptionBase();

        //!
        //! Add a subscriber to open/close events.
        //! @param [in] handler The object to call on open() and close().
        //!
        void addSubscription(SubscriptionHandlerInterface* handler);

        //!
        //! Remove a subscriber to open/close events.
        //! @param [in] handler The object to no longer call on open() and close().
        //!
        void cancelSubscription(SubscriptionHandlerInterface* handler);

    protected:
        //!
        //! Call a handler on all subscribers, using a lambda expression.
        //! @param [in] func Function to call as lambda expression.
        //!
        template <typename HANDLER, typename F>
            requires std::derived_from<HANDLER, SubscriptionHandlerInterface>
        void callSubscribers(F&& func) {
            // We must iterate over a copy of the set because we call a handler at each iteration and the handler
            // may modify the list of subscribers. Furthermore, we need to recheck that a handler is still present
            // after calling previous handlers.
            HandlerSet copy;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                copy = _subscribers;
            }
            for (auto subs : copy) {
                HANDLER* h = dynamic_cast<HANDLER*>(subs);
                if (h != nullptr) {
                    bool still_there = false;
                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        still_there = _subscribers.contains(subs);
                    }
                    if (still_there) {
                        func(h);
                    }
                }
            }
        }

    private:
        using HandlerSet = std::set<SubscriptionHandlerInterface*>;
        std::mutex _mutex {};
        HandlerSet _subscribers {};
    };
}
