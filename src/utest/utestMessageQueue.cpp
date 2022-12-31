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
//
//  TSUnit test suite for class ts::MessageQueue
//
//----------------------------------------------------------------------------

#include "tsMessageQueue.h"
#include "tsMessagePriorityQueue.h"
#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "tsunit.h"
#include "utestTSUnitThread.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MessageQueueTest: public tsunit::Test
{
public:
    MessageQueueTest();

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
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

TSUNIT_REGISTER(MessageQueueTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
MessageQueueTest::MessageQueueTest() :
    _nsPrecision(0),
    _msPrecision(0)
{
}

// Test suite initialization method.
void MessageQueueTest::beforeTest()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    debug() << "MonotonicTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, " << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void MessageQueueTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

typedef ts::MessageQueue<int> TestQueue;

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
            ts::SleepThread(500);

            // Read messages. Expect consecutive values until negative value.
            int expected = 0;
            TestQueue::MessagePtr message;
            do {
                TSUNIT_ASSERT(_queue.dequeue(message, 10000));
                TSUNIT_ASSERT(!message.isNull());
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
    TSUNIT_ASSERT(queue.enqueue(new int(message++)));
    TSUNIT_ASSERT(queue.enqueue(new int(message++)));

    // Next 8 messages are enqueued with 100 ms timeout.
    // No specific reason for this, simply test both versions of enqueue().
    while (message < 10) {
        TSUNIT_ASSERT(queue.enqueue(new int(message++), 100));
    }

    // Start the thread
    const ts::Time start(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(thread.start());
    debug() << "MessageQueueTest: main thread: test thread started" << std::endl;

    // Enqueue 11th message with 50 ms timeout, should fail
    debug() << "MessageQueueTest: main thread: enqueueing " << message << " (should fail)" << std::endl;
    TSUNIT_ASSERT(!queue.enqueue(new int(message), 50));

    // Enqueue message, should take at least 500 ms
    debug() << "MessageQueueTest: main thread: enqueueing " << message << " (10 s. timeout)" << std::endl;
    const bool enqueued = queue.enqueue(new int(message++), 10000);
    const ts::MilliSecond duration = ts::Time::CurrentUTC() - start;
    debug() << "MessageQueueTest: main thread: enqueue = " << ts::UString::TrueFalse(enqueued) << ", duration = " << ts::UString::Decimal(duration) << " ms" << std::endl;
    TSUNIT_ASSERT(enqueued);
    TSUNIT_ASSUME(duration >= 500 - 20 * _msPrecision); // imprecisions accumulate on Windows

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

    ts::MessagePriorityQueue<Message> queue;
    ts::MessagePriorityQueue<Message>::MessagePtr msg;

    TSUNIT_ASSERT(queue.enqueue(new Message(1, 1), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(5, 2), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(2, 3), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(6, 4), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(3, 5), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(2, 6), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(0, 7), 0));
    TSUNIT_ASSERT(queue.enqueue(new Message(0, 8), 0));

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(7, msg->b);

    msg = queue.peek();
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(8, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(0, msg->a);
    TSUNIT_EQUAL(8, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(1, msg->a);
    TSUNIT_EQUAL(1, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(2, msg->a);
    TSUNIT_EQUAL(3, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(2, msg->a);
    TSUNIT_EQUAL(6, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(3, msg->a);
    TSUNIT_EQUAL(5, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(5, msg->a);
    TSUNIT_EQUAL(2, msg->b);

    TSUNIT_ASSERT(queue.dequeue(msg, 0));
    TSUNIT_ASSERT(!msg.isNull());
    TSUNIT_EQUAL(6, msg->a);
    TSUNIT_EQUAL(4, msg->b);

    TSUNIT_ASSERT(!queue.dequeue(msg, 0));
}
