//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Template message queue for inter-thread communication
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Template message queue for inter-thread communication.
    //! @ingroup libtscore thread
    //!
    //! The ts::MessageQueue template class implements a synchronized
    //! access to a shared queue of generic messages.
    //!
    //! @tparam MSG The type of the messages to exchange.
    //!
    template <typename MSG>
    class MessageQueue
    {
        TS_NOCOPY(MessageQueue);
    public:
        //!
        //! Safe pointer to messages.
        //!
        //! Since data are copied from the producer thread into the queue and later copied
        //! again from the queue into the consumer thread, the copied data is always a
        //! shared pointer to the actual message content.
        //!
        using MessagePtr = std::shared_ptr<MSG>;

        //!
        //! Constructor.
        //! @param [in] maxMessages Maximum number of messages in the queue.
        //! @see setMaxMessages()
        //!
        MessageQueue(size_t maxMessages = 0);

        //!
        //! Destructor
        //!
        virtual ~MessageQueue();

        //!
        //! Get the maximum allowed messages in the queue.
        //! @return The maximum allowed messages in the queue (0 means unlimited).
        //!
        size_t getMaxMessages() const;

        //!
        //! Change the maximum allowed messages in the queue.
        //! @param [in] maxMessages Maximum number of messages in the queue. When a thread attempts to
        //! enqueue a message and the queue is full, the thread waits until at least one message is dequeued.
        //! If @a maxMessages is 0, the queue is unlimited. In that case, the logic of the application must
        //! ensure that the queue is bounded somehow, otherwise the queue may exhaust all the process memory.
        //!
        void setMaxMessages(size_t maxMessages);

        //!
        //! Insert a message in the queue.
        //! If the queue is full, the calling thread waits until some space becomes available in the queue.
        //! @param [in,out] msg The message to enqueue. The ownership of the pointed object
        //! is transfered to the message queue. Upon return, the @a msg safe pointer becomes
        //! a null pointer if the message was successfully enqueued.
        //!
        void enqueue(MessagePtr& msg);

        //!
        //! Insert a message in the queue.
        //! If the queue is full, the calling thread waits until some space becomes
        //! available in the queue or the timeout expires.
        //! @param [in,out] msg The message to enqueue. The ownership of the pointed object
        //! is transfered to the message queue. Upon return, the @a msg safe pointer becomes
        //! a null pointer if the message was successfully enqueued (no timeout).
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! @return True on success, false on error (queue still full after timeout).
        //!
        bool enqueue(MessagePtr& msg, cn::milliseconds timeout);

        //!
        //! Insert a message in the queue.
        //! @param [in] msg A pointer to the message to enqueue. This pointer shall not
        //! be owned by a safe pointer. When the message is successfully enqueued, the
        //! pointer becomes owned by a safe pointer and will be deallocated when no
        //! longer used.
        //!
        void enqueue(MSG* msg);

        //!
        //! Insert a message in the queue.
        //! @param [in] msg A pointer to the message to enqueue. This pointer shall not
        //! be owned by a safe pointer. When the message is successfully enqueued, the
        //! pointer becomes owned by a safe pointer and will be deallocated when no
        //! longer used. In case of timeout, the object is not equeued and immediately
        //! deallocated.
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! @return True on success, false on error (queue still full after timeout).
        //!
        bool enqueue(MSG* msg, cn::milliseconds timeout);

        //!
        //! Insert a message in the queue, even if the queue is full.
        //! This method immediately inserts the message, even if the queue is full.
        //! This can be used to allow exceptional overflow of the queue with unique messages,
        //! to enqueue a message to instruct the consumer thread to terminate for instance.
        //! @param [in,out] msg The message to enqueue. The ownership of the pointed object
        //! is transfered to the message queue. Upon return, the @a msg safe pointer becomes
        //! a null pointer.
        //!
        void forceEnqueue(MessagePtr& msg);

        //!
        //! Insert a message in the queue, even if the queue is full.
        //! This method immediately inserts the message, even if the queue is full.
        //! This can be used to allow exceptional overflow of the queue with unique messages,
        //! to enqueue a message to instruct the consumer thread to terminate for instance.
        //! @param [in] msg A pointer to the message to enqueue. This pointer shall not
        //! be owned by a safe pointer. When the message is enqueued, the pointer becomes
        //! owned by a safe pointer and will be deallocated when no longer used.
        //!
        void forceEnqueue(MSG* msg);

        //!
        //! Remove a message from the queue.
        //! Wait until a message is received.
        //! @param [out] msg Received message.
        //!
        void dequeue(MessagePtr& msg);

        //!
        //! Remove a message from the queue.
        //! Wait until a message is received or the timeout expires.
        //! @param [out] msg Received message.
        //! @param [in] timeout Maximum time to wait in milliseconds.
        //! If @a timeout is zero and the queue is empty, return immediately.
        //! @return True on success, false on error (queue still empty after timeout).
        //!
        bool dequeue(MessagePtr& msg, cn::milliseconds timeout);

        //!
        //! Peek the next message from the queue, without dequeueing it.
        //! If several threads simultaneously read from the queue, the returned
        //! message may be deqeued in the meantime by another thread.
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
        using MessageList = std::list<MessagePtr>;

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
        // The _mutex is used to syn
        // Private members.
        mutable std::mutex              _mutex {};         // Protect access to all private members (must be std::mutext, not MutexType)
        mutable std::condition_variable _enqueued {};      // Signaled when some message is inserted
        mutable std::condition_variable _dequeued {};      // Signaled when some message is removed
        size_t                          _maxMessages = 0;  // Max number of messages in the queue
        MessageList                     _queue {};         // Actual message queue

        // Enqueue/dequeue a safe pointer in the list and signal the corresponding condition.
        // Dequeue returns false if the list is empty. Must be executed under the protection of the lock.
        void enqueuePtr(const MessagePtr& ptr);
        bool dequeuePtr(MessagePtr& ptr);

        // Wait for free space in the queue using a specific timeout, under the protection of the mutex.
        void waitFreeSpace(std::unique_lock<std::mutex>& lock);
        bool waitFreeSpace(std::unique_lock<std::mutex>& lock, cn::milliseconds timeout);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <typename MSG>
ts::MessageQueue<MSG>::MessageQueue(size_t maxMessages) :
    _maxMessages(maxMessages)
{
}

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename MSG>
ts::MessageQueue<MSG>::~MessageQueue()
{
}
TS_POP_WARNING()


//----------------------------------------------------------------------------
// Access max allowed messages in queue (0 means unlimited)
//----------------------------------------------------------------------------

template <typename MSG>
size_t ts::MessageQueue<MSG>::getMaxMessages() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _maxMessages;
}

