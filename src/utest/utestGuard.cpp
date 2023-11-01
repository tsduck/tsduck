//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::GuardMutex
//
//----------------------------------------------------------------------------

#include "tsGuardMutex.h"
#include "tsSysUtils.h"
#include "tsunit.h"

namespace {
    class MutexTest;
}


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class GuardTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testGuard();
    [[noreturn]] void testAcquireFailed();
    void testReleaseFailed();

    TSUNIT_TEST_BEGIN(GuardTest);
    TSUNIT_TEST(testGuard);
    TSUNIT_TEST_EXCEPTION(testAcquireFailed, ts::TemplateGuardMutex<MutexTest>::GuardMutexError);
    TSUNIT_TEST(testReleaseFailed);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(GuardTest);


//----------------------------------------------------------------------------
// A mutex class which counts acquire & release.
// Can also voluntarily fail on acquire and/or release.
//----------------------------------------------------------------------------

namespace {
    class MutexTest
    {
        TS_NOCOPY(MutexTest);
    private:
        int _count;
        const bool _acquireResult;
        const bool _failResult;
    public:
        // Constructor
        MutexTest(bool acquireResult = true, bool failResult = true) :
            _count(0),
            _acquireResult(acquireResult),
            _failResult(failResult)
        {
        }

        // Get the count
        int count() const
        {
            return _count;
        }

        // Acquire the mutex. Block until granted.
        // Return true on success and false on error.
        bool acquire(ts::MilliSecond timeout = ts::Infinite)
        {
            _count++;
            return _acquireResult;
        }

        // Release the mutex.
        // Return true on success and false on error.
        bool release()
        {
            _count--;
            return _failResult;
        }
    };
}


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void GuardTest::beforeTest()
{
}

// Test suite cleanup method.
void GuardTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: basic locking
void GuardTest::testGuard()
{
    MutexTest mutex;
    TSUNIT_ASSERT(mutex.count() == 0);
    {
        ts::TemplateGuardMutex<MutexTest> gard1(mutex);
        TSUNIT_ASSERT(mutex.count() == 1);
        {
            ts::TemplateGuardMutex<MutexTest> gard2(mutex);
            TSUNIT_ASSERT(mutex.count() == 2);
        }
        TSUNIT_ASSERT(mutex.count() == 1);
    }
    TSUNIT_ASSERT(mutex.count() == 0);
}

// Test case: acquire() error is properly handled.
void GuardTest::testAcquireFailed()
{
    MutexTest mutex(false, true);
    TSUNIT_ASSERT(mutex.count() == 0);
    {
        ts::TemplateGuardMutex<MutexTest> gard(mutex);
        TSUNIT_FAIL("mutex.acquire() passed, should not get there");
    }
}

// Test case: release() error is properly handled.
void GuardTest::testReleaseFailed()
{
    if (ts::EnvironmentExists(u"UTEST_FATAL_CRASH_ALLOWED")) {
        std::cerr << "FatalTest: GuardMutex destructor should fail !" << std::endl
                  << "Unset UTEST_FATAL_CRASH_ALLOWED to skip the crash test" << std::endl;
        MutexTest mutex(true, false);
        TSUNIT_ASSERT(mutex.count() == 0);
        {
            ts::TemplateGuardMutex<MutexTest> gard(mutex);
            TSUNIT_ASSERT(mutex.count() == 1);
        }
        TSUNIT_FAIL("mutex.release() passed, should not get there");
    }
    else {
        debug() << "FatalTest: crash test for failing GuardMutex destructor skipped, define UTEST_FATAL_CRASH_ALLOWED to force it" << std::endl;
    }
}
