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
//  TSUnit test suite for class ts::Thread
//
//  This is also a test suite for classes ts::Mutex and ts::Condition
//  which are difficult to test independently of threads.
//
//----------------------------------------------------------------------------

#include "tsThread.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuardCondition.h"
#include "tsGuardMutex.h"
#include "tsTime.h"
#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "utestTSUnitThread.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ThreadTest: public tsunit::Test
{
public:
    ThreadTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAttributes();
    void testTermination();
    void testDeleteWhenTerminated();
    void testMutexRecursion();
    void testMutexTimeout();
    void testCondition();

    TSUNIT_TEST_BEGIN(ThreadTest);
    TSUNIT_TEST(testAttributes);
    TSUNIT_TEST(testTermination);
    TSUNIT_TEST(testDeleteWhenTerminated);
    TSUNIT_TEST(testMutexRecursion);
    TSUNIT_TEST(testMutexTimeout);
    TSUNIT_TEST(testCondition);
    TSUNIT_TEST_END();
private:
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

TSUNIT_REGISTER(ThreadTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
ThreadTest::ThreadTest() :
    _nsPrecision(0),
    _msPrecision(0)
{
}

// Test suite initialization method.
void ThreadTest::beforeTest()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    debug() << "ThreadTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, " << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void ThreadTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

//
// Test case: Constructor with attributes
//
namespace {
    class ThreadConstructor: public utest::TSUnitThread
    {
    public:
        explicit ThreadConstructor(const ts::ThreadAttributes& attributes) :
            utest::TSUnitThread(attributes)
        {
        }
        virtual ~ThreadConstructor() override
        {
            waitForTermination();
        }
        virtual void test() override
        {
            TSUNIT_FAIL("ThreadConstructor should not have started");
        }
    };
}

void ThreadTest::testAttributes()
{
    const int prio = ts::ThreadAttributes::GetMinimumPriority();
    ThreadConstructor thread(ts::ThreadAttributes().setStackSize(123456).setPriority(prio));
    ts::ThreadAttributes attr;
    thread.getAttributes(attr);
    TSUNIT_ASSERT(attr.getPriority() == prio);
    TSUNIT_ASSERT(attr.getStackSize() == 123456);

    ts::ThreadAttributes attr2;
    TSUNIT_ASSERT(thread.setAttributes(attr2));
}

//
// Test case: Ensure that destructor waits for termination.
// The will slow down our test suites by 200 ms.
//
namespace {
    class ThreadTermination: public utest::TSUnitThread
    {
    private:
        volatile bool&  _report;
        ts::MilliSecond _delay;
        ts::MilliSecond _msPrecision;
    public:
        ThreadTermination (volatile bool& report, ts::MilliSecond delay, ts::MilliSecond msPrecision) :
            utest::TSUnitThread(ts::ThreadAttributes().setStackSize(1000000)),
            _report(report),
            _delay(delay),
            _msPrecision(msPrecision)
        {
        }
        virtual ~ThreadTermination() override
        {
            waitForTermination();
        }
        virtual void test() override
        {
            TSUNIT_ASSERT(isCurrentThread());
            const ts::Time before(ts::Time::CurrentUTC());
            ts::SleepThread(_delay);
            const ts::Time after(ts::Time::CurrentUTC());
            tsunit::Test::debug() << "ThreadTest::ThreadTermination: delay = " << _delay << ", after - before = " << (after - before) << std::endl;
            TSUNIT_ASSERT(after >= before + _delay - _msPrecision);
            _report = true;
        }
    };
}

void ThreadTest::testTermination()
{
    volatile bool report = false;
    const ts::Time before(ts::Time::CurrentUTC());
    {
        ThreadTermination thread(report, 200, _msPrecision);
        TSUNIT_ASSERT(thread.start());
        TSUNIT_ASSERT(!thread.isCurrentThread());
    }
    const ts::Time after(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(after >= before + 200 - _msPrecision);
    TSUNIT_ASSERT(report);
}


//
// Test case: Ensure that the "delete when terminated" flag
// properly cleanup the thread.
//
namespace {
    class ThreadDeleteWhenTerminated: public utest::TSUnitThread
    {
    private:
        volatile bool&  _report;
        ts::MilliSecond _delay;
        ts::MilliSecond _msPrecision;
    public:
        ThreadDeleteWhenTerminated (volatile bool& report, ts::MilliSecond delay, ts::MilliSecond msPrecision) :
            utest::TSUnitThread(ts::ThreadAttributes().setStackSize(1000000).setDeleteWhenTerminated(true)),
            _report(report),
            _delay(delay),
            _msPrecision(msPrecision)
        {
        }
        virtual ~ThreadDeleteWhenTerminated() override
        {
            waitForTermination();
            tsunit::Test::debug() << "ThreadTest: ThreadDeleteWhenTerminated deleted" << std::endl;
            _report = true;
        }
        virtual void test() override
        {
            const ts::Time before(ts::Time::CurrentUTC());
            ts::SleepThread(_delay);
            const ts::Time after(ts::Time::CurrentUTC());
            TSUNIT_ASSERT(after >= before + _delay - _msPrecision);
        }
    };
}

void ThreadTest::testDeleteWhenTerminated()
{
    volatile bool report = false;
    const ts::Time before(ts::Time::CurrentUTC());
    ThreadDeleteWhenTerminated* thread = new ThreadDeleteWhenTerminated(report, 100, _msPrecision);
    TSUNIT_ASSERT(thread->start());
    int counter = 100;
    while (!report && counter-- > 0) {
        ts::SleepThread(20);
    }
    const ts::Time after(ts::Time::CurrentUTC());
    if (counter > 0) {
        debug() << "ThreadTest::testDeleteWhenTerminated: ThreadDeleteWhenTerminated deleted after " << (after - before) << " milliseconds" << std::endl;
    }
    else {
        TSUNIT_FAIL(ts::UString::Format(u"Thread with \"delete when terminated\" not deleted after %d milliseconds", {after - before}).toUTF8());
    }
}

//
// Test case: Check mutex recursion
//
void ThreadTest::testMutexRecursion()
{
    ts::Mutex mutex;

    TSUNIT_ASSERT(!mutex.release());

    TSUNIT_ASSERT(mutex.acquire());
    TSUNIT_ASSERT(mutex.acquire());
    TSUNIT_ASSERT(mutex.acquire());

    TSUNIT_ASSERT(mutex.release());
    TSUNIT_ASSERT(mutex.release());
    TSUNIT_ASSERT(mutex.release());

    TSUNIT_ASSERT(!mutex.release());
    TSUNIT_ASSERT(!mutex.release());
}

//
// Test case: Check mutex timeout
//
namespace {
    class TestThreadMutexTimeout: public utest::TSUnitThread
    {
    private:
        ts::Mutex& _mutex;
        ts::Mutex& _mutexSig;
        ts::Condition& _condSig;
    public:
        TestThreadMutexTimeout(ts::Mutex& mutex, ts::Mutex& mutexSig, ts::Condition& condSig) :
            utest::TSUnitThread(),
            _mutex(mutex),
            _mutexSig(mutexSig),
            _condSig(condSig)
        {
        }
        virtual ~TestThreadMutexTimeout() override
        {
            waitForTermination();
        }
        virtual void test() override
        {
            // Acquire the test mutex.
            ts::GuardMutex lock1(_mutex, 0);
            TSUNIT_ASSERT(lock1.isLocked());
            // Signal that we have acquired it.
            {
                ts::GuardCondition lock2(_mutexSig, _condSig);
                TSUNIT_ASSERT(lock2.isLocked());
                lock2.signal();
            }
            // And sleep 100 ms.
            ts::SleepThread(100);
            // The test mutex is implicitely released.
        }
    };
}

void ThreadTest::testMutexTimeout()
{
    ts::Mutex mutex;
    ts::Mutex mutexSig;
    ts::Condition condSig;
    TestThreadMutexTimeout thread(mutex, mutexSig, condSig);

    // Start thread and wait for it to acquire mutex.
    {
        ts::GuardCondition lock(mutexSig, condSig);
        TSUNIT_ASSERT(lock.isLocked());
        TSUNIT_ASSERT(thread.start());
        TSUNIT_ASSERT(lock.waitCondition());
    }

    // Now, the thread holds the mutex for 100 ms.
    const ts::Time start(ts::Time::CurrentUTC());
    const ts::Time dueTime1(start + 50 - _msPrecision);
    const ts::Time dueTime2(start + 100 - _msPrecision);

    // Use assumptions instead of assertions for time-dependent checks.
    // Timing can be very weird on virtual machines which are used for unitary tests.
    TSUNIT_ASSUME(!mutex.acquire(50));
    TSUNIT_ASSERT(ts::Time::CurrentUTC() >= dueTime1);
    TSUNIT_ASSUME(ts::Time::CurrentUTC() < dueTime2);
    TSUNIT_ASSERT(mutex.acquire(1000));
    TSUNIT_ASSERT(ts::Time::CurrentUTC() >= dueTime2);
    TSUNIT_ASSERT(mutex.release());

    debug() << "ThreadTest::testMutexTimeout: type name: \"" << thread.getTypeName() << "\"" << std::endl;
}

//
// Test case: Use mutex and condition
//
namespace {
    class TestThreadCondition: public utest::TSUnitThread
    {
    private:
        ts::Mutex& _mutex;
        ts::Condition& _condition;
        int& _data;
    public:
        TestThreadCondition(ts::Mutex& mutex, ts::Condition& condition, int& data) :
            utest::TSUnitThread(),
            _mutex(mutex),
            _condition(condition),
            _data(data)
        {
        }
        virtual ~TestThreadCondition() override
        {
            waitForTermination();
        }
        // Main: decrement data and signal condition every 100 ms
        virtual void test() override
        {
            {
                ts::GuardMutex lock(_mutex);
                TSUNIT_ASSERT(_data == 4);
            }
            while (_data > 0) {
                ts::SleepThread(100);
                {
                    ts::GuardCondition lock(_mutex, _condition);
                    _data--;
                    lock.signal();
                }
            }
        }
    };
}

void ThreadTest::testCondition()
{
    int data = 4;
    int previous = data;
    ts::Mutex mutex;
    ts::Condition condition;
    {
        TestThreadCondition thread(mutex, condition, data);
        TSUNIT_ASSERT(thread.start());
        debug() << "ThreadTest::testCondition: type name: \"" << thread.getTypeName() << "\"" << std::endl;

        ts::GuardCondition lock(mutex, condition);
        ts::Time dueTime(ts::Time::CurrentUTC() + 100 - _msPrecision);
        while (data > 0) {
            // Wait until data is decremented (timeout: 10 seconds)
            while (data == previous) {
                TSUNIT_ASSERT(lock.waitCondition(10000));
            }
            TSUNIT_ASSERT(ts::Time::CurrentUTC() >= dueTime);
            dueTime += 100;
            TSUNIT_ASSERT(data == previous - 1);
            previous = data;
        }
    }
}
