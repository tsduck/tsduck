//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Template message queue for inter-thread communication
//
//----------------------------------------------------------------------------

#include "tsGuard.h"
#include "tsGuardCondition.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Constructor.
// If max_messages is 0, the queue is unlimited.
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


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
ts::MessageQueue<MSG, MUTEX>::~MessageQueue()
{
}


//----------------------------------------------------------------------------
// Access max allowed messages in queue (0 means unlimited)
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
size_t ts::MessageQueue<MSG, MUTEX>::getMaxMessages() const
{
    Guard lock(_mutex);
    return _maxMessages;
}

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::setMaxMessages(size_t max)
{
    Guard lock(_mutex);
    _maxMessages = max;
}


//----------------------------------------------------------------------------
// Insert a message in the queue, even if the queue is full.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
void ts::MessageQueue<MSG, MUTEX>::forceEnqueue(const MessagePtr& msg)
{
    Guard lock(_mutex);

    // Enqueue the message
    _queue.push_back(msg);

    // Signal that a message has been enqueued
    _enqueued.signal();
}


//----------------------------------------------------------------------------
// Insert a message in the queue with a timeout.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX>
bool ts::MessageQueue<MSG, MUTEX>::enqueue(const MessagePtr& msg, MilliSecond timeout)
{
    // Take mutex, potentially wait on dequeued condition.
    // Note that we lock the mutex _without_ timeout. Nobody keeps the mutex longer
    // than accessing a field. So the timeout does not apply here. The timeout applies
    // on waiting for space in the queue.
    GuardCondition lock(_mutex, _dequeued);

    // If the queue is full, wait for the queue not being full
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
    if (_maxMessages != 0 && _queue.size() >= _maxMessages) {
        // Queue is full
        return false;
    }

    // Enqueue the message
    _queue.push_back(msg);

    // Signal that a message has been enqueued
    _enqueued.signal();
    return true;
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

    // Now, attempt to dequeue a message
    if (_queue.empty()) {
        // Queue empty, no message
        return false;
    }
    else {
        // Queue not empty, remove a message
        msg = _queue.front();
        _queue.pop_front();

        // Signal that a message has been dequeued
        _dequeued.signal();
        return true;
    }
}