template <typename MSG>
void ts::MessageQueue<MSG>::setMaxMessages(size_t max)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _maxMessages = max;
}


//----------------------------------------------------------------------------
// Placement in the message queue (virtual protected methods).
//----------------------------------------------------------------------------

template <typename MSG>
typename ts::MessageQueue<MSG>::MessageList::iterator
ts::MessageQueue<MSG>::enqueuePlacement(const MessagePtr& msg, MessageList& list)
{
    // The default placement is pushing at the back of the queue.
    return list.end();

}

template <typename MSG>
typename ts::MessageQueue<MSG>::MessageList::iterator
ts::MessageQueue<MSG>::dequeuePlacement(MessageList& list)
{
    // The default placement is fetching from the head of the queue.
    return list.begin();
}


//----------------------------------------------------------------------------
// Enqueue/dequeue a safe pointer in the list and signal the condition.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::enqueuePtr(const MessagePtr& ptr)
{
    _queue.insert(enqueuePlacement(ptr, _queue), ptr);
    _enqueued.notify_all();
}

template <typename MSG>
bool ts::MessageQueue<MSG>::dequeuePtr(MessagePtr& ptr)
{
    const auto it = dequeuePlacement(_queue);
    if (it == _queue.end()) {
        // Queue empty or nothing to dequeue, no message
        return false;
    }
    else {
        // Queue not empty, remove a message
        ptr = *it;
        _queue.erase(it);

        // Signal that a message has been dequeued
        _dequeued.notify_all();
        return true;
    }
}


