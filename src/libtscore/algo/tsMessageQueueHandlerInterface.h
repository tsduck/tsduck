//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface class for notification of enqueue/dequeue on message queues.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSubscriptionHandlerInterface.h"

namespace ts {
    //!
    //! Interface class for notification of enqueue/dequeue on message queues.
    //! All methods are empty by default. An application may implement the required ones only.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL MessageQueueHandlerInterface: public SubscriptionHandlerInterface
    {
        TS_SUBINTERFACE(MessageQueueHandlerInterface);
    public:
        //!
        //! Called when a message was enqueued.
        //! This handler is called in the context of the thread which enqueued the message.
        //! @param [in] queue_size Number of messages in the queue after enqueuing the message.
        //! This value is informational only because messages may have been enqueued or dequeued
        //! just before this handler is called.
        //!
        virtual void handleMessageEnqueued(size_t queue_size);

        //!
        //! Called when a message was dequeued.
        //! This handler is called in the context of the thread which dequeued the message.
        //! @param [in] queue_size Number of messages in the queue after dequeuing the message.
        //! This value is informational only because messages may have been enqueued or dequeued
        //! just before this handler is called.
        //!
        virtual void handleMessageDequeued(size_t queue_size);
    };
}
