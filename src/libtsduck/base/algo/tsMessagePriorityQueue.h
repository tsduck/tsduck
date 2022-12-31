//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
    //! @tparam MUTEX The type of mutex for synchronization (ts::Mutex by default).
    //! @tparam COMPARE A function object to sort @a MSG instances. By default,
    //! the '<' operator on @a MSG is used.
    //!
    template <typename MSG, class MUTEX = Mutex, class COMPARE = std::less<MSG>>
    class MessagePriorityQueue: public MessageQueue<MSG, MUTEX>
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
        MessagePriorityQueue(size_t maxMessages = 0);

    protected:
        //!
        //! Explicit reference to superclass.
        //!
        typedef MessageQueue<MSG, MUTEX> SuperClass;

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

#include "tsMessagePriorityQueueTemplate.h"
