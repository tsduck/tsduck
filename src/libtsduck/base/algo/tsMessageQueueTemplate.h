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

#include "tsGuardMutex.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
ts::MessageQueue<MSG, MUTEX>::MessageQueue(size_t maxMessages) :
    _mutex(),
    _enqueued(),
    _dequeued(),
    _maxMessages(maxMessages),
    _queue()
{
}

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename MSG, class MUTEX>
ts::MessageQueue<MSG, MUTEX>::~MessageQueue()
{
}
TS_POP_WARNING()


//----------------------------------------------------------------------------
// Access max allowed messages in queue (0 means unlimited)
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
size_t ts::MessageQueue<MSG, MUTEX>::getMaxMessages() const
{
    GuardMutex lock(_mutex);
    return _maxMessages;
}

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::setMaxMessages(size_t max)
{
    GuardMutex lock(_mutex);
    _maxMessages = max;
}


//----------------------------------------------------------------------------
// Placement in the message queue (virtual protected methods).
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
typename ts::MessageQueue<MSG, MUTEX>::MessageList::iterator
    ts::MessageQueue<MSG, MUTEX>::enqueuePlacement(const MessagePtr& msg, MessageList& list)
{
    // The default placement is pushing at the back of the queue.
    return list.end();

}

template <typename MSG, class MUTEX>
typename ts::MessageQueue<MSG, MUTEX>::MessageList::iterator
    ts::MessageQueue<MSG, MUTEX>::dequeuePlacement(MessageList& list)
{
    // The default placement is fetching from the head of the queue.
    return list.begin();
}



//----------------------------------------------------------------------------
// Enqueue a safe pointer in the list and signal the condition.
// Must be executed under the protection of the lock.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::enqueuePtr(const MessagePtr& ptr)
{
    _queue.insert(enqueuePlacement(ptr, _queue), ptr);
    _enqueued.signal();
}


//----------------------------------------------------------------------------
// Wait for free space in the queue using a specific timeout.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
bool ts::MessageQueue<MSG, MUTEX>::waitFreeSpace(GuardCondition& lock, MilliSecond timeout)
{
    // If the queue is full, wait for the queue not being full.
    if (_maxMessages != 0 && timeout > 0) {
        Time start(Time::CurrentUTC());
        while (_queue.size() >= _maxMessages) {

            // Reduce timeout
            if (timeout != Infinite) {
                const Time now(Time::CurrentUTC());
                timeout -= now - start;
                start = now;
                if (timeout <= 0) {
                    break; // timeout
                }
            }

            // Wait for a message to be dequeued
            // => temporarily release mutex and wait for dequeued condition.
            if (!lock.waitCondition(timeout)) {
                break; // timeout
            }
        }
    }

    // Now, may we enqueue the message?
    return _maxMessages == 0 || _queue.size() < _maxMessages;
}


//----------------------------------------------------------------------------
// Insert a message in the queue with a timeout.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
bool ts::MessageQueue<MSG, MUTEX>::enqueue(MessagePtr& msg, MilliSecond timeout)
{
    // Take mutex, potentially wait on dequeued condition.
    // Note that we lock the mutex _without_ timeout. Nobody keeps the mutex longer
    // than accessing a field. So the timeout does not apply here. The timeout applies
    // on waiting for space in the queue.
    GuardCondition lock(_mutex, _dequeued);

    if (waitFreeSpace(lock, timeout)) {
        // Successfully waited for free space in the queue.
        // Transfer ownership of the pointed object inside a code block which guarantees
        // that the new safe pointer will be destructed before releasing the lock.
        const MessagePtr transfered(msg.release());
        enqueuePtr(transfered);
        return true;
    }
    else {
        // Timeout, queue still full.
        return false;
    }
}

template <typename MSG, class MUTEX>
bool ts::MessageQueue<MSG, MUTEX>::enqueue(MSG* msg, MilliSecond timeout)
{
    // Same code template as above.
    GuardCondition lock(_mutex, _dequeued);

    if (waitFreeSpace(lock, timeout)) {
        // Create a safe pointer to the pointed object inside a code block which guarantees
        // that the safe pointer will be destructed before releasing the lock.
        const MessagePtr ptr(msg);
        enqueuePtr(ptr);
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

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::forceEnqueue(MessagePtr& msg)
{
    GuardMutex lock(_mutex);
    {
        // Transfer ownership of the pointed object inside a code block which guarantees
        // that the new safe pointer will be destructed before releasing the lock.
        const MessagePtr transfered(msg.release());
        enqueuePtr(transfered);
    }
}

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::forceEnqueue(MSG* msg)
{
    GuardMutex lock(_mutex);
    {
        // Create a safe pointer to the pointed object inside a code block which guarantees
        // that the safe pointer will be destructed before releasing the lock.
        const MessagePtr ptr(msg);
        enqueuePtr(ptr);
    }
}


//----------------------------------------------------------------------------
// Remove a message from the queue.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
bool ts::MessageQueue<MSG, MUTEX>::dequeue(MessagePtr& msg, MilliSecond timeout)
{
    // Take mutex, potentially wait on enqueued condition.
    // Note that we lock the mutex _without_ timeout. Nobody keeps the mutex longer
    // than accessing a field. So the timeout does not apply here. The timeout applies
    // on waiting for a message from an empty queue.
    GuardCondition lock(_mutex, _enqueued);

    // If the timeout is non-zero, wait for the queue not being empty.
    if (timeout > 0) {
        Time start(Time::CurrentUTC());
        while (_queue.empty()) {

            // Reduce timeout
            if (timeout != Infinite) {
                const Time now(Time::CurrentUTC());
                timeout -= now - start;
                start = now;
                if (timeout <= 0) {
                    break; // timeout
                }
            }

            // Wait for a message to be enqueued
            // => temporarily release mutex and wait for enqueued condition.
            if (!lock.waitCondition(timeout)) {
                break; // timeout
            }
        }
    }

    // Now, attempt to dequeue a message.
    const auto it = dequeuePlacement(_queue);
    if (it == _queue.end()) {
        // Queue empty or nothing to queue, no message
        return false;
    }
    else {
        // Queue not empty, remove a message
        msg = *it;
        _queue.erase(it);

        // Signal that a message has been dequeued
        _dequeued.signal();
        return true;
    }
}


//----------------------------------------------------------------------------
// Peek the next message from the queue, without dequeueing it.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
typename ts::MessageQueue<MSG, MUTEX>::MessagePtr ts::MessageQueue<MSG, MUTEX>::peek()
{
    GuardMutex lock(_mutex);
    const auto it = dequeuePlacement(_queue);
    return it == _queue.end() ? MessagePtr() : *it;
}


//----------------------------------------------------------------------------
// Clear the queue.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::clear()
{
    GuardMutex lock(_mutex);
    if (!_queue.empty()) {
        _queue.clear();
        // Signal that messages have been dequeued (dropped in fact).
        _dequeued.signal();
    }
}
