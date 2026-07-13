//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for reactive message queue handlers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsMessageQueue.h"

namespace ts {

    template <typename MSG> class ReactiveMessageQueue;

    //!
    //! Interface class for reactive message queue handlers.
    //! @ingroup libtscore reactor
    //! @tparam MSG The type of the messages to exchange.
    //!
    template <typename MSG>
    class ReactiveMessageQueueHandlerInterface
    {
        TS_INTERFACE(ReactiveMessageQueueHandlerInterface);
    public:
        //!
        //! Handle a dequeued message from a message queue.
        //! @param [in,out] queue Message queue for which the handler is invoked.
        //! @param [in,out] msg A shared pointer to the dequeued message.
        //! The pointer is null if the enqueued message was a null pointer.
        //! @param [in] user_data Application-specific shared pointer which was passed by the application.
        //!
        virtual void handleDequeuedMessage(ReactiveMessageQueue<MSG>& queue, MessageQueue<MSG>::MessagePtr& msg, const ObjectPtr& user_data) = 0;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN) // doxygen complains about undefined desctructor, bug ?
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename MSG>
ts::ReactiveMessageQueueHandlerInterface<MSG>::~ReactiveMessageQueueHandlerInterface()
{
}
TS_POP_WARNING()
#endif
