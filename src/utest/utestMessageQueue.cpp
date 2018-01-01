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
//  CppUnit test suite for class ts::MessageQueue
//
//----------------------------------------------------------------------------

#include "tsMessageQueue.h"
#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
#include "utestCppUnitThread.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MessageQueueTest: public CppUnit::TestFixture
{
public:
    MessageQueueTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testConstructor();
    void testQueue();

    CPPUNIT_TEST_SUITE(MessageQueueTest);
    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testQueue);
    CPPUNIT_TEST_SUITE_END();
private:
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

CPPUNIT_TEST_SUITE_REGISTRATION (MessageQueueTest);


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
void MessageQueueTest::setUp()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    utest::Out() << "MonotonicTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, " << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void MessageQueueTest::tearDown()
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

    CPPUNIT_ASSERT(queue1.getMaxMessages() == 0);
    CPPUNIT_ASSERT(queue2.getMaxMessages() == 10);

    queue1.setMaxMessages(27);
    CPPUNIT_ASSERT(queue1.getMaxMessages() == 27);
}

// Thread for testQueue()
namespace {
    class MessageQueueTestThread: public utest::CppUnitThread
    {
    private:
        TestQueue& _queue;
    public:
        explicit MessageQueueTestThread(TestQueue& queue) :
            utest::CppUnitThread(),
            _queue(queue)
        {
        }

        virtual void test() override
        {
            utest::Out() << "MessageQueueTest: test thread: started" << std::endl;

            // Initial suspend of 500 ms
            ts::SleepThread(500);

            // Read messages. Expect consecutive values until negative value.
            int expected = 0;
            TestQueue::MessagePtr message;
            do {
                CPPUNIT_ASSERT(_queue.dequeue(message, 10000));
                CPPUNIT_ASSERT(!message.isNull());
                utest::Out() << "MessageQueueTest: test thread: received " << *message << std::endl;
                if (*message >= 0) {
                    CPPUNIT_ASSERT(*message == expected);
                    expected++;
                }
                // Make sure the main thread has the opportunity to insert the 11th message.
                ts::Thread::Yield();
            } while (*message >= 0);

            utest::Out() << "MessageQueueTest: test thread: end" << std::endl;
        }
    };
}

void MessageQueueTest::testQueue()
{
    TestQueue queue(10);
    MessageQueueTestThread thread(queue);
    int message = 0;

    utest::Out() << "MessageQueueTest: main thread: starting test" << std::endl;

    // Enqueue 10 message, should not fail.
    // First 2 messages are enqueued without timeout.
    CPPUNIT_ASSERT(queue.enqueue(new int(message++)));
    CPPUNIT_ASSERT(queue.enqueue(new int(message++)));

    // Next 8 messages are enqueued with 100 ms timeout.
    // No specific reason for this, simply test both versions of enqueue().
    while (message < 10) {
        CPPUNIT_ASSERT(queue.enqueue(new int(message++), 100));
    }

    // Start the thread
    const ts::Time start(ts::Time::CurrentUTC());
    CPPUNIT_ASSERT(thread.start());
    utest::Out() << "MessageQueueTest: main thread: test thread started" << std::endl;

    // Enqueue 11th message with 50 ms timeout, should fail
    utest::Out() << "MessageQueueTest: main thread: enqueueing " << message << " (should fail)" << std::endl;
    CPPUNIT_ASSERT(!queue.enqueue(new int(message), 50));

    // Enqueue message, should take at least 500 ms
    utest::Out() << "MessageQueueTest: main thread: enqueueing " << message << " (10 s. timeout)" << std::endl;
    const bool enqueued = queue.enqueue(new int(message++), 10000);
    const ts::MilliSecond duration = ts::Time::CurrentUTC() - start;
    utest::Out() << "MessageQueueTest: main thread: enqueue = " << ts::UString::TrueFalse(enqueued) << ", duration = " << ts::UString::Decimal(duration) << " ms" << std::endl;
    CPPUNIT_ASSERT(enqueued);
    CPPUNIT_ASSERT(duration >= 500 - 20 * _msPrecision); // imprecisions accumulate on Windows

    // Enqueue exit request
    utest::Out() << "MessageQueueTest: main thread: force enqueueing -1" << std::endl;
    queue.forceEnqueue(new int(-1));

    utest::Out() << "MessageQueueTest: main thread: end of test" << std::endl;
}
