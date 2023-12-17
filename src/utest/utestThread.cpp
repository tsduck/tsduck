//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Thread
//
//----------------------------------------------------------------------------

#include "tsThread.h"
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
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAttributes();
    void testTermination();
    void testDeleteWhenTerminated();
    void testMutexTimeout();

    TSUNIT_TEST_BEGIN(ThreadTest);
    TSUNIT_TEST(testAttributes);
    TSUNIT_TEST(testTermination);
    TSUNIT_TEST(testDeleteWhenTerminated);
    TSUNIT_TEST(testMutexTimeout);
    TSUNIT_TEST_END();
private:
    ts::NanoSecond  _nsPrecision = 0;
    ts::MilliSecond _msPrecision = 0;
};

TSUNIT_REGISTER(ThreadTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

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
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
// Test case: Check mutex timeout
//
namespace {
    class TestThreadMutexTimeout: public utest::TSUnitThread
    {
    private:
        std::timed_mutex& _mutex;
        std::mutex& _mutexSig;
        std::condition_variable& _condSig;
        bool _gotMutex = false;
    public:
        TestThreadMutexTimeout(std::timed_mutex& mutex, std::mutex& mutexSig, std::condition_variable& condSig) :
            utest::TSUnitThread(),
            _mutex(mutex),
            _mutexSig(mutexSig),
            _condSig(condSig)
        {
        }
        bool gotMutex() const
        {
            return _gotMutex;
        }
        virtual ~TestThreadMutexTimeout() override
        {
            waitForTermination();
        }
        virtual void test() override
        {
            // Acquire the test mutex immediately, should pass.
            TSUNIT_ASSERT(_mutex.try_lock());
            // Signal that we have acquired it.
            {
                std::lock_guard<std::mutex> lock(_mutexSig);
                _gotMutex = true;
                _condSig.notify_one();
            }
            // And sleep 100 ms.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _mutex.unlock();
        }
    };
}

void ThreadTest::testMutexTimeout()
{
    std::timed_mutex mutex;
    std::mutex mutexSig;
    std::condition_variable condSig;
    TestThreadMutexTimeout thread(mutex, mutexSig, condSig);

    // Start thread and wait for it to acquire mutex.
    {
        std::unique_lock<std::mutex> lock(mutexSig);
        TSUNIT_ASSERT(thread.start());
        condSig.wait(lock, [&thread]() { return thread.gotMutex(); });
    }

    // Now, the thread holds the mutex for 100 ms.
    const ts::Time start(ts::Time::CurrentUTC());
    const ts::Time dueTime1(start + 50 - _msPrecision);
    const ts::Time dueTime2(start + 100 - _msPrecision);

    // Use assumptions instead of assertions for time-dependent checks.
    // Timing can be very weird on virtual machines which are used for unitary tests.
    const bool locked = mutex.try_lock_for(std::chrono::milliseconds(50));
    if (locked) {
        mutex.unlock();
    }
    TSUNIT_ASSUME(!locked);
    TSUNIT_ASSERT(ts::Time::CurrentUTC() >= dueTime1);
    TSUNIT_ASSUME(ts::Time::CurrentUTC() < dueTime2);
    TSUNIT_ASSERT(mutex.try_lock_for(std::chrono::milliseconds(1000)));
    TSUNIT_ASSERT(ts::Time::CurrentUTC() >= dueTime2);
    mutex.unlock();

    debug() << "ThreadTest::testMutexTimeout: type name: \"" << thread.getTypeName() << "\"" << std::endl;
}
