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
//!  Template message queue for inter-thread communication
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsSafePtr.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuardCondition.h"

namespace ts {
    //!
    //! Template message queue for inter-thread communication.
    //! @ingroup thread
    //!
    //! The ts::MessageQueue template class implements a synchronized
    //! access to a shared queue of generic messages.
    //!
    //! @tparam MSG The type of the messages to exchange.
    //! @tparam MUTEX The type of mutex for synchronization of message pointers (ts::Mutex by default).
    //!
    template <typename MSG, class MUTEX = Mutex>
    class MessageQueue
    {
        TS_NOCOPY(MessageQueue);
    public:
        //!
        //! Safe pointer to messages.
        //!
        //! Since data are copied from the producer thread into the queue and later copied
        //! again from the queue into the consumer thread, the copied data is always a
        //! safe-pointer to the actual message content.
        //!
        typedef SafePtr<MSG, MUTEX> MessagePtr;

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
        MessageQueue(size_t maxMessages = 0);

        //!
        //! Destructor
        //!
        virtual ~MessageQueue();

        //!
        //! Get the maximum allowed messages in the queue.
        //!
        //! @return The maximum allowed messages in the queue (0 means unlimited).
        //!
        size_t getMaxMessages() const;

        //!
        //! Change the maximum allowed messages in the queue.
        //!
        //! @param [in] maxMessages Maximum number of messages in the queue.
        //! When a thread attempts to enqueue a message and the queue is full,
        //! the thread waits until at least one message is dequeued.
        //! If @a maxMessages is 0, the queue is unlimited. In that case,
        //! the logic of the application must ensure that the queue is
        //! bounded somehow, otherwise the queue may exhaust all the process
        //! memory.
        //!
        void setMaxMessages(size_t maxMessages);

        //!
        //! Insert a message in the queue.
        //!
        //! If the queue is full, the calling thread waits until some space becomes
        //! available in the queue or the timeout expires.
        //!
        //! @param [in,out] msg The message to enqueue. The ownership of the pointed object
        //! is transfered to the message queue. Upon return, the @a msg safe pointer becomes
        //! a null pointer if the message was successfully enqueued (no timeout).
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! @return True on success, false on error (queue still full after timeout).
        //!
        bool enqueue(MessagePtr& msg, MilliSecond timeout = Infinite);

        //!
        //! Insert a message in the queue.
        //!
        //! If the queue is full, the calling thread waits until some space becomes
        //! available in the queue or the timeout expires.
        //!
        //! @param [in] msg A pointer to the message to enqueue. This pointer shall not
        //! be owned by a safe pointer. When the message is successfully enqueued, the
        //! pointer becomes owned by a safe pointer and will be deallocated when no
        //! longer used. In case of timeout, the object is not equeued and immediately
        //! deallocated.
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! @return True on success, false on error (queue still full after timeout).
        //!
        bool enqueue(MSG* msg, MilliSecond timeout = Infinite);

        //!
        //! Insert a message in the queue, even if the queue is full.
        //!
        //! This method immediately inserts the message, even if the queue is full.
        //! This can be used to allow exceptional overflow of the queue with unique messages,
        //! to enqueue a message to instruct the consumer thread to terminate for instance.
        //!
        //! @param [in,out] msg The message to enqueue. The ownership of the pointed object
        //! is transfered to the message queue. Upon return, the @a msg safe pointer becomes
        //! a null pointer.
        //!
        void forceEnqueue(MessagePtr& msg);

        //!
        //! Insert a message in the queue, even if the queue is full.
        //!
        //! This method immediately inserts the message, even if the queue is full.
        //! This can be used to allow exceptional overflow of the queue with unique messages,
        //! to enqueue a message to instruct the consumer thread to terminate for instance.
        //!
        //! @param [in] msg A pointer to the message to enqueue. This pointer shall not
        //! be owned by a safe pointer. When the message is enqueued, the pointer becomes
        //! owned by a safe pointer and will be deallocated when no longer used.
        //!
        void forceEnqueue(MSG* msg);

        //!
        //! Remove a message from the queue.
        //!
        //! Wait until a message is received or the timeout expires.
        //!
        //! @param [out] msg Received message.
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! If @a timeout is zero and the queue is empty, return immediately.
        //! @return True on success, false on error (queue still empty after timeout).
        //!
        bool dequeue(MessagePtr& msg, MilliSecond timeout = Infinite);

        //!
        //! Peek the next message from the queue, without dequeueing it.
        //!
        //! If several threads simultaneously read from the queue, the returned
        //! message may be deqeued in the meantime by another thread.
        //!
        //! @return A safe pointer to the first message in the queue or a null pointer
        //! if the queue is empty.
        //!
        MessagePtr peek();

        //!
        //! Clear the content of the queue.
        //!
        void clear();

    protected:
        //!
        //! Queues are implemented as list of smart pointers to messages.
        //!
        typedef std::list<MessagePtr> MessageList;

        //!
        //! This virtual protected method performs placement in the message queue.
        //! @param [in] msg The message to enqueue later. The message is not enqueued.
        //! Its value is used to compute the place where it should be inserted.
        //! @param [in] list The content of the queue.
        //! @return An iterator to the place where @a msg shall be placed.
        //!
        virtual typename MessageList::iterator enqueuePlacement(const MessagePtr& msg, MessageList& list);

        //!
        //! This virtual protected method performs dequeue location in the message queue.
        //! @param [in] list The content of the queue.
        //! @return An iterator to the place from where the next message shall be removed.
        //!
        virtual typename MessageList::iterator dequeuePlacement(MessageList& list);

    private:
        // Private members.
        mutable Mutex     _mutex;        //!< Protect access to all private members
        mutable Condition _enqueued;     //!< Signaled when some message is inserted
        mutable Condition _dequeued;     //!< Signaled when some message is removed
        size_t            _maxMessages;  //!< Max number of messages in the queue
        MessageList       _queue;        //!< Actual message queue.

        // Enqueue a safe pointer in the list and signal the condition.
        // Must be executed under the protection of the lock.
        void enqueuePtr(const MessagePtr& ptr);

        // Wait for free space in the queue using a specific timeout, under the protection of the mutex.
        bool waitFreeSpace(GuardCondition& lock, MilliSecond timeout);
    };
}

#include "tsMessageQueueTemplate.h"
