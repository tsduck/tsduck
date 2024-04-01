//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::MessageQueue
//
//----------------------------------------------------------------------------

#include "tsMessageQueue.h"
#include "tsMessagePriorityQueue.h"
#include "tsSysUtils.h"
#include "tsTime.h"
#include "tsunit.h"
#include "utestTSUnitThread.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MessageQueueTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testConstructor();
    void testQueue();
    void testPriorityQueue();

    TSUNIT_TEST_BEGIN(MessageQueueTest);
    TSUNIT_TEST(testConstructor);
    TSUNIT_TEST(testQueue);
    TSUNIT_TEST(testPriorityQueue);
    TSUNIT_TEST_END();
private:
    cn::milliseconds _precision {};
};

TSUNIT_REGISTER(MessageQueueTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MessageQueueTest::beforeTest()
{
    _precision = cn::milliseconds(2);
    ts::SetTimersPrecision(_precision);
    debug() << "MessageQueueTest: timer precision = " << ts::UString::Chrono(_precision) << std::endl;
}

// Test suite cleanup method.
void MessageQueueTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

using TestQueue = ts::MessageQueue<int>;

// Test case: Constructor
void MessageQueueTest::testConstructor()
{
    TestQueue queue1;
    TestQueue queue2(10);

    TSUNIT_ASSERT(queue1.getMaxMessages() == 0);
    TSUNIT_ASSERT(queue2.getMaxMessages() == 10);

    queue1.setMaxMessages(27);
    TSUNIT_ASSERT(queue1.getMaxMessages() == 27);
}

// Thread for testQueue()
namespace {
    class MessageQueueTestThread: public utest::TSUnitThread
    {
    private:
        TestQueue& _queue;
    public:
        explicit MessageQueueTestThread(TestQueue& queue) :
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
            tsunit::Test::debug() << "MessageQueueTest: test thread: started" << std::endl;

            // Initial suspend of 500 ms
            std::this_thread::sleep_for(cn::milliseconds(500));

            // Read messages. Expect consecutive values until negative value.
            int expected = 0;
            TestQueue::MessagePtr message;
            do {
                TSUNIT_ASSERT(_queue.dequeue(message, cn::milliseconds(10000)));
                TSUNIT_ASSERT(message != nullptr);
                tsunit::Test::debug() << "MessageQueueTest: test thread: received " << *message << std::endl;
                if (*message >= 0) {
                    TSUNIT_ASSERT(*message == expected);
                    expected++;
                }
                // Make sure the main thread has the opportunity to insert the 11th message.
                ts::Thread::Yield();
            } while (*message >= 0);

            tsunit::Test::debug() << "MessageQueueTest: test thread: end" << std::endl;
        }
    };
}

void MessageQueueTest::testQueue()
{
    TestQueue queue(10);
    MessageQueueTestThread thread(queue);
    int message = 0;

    debug() << "MessageQueueTest: main thread: starting test" << std::endl;

    // Enqueue 10 message, should not fail.
    // First 2 messages are enqueued without timeout.
    queue.enqueue(new int(message++));
    queue.enqueue(new int(message++));

    // Next 8 messages are enqueued with 100 ms timeout.
    // No specific reason for this, simply test both versions of enqueue().
    while (message < 10) {
        TSUNIT_ASSERT(queue.enqueue(new int(message++), cn::milliseconds(100)));
    }

    // Start the thread
    const ts::Time start(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(thread.start());
    debug() << "MessageQueueTest: main thread: test thread started" << std::endl;

    // Enqueue 11th message with 50 ms timeout, should fail
    debug() << "MessageQueueTest: main thread: enqueueing " << message << " (should fail)" << std::endl;
    TSUNIT_ASSERT(!queue.enqueue(new int(message), cn::milliseconds(50)));

    // Enqueue message, should take at least 500 ms
    debug() << "MessageQueueTest: main thread: enqueueing " << message << " (10 s. timeout)" << std::endl;
    const bool enqueued = queue.enqueue(new int(message++), cn::milliseconds(10000));
    const cn::milliseconds duration = ts::Time::CurrentUTC() - start;
    debug() << "MessageQueueTest: main thread: enqueue = " << ts::UString::TrueFalse(enqueued) << ", duration = " << ts::UString::Chrono(duration) << std::endl;
    TSUNIT_ASSERT(enqueued);
    TSUNIT_ASSUME(duration >= cn::milliseconds(500) - 20 * _precision); // imprecisions accumulate on Windows

    // Enqueue exit request
    debug() << "MessageQueueTest: main thread: force enqueueing -1" << std::endl;
    queue.forceEnqueue(new int(-1));

    debug() << "MessageQueueTest: main thread: end of test" << std::endl;
}

void MessageQueueTest::testPriorityQueue()
{
    struct Message
    {
        int a;
        int b;
        Message(int a1 = 0, int b1 = 0) : a(a1), b(b1) {}
        bool operator<(const Message& other) const { return a < other.a; }
    };

    using Queue = ts::MessagePriorityQueue<Message>;
    Queue queue;
    Queue::MessagePtr msg;

    TSUNIT_ASSERT(queue.enqueue(new Message(1, 1), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(5, 2), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(2, 3), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(6, 4), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(3, 5), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(2, 6), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(0, 7), cn::milliseconds::zero()));
    TSUNIT_ASSERT(queue.enqueue(new Message(0, 8), cn::milliseconds::zero()));

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(7, msg->b);

    msg = queue.peek();
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(8, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(8, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(1, msg->a);
    TSUNIT_EQUAL(1, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(2, msg->a);
    TSUNIT_EQUAL(3, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(2, msg->a);
    TSUNIT_EQUAL(6, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(3, msg->a);
    TSUNIT_EQUAL(5, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(5, msg->a);
    TSUNIT_EQUAL(2, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, cn::milliseconds::zero()));
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(6, msg->a);
    TSUNIT_EQUAL(4, msg->b);

    TSUNIT_ASSERT(!queue.dequeue(msg, cn::milliseconds::zero()));
}
