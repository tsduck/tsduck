//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Mutex
//
//  Note that is is difficult to test mutexes without threads. This test suite
//  is partial. More mutex tests are available in test suite ThreadTest.
//
//----------------------------------------------------------------------------

#include "tsMutex.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MutexTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRecursion();

    TSUNIT_TEST_BEGIN(MutexTest);
    TSUNIT_TEST(testRecursion);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(MutexTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MutexTest::beforeTest()
{
}

// Test suite cleanup method.
void MutexTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void MutexTest::testRecursion()
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
