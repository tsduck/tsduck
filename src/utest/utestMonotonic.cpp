//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for Monotonic class.
//
//----------------------------------------------------------------------------

#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "tsTime.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MonotonicTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testArithmetic();
    void testSysWait();
    void testWait();

    TSUNIT_TEST_BEGIN(MonotonicTest);
    TSUNIT_TEST(testArithmetic);
    TSUNIT_TEST(testSysWait);
    TSUNIT_TEST(testWait);
    TSUNIT_TEST_END();
private:
    ts::NanoSecond  _nsPrecision = 0;
    ts::MilliSecond _msPrecision = 0;
};

TSUNIT_REGISTER(MonotonicTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MonotonicTest::beforeTest()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    debug() << "MonotonicTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, " << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void MonotonicTest::afterTest()
{
}

//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void MonotonicTest::testArithmetic()
{
    ts::Monotonic m1, m2;

    TSUNIT_ASSERT(m1 == m2);
    m1.getSystemTime();
    TSUNIT_ASSERT(m1 != m2);
    m2 = m1;
    TSUNIT_ASSERT(m1 == m2);

    m2 += 100; // nanoseconds
    TSUNIT_ASSERT(m1 < m2);
    TSUNIT_ASSERT(m1 - m2 == -100);

    m2 -= 100; // nanoseconds
    TSUNIT_ASSERT(m1 == m2);
    TSUNIT_ASSERT(m1 - m2 == 0);

    m2 -= 100; // nanoseconds
    TSUNIT_ASSERT(m1 > m2);
    TSUNIT_ASSERT(m1 - m2 == 100);
}

void MonotonicTest::testSysWait()
{
    ts::Monotonic start;
    ts::Monotonic end;

    start.getSystemTime();
    std::this_thread::sleep_for(cn::milliseconds(100));
    end.getSystemTime();

    ts::Monotonic check1(start);
    ts::Monotonic check2(start);

    check1 += 100 * ts::NanoSecPerMilliSec - _nsPrecision;
    check2 += 150 * ts::NanoSecPerMilliSec;

    TSUNIT_ASSERT(end >= check1);
    TSUNIT_ASSUME(end < check2);
}

void MonotonicTest::testWait()
{
    const ts::Time start(ts::Time::CurrentLocalTime());

    ts::Monotonic m;
    m.getSystemTime();
    m += 100 * ts::NanoSecPerMilliSec;
    m.wait();

    const ts::Time end(ts::Time::CurrentLocalTime());

    debug() << "MonotonicTest::testWait: end - start = " << (end - start) << " ms" << std::endl;
    TSUNIT_ASSERT(end >= start + 100 - _msPrecision);
    TSUNIT_ASSUME(end < start + 150);
}