//----------------------------------------------------------------------------
// Wait for free space in the queue using a specific timeout.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::waitFreeSpace(std::unique_lock<std::mutex>& lock)
{
    if (_maxMessages != 0) {
        _dequeued.wait(lock, [this]() { return _queue.size() < _maxMessages; });
    }
}

template <typename MSG>
bool ts::MessageQueue<MSG>::waitFreeSpace(std::unique_lock<std::mutex>& lock, cn::milliseconds timeout)
{
    return _maxMessages == 0 || _dequeued.wait_for(lock, timeout, [this]() { return _queue.size() < _maxMessages; });
}


//----------------------------------------------------------------------------
// Insert a message.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::enqueue(MessagePtr& msg)
{
    std::unique_lock<std::mutex> lock(_mutex);
    waitFreeSpace(lock);
    enqueuePtr(msg);
    msg.reset();
}

template <typename MSG>
bool ts::MessageQueue<MSG>::enqueue(MessagePtr& msg, cn::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (waitFreeSpace(lock, timeout)) {
        enqueuePtr(msg);
        msg.reset();
        return true;
    }
    else {
        // Timeout, queue still full.
        return false;
    }
}

template <typename MSG>
void ts::MessageQueue<MSG>::enqueue(MSG* msg)
{
    std::unique_lock<std::mutex> lock(_mutex);
    waitFreeSpace(lock);
    enqueuePtr(MessagePtr(msg));
}

template <typename MSG>
bool ts::MessageQueue<MSG>::enqueue(MSG* msg, cn::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (waitFreeSpace(lock, timeout)) {
        enqueuePtr(MessagePtr(msg));
        return true;
    }
    else {
        // Timeout, queue still full. Deallocated the message.
        delete msg;
        return false;
    }
}


//----------------------------------------------------------------------------
// Insert a message in the queue, even if the queue is full.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::forceEnqueue(MessagePtr& msg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    enqueuePtr(msg);
    msg.reset();
}

template <typename MSG>
void ts::MessageQueue<MSG>::forceEnqueue(MSG* msg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    enqueuePtr(MessagePtr(msg));
}


//----------------------------------------------------------------------------
// Remove a message from the queue.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::dequeue(MessagePtr& msg)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _enqueued.wait(lock, [this]() { return !_queue.empty(); });
    if (!dequeuePtr(msg)) {
        // Queue cannot be empty at end of wait without timeout.
        msg.reset();
    }
}


template <typename MSG>
bool ts::MessageQueue<MSG>::dequeue(MessagePtr& msg, cn::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _enqueued.wait_for(lock, timeout, [this]() { return !_queue.empty(); });
    return dequeuePtr(msg);
}


//----------------------------------------------------------------------------
// Peek the next message from the queue, without dequeueing it.
//----------------------------------------------------------------------------

template <typename MSG>
typename ts::MessageQueue<MSG>::MessagePtr ts::MessageQueue<MSG>::peek()
{
    std::lock_guard<std::mutex> lock(_mutex);
    const auto it = dequeuePlacement(_queue);
    return it == _queue.end() ? MessagePtr() : *it;
}


//----------------------------------------------------------------------------
// Clear the queue.
//----------------------------------------------------------------------------

template <typename MSG>
void ts::MessageQueue<MSG>::clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_queue.empty()) {
        _queue.clear();
        // Signal that messages have been dequeued (dropped in fact).
        _dequeued.notify_all();
    }
}
