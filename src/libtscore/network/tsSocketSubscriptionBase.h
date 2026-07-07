//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for subscription to socket events.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocketHandlerInterface.h"

namespace ts {
    //!
    //! Base class for subscription to socket events.
    //! Used as base class in Socket and other socket wrappers which intercept socket events.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL SocketSubscriptionBase
    {
        TS_NOCOPY(SocketSubscriptionBase);
    public:
        //!
        //! Default constructor.
        //!
        SocketSubscriptionBase() = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~SocketSubscriptionBase();

        //!
        //! Add a subscriber to open/close events.
        //! @param [in] handler The object to call on open() and close().
        //!
        void addSubscription(SocketHandlerInterface* handler);

        //!
        //! Remove a subscriber to open/close events.
        //! @param [in] handler The object to no longer call on open() and close().
        //!
        void cancelSubscription(SocketHandlerInterface* handler);

    protected:
        //!
        //! Call a handler on all subscribers, using a lambda expression.
        //! @param [in] func Function to call as lambda expression.
        //!
        template <typename F>
        void callSubscribers(F&& func) {
            // We must iterate over a copy of the set because we call a handler at each iteration
            // and the handler may modify the socket state.
            for (auto subs : SocketHandlerSet(_subscribers)) {
                func(subs);
            }
        }

    private:
        using SocketHandlerSet = std::set<SocketHandlerInterface*>;
        SocketHandlerSet _subscribers {};
    };
}
