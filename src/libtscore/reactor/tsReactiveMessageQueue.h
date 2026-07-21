//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic message queue for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveBase.h"
#include "tsReactiveMessageQueueHandlerInterface.h"

namespace ts {
    //!
    //! Generic message queue for use in a Reactor environment
    //! @ingroup libtscore reactor
    //!
    //! The template class ReactiveMessageQueue is a wrapper around MessageQueue to handle reactive events.
    //!
    //! The actual message queue is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call dequeue() on this message queue and delegate this operation to
    //! startReceive() in ReactiveMessageQueue.
    //!
    //! @tparam MSG The type of the messages to receive.
    //!
    template <typename MSG>
    class ReactiveMessageQueue: public ReactiveBase, private MessageQueueHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ReactiveMessageQueue);
    public:
        //!
        //! Common renaming of the queue type.
        //!
        using MessageQueueType = MessageQueue<MSG>;

        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] queue Associated message queue. The message queue object must remain valid as long as this object is valid.
        //!
        ReactiveMessageQueue(Reactor& reactor, MessageQueue<MSG>& queue);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveMessageQueue() override;

        //!
        //! Get a reference to the associated message queue.
        //! @return A reference to the associated socket.
        //!
        MessageQueue<MSG>& queue() { return _queue; }

        //!
        //! Declare the handler to receive dequeued messages.
        //! @param [in] handler Handler class to call each time a message is received. The method handleDequeuedMessage() will be called
        //! for each new message. If a previous receive handler was registered, it is replaced.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //!
        void startReceive(ReactiveMessageQueueHandlerInterface<MSG>* handler, const ObjectPtr& user_data = ObjectPtr());

    protected:
        // Inherited from ReactiveBase.
        virtual void processQueuedOperations() override;

    private:
        MessageQueue<MSG>& _queue;
        bool _subscribed = false;
        ReactiveMessageQueueHandlerInterface<MSG>* _handler = nullptr;
        ObjectPtr _user_data {};

        // Called when a message is enqueued. Called from any thread.
        virtual void handleMessageEnqueued(size_t queue_size) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Constructor.
template <typename MSG>
ts::ReactiveMessageQueue<MSG>::ReactiveMessageQueue(Reactor& reactor, MessageQueue<MSG>& queue) :
    ReactiveBase(reactor),
    _queue(queue)
{
}

// Destructor.
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename MSG>
ts::ReactiveMessageQueue<MSG>::~ReactiveMessageQueue()
{
}
TS_POP_WARNING()

// Declare the handler to receive dequeued messages.
template <typename MSG>
void ts::ReactiveMessageQueue<MSG>::startReceive(ReactiveMessageQueueHandlerInterface<MSG>* handler, const ObjectPtr& user_data)
{
    // Register the new handler.
    _handler = handler;
    _user_data = user_data;

    // Subscribe to enqueue/dequeue events in the message queue (only the first time).
    if (!_subscribed) {
        _subscribed = true;
        _queue.addSubscription(this);
    }

    // Force the creation of the internal user event and trigger an initial queue check (if not empty).
    signalQueuedOperations();
}

// Called when a message is enqueued. Called from any thread.
template <typename MSG>
void ts::ReactiveMessageQueue<MSG>::handleMessageEnqueued(size_t queue_size)
{
    // Trigger the execution of processQueuedOperations() in reactor context.
    uncheckedSignalQueuedOperations();
}

// Called in reactor context to dequeue messages.
template <typename MSG>
void ts::ReactiveMessageQueue<MSG>::processQueuedOperations()
{
    typename MessageQueue<MSG>::MessagePtr msg;
    while (_queue.dequeue(msg, cn::milliseconds::zero())) {
        if (_handler != nullptr) {
            _handler->handleDequeuedMessage(*this, msg, _user_data);
        }
    }
}
