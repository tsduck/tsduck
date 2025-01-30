//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Template message queue for inter-thread communication with priority
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMessageQueue.h"

namespace ts {
    //!
    //! Template message queue for inter-thread communication with priority.
    //! @ingroup thread
    //!
    //! The ts::MessagePriorityQueue template class implements a synchronized
    //! access to a shared queue of generic messages. Each message has a priority.
    //! Messages with higher priority are deqeued first. Messages with equal
    //! priority are dequeued in their enqueueing order.
    //!
    //! @tparam MSG The type of the messages to exchange.
    //! @tparam COMPARE A function object to sort @a MSG instances. By default,
    //! the '<' operator on @a MSG is used.
    //!
    template <typename MSG, class COMPARE = std::less<MSG>>
    class MessagePriorityQueue: public MessageQueue<MSG>
    {
        TS_NOCOPY(MessagePriorityQueue);
    public:
        //!
        //! Constructor.
        //!
        //! @param [in] maxMessages Maximum number of messages in the queue.
        //! When a thread attempts to enqueue a message and the queue is full,
        //! the thread waits until at least one message is dequeued.
        //! If @a maxMessages is 0, the queue is unlimited. In that case,
        //! the logic of the application must ensure that the queue is
        //! bounded somehow, otherwise the queue may exhaust all the process
        //! memory.
        //!
        MessagePriorityQueue(size_t maxMessages = 0) : SuperClass(maxMessages) {}

    protected:
        //!
        //! Explicit reference to superclass.
        //!
        using SuperClass = MessageQueue<MSG>;

        //!
        //! This virtual protected method performs placement in the message queue.
        //! @param [in] msg The message to enqueue.
        //! @param [in] list The content of the queue.
        //! @return An iterator to the place where @a msg shall be placed.
        //!
        virtual typename SuperClass::MessageList::iterator
            enqueuePlacement(const typename SuperClass::MessagePtr& msg, typename SuperClass::MessageList& list) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Placement in the message queue (virtual protected methods).
template <typename MSG, class COMPARE>
typename ts::MessagePriorityQueue<MSG, COMPARE>::SuperClass::MessageList::iterator
ts::MessagePriorityQueue<MSG, COMPARE>::enqueuePlacement(const typename SuperClass::MessagePtr& msg, typename SuperClass::MessageList& list)
{
    auto loc = list.end();

    // Null pointers are stored at end (anywhere else would be probably fine).
    if (msg == nullptr) {
        return loc;
    }

    // Loop until the previous element is lower that msg.
    while (loc != list.begin()) {
        const auto cur = loc;
        --loc;
        if (*loc != nullptr && !COMPARE()(*msg, **loc)) {
            return cur;
        }
    }

    // Reached begin of list, all elements are greater than msg.
    return loc;
}
