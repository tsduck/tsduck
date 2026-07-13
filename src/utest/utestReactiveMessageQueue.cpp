//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ReactiveMessageQueue
//
//----------------------------------------------------------------------------

#include "tsReactiveMessageQueue.h"
#include "tsunit.h"
#include "utestTSUnitThread.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactiveMessageQueueTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Queue);
};

TSUNIT_REGISTER(ReactiveMessageQueueTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------


namespace {

    class MessageQueueTestThread: public utest::TSUnitThread
    {
    private:
        ts::MessageQueue<int>& _queue;
    public:
        explicit MessageQueueTestThread(ts::MessageQueue<int>& queue) :
            utest::TSUnitThread(),
            _queue(queue)
        {
        }

        virtual ~MessageQueueTestThread() override
        {
            waitForTermination();
        }

        virtual void test() override
        {
            tsunit::Test::debug() << "ReactiveMessageQueueTest: test thread: started" << std::endl;

            // Initial suspend of 490 ms
            std::this_thread::sleep_for(cn::milliseconds(490));

            // Send 3 messages every 10 ms.
            for (int i = 1; i < 4; ++i) {
                std::this_thread::sleep_for(cn::milliseconds(10));
                auto msg = std::make_shared<int>(i);
                _queue.enqueue(msg);
            }

            // Enqueue a null pointer to signal end.
            _queue.enqueue(nullptr);
            tsunit::Test::debug() << "ReactiveMessageQueueTest: test thread: end" << std::endl;
        }
    };

    class MessageQueueTestHandler: public ts::ReactiveMessageQueueHandlerInterface<int>
    {
    public:
        int last_message = 0;

        virtual void handleDequeuedMessage(ts::ReactiveMessageQueue<int>& queue, ts::MessageQueue<int>::MessagePtr& msg, const ts::ObjectPtr& user_data) override
        {
            if (msg == nullptr) {
                tsunit::Test::debug() << "ReactiveMessageQueueTest: MessageQueueTestHandler: end" << std::endl;
                queue.reactor().exitEventLoop();
            }
            else {
                tsunit::Test::debug() << "ReactiveMessageQueueTest: MessageQueueTestHandler: message = " << *msg << std::endl;
                TSUNIT_EQUAL(++last_message, *msg);
            }
        }
    };
}

TSUNIT_DEFINE_TEST(Queue)
{
    ts::Reactor reactor(&CERR);
    ts::MessageQueue<int> queue;
    ts::ReactiveMessageQueue<int> rqueue(reactor, queue);
    MessageQueueTestThread thread(queue);
    MessageQueueTestHandler handler;

    debug() << "ReactiveMessageQueueTest: main thread: starting test" << std::endl;

    TSUNIT_ASSERT(reactor.open());
    rqueue.startReceive(&handler);
    TSUNIT_ASSERT(thread.start());
    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());
    TSUNIT_EQUAL(3, handler.last_message);

    debug() << "MessageQueueTest: main thread: end of test" << std::endl;
}
