//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  CppUnit test suite for Monotonic class.
//
//----------------------------------------------------------------------------

#include "tsMonotonic.h"
#include "tsSysUtils.h"
#include "tsDecimal.h"
#include "tsTime.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MonotonicTest: public CppUnit::TestFixture
{
public:
    MonotonicTest();
    void setUp();
    void tearDown();
    void testArithmetic();
    void testSysWait();
    void testWait();

    CPPUNIT_TEST_SUITE(MonotonicTest);
    CPPUNIT_TEST(testArithmetic);
    CPPUNIT_TEST(testSysWait);
    CPPUNIT_TEST(testWait);
    CPPUNIT_TEST_SUITE_END();
private:
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MonotonicTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
MonotonicTest::MonotonicTest() :
    _nsPrecision(0),
    _msPrecision(0)
{
}

// Test suite initialization method.
void MonotonicTest::setUp()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    utest::Out() << "MonotonicTest: timer precision = " << ts::Decimal(_nsPrecision) << " ns, " << ts::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void MonotonicTest::tearDown()
{
}

//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void MonotonicTest::testArithmetic()
{
    ts::Monotonic m1, m2;

    CPPUNIT_ASSERT(m1 == m2);
    m1.getSystemTime();
    CPPUNIT_ASSERT(m1 != m2);
    m2 = m1;
    CPPUNIT_ASSERT(m1 == m2);

    m2 += 100; // nanoseconds
    CPPUNIT_ASSERT(m1 < m2);
    CPPUNIT_ASSERT(m1 - m2 == -100);

    m2 -= 100; // nanoseconds
    CPPUNIT_ASSERT(m1 == m2);
    CPPUNIT_ASSERT(m1 - m2 == 0);

    m2 -= 100; // nanoseconds
    CPPUNIT_ASSERT(m1 > m2);
    CPPUNIT_ASSERT(m1 - m2 == 100);
}

void MonotonicTest::testSysWait()
{
    ts::Monotonic start;
    ts::Monotonic end;

    start.getSystemTime();
    ts::SleepThread(100); // milliseconds
    end.getSystemTime();

    ts::Monotonic check1(start);
    ts::Monotonic check2(start);

    check1 += 100 * ts::NanoSecPerMilliSec - _nsPrecision;
    check2 += 150 * ts::NanoSecPerMilliSec;

    CPPUNIT_ASSERT(end >= check1);
    CPPUNIT_ASSERT(end < check2);
}

void MonotonicTest::testWait()
{
    const ts::Time start(ts::Time::CurrentLocalTime());

    ts::Monotonic m;
    m.getSystemTime();
    m += 100 * ts::NanoSecPerMilliSec;
    m.wait();

    const ts::Time end(ts::Time::CurrentLocalTime());

    CPPUNIT_ASSERT(end >= start + 100 - _msPrecision);
    CPPUNIT_ASSERT(end < start + 130);
}
