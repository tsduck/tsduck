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
//  CppUnit test suite for class ts::Time
//
//----------------------------------------------------------------------------

#include "tsTime.h"
#include "utestCppUnitTest.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TimeTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testTime();
    void testFormat();
    void testOperators();
    void testLocalTime();
    void testThisNext();
    void testFields();

    CPPUNIT_TEST_SUITE (TimeTest);
    CPPUNIT_TEST (testTime);
    CPPUNIT_TEST (testFormat);
    CPPUNIT_TEST (testOperators);
    CPPUNIT_TEST (testLocalTime);
    CPPUNIT_TEST (testThisNext);
    CPPUNIT_TEST (testFields);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (TimeTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TimeTest::setUp()
{
}

// Test suite cleanup method.
void TimeTest::tearDown()
{
}

//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void TimeTest::testTime()
{
    ts::Time t1;
    utest::Out() << "TimeTest: Default constructor: " << t1 << std::endl;
    CPPUNIT_ASSERT(t1 == ts::Time::Epoch);
}

void TimeTest::testFormat()
{
    ts::Time t1 (2006, 7, 24, 10, 25, 12, 20);
    CPPUNIT_ASSERT_EQUAL(std::string("2006/07/24 10:25:12.020"), t1.format());
    CPPUNIT_ASSERT_EQUAL(std::string("2006/07/24 10:25:12.020"), t1.format(ts::Time::ALL));
    CPPUNIT_ASSERT_EQUAL(std::string("2006"), t1.format (ts::Time::YEAR));
    CPPUNIT_ASSERT_EQUAL(std::string("07"), t1.format (ts::Time::MONTH));
    CPPUNIT_ASSERT_EQUAL(std::string("24"), t1.format (ts::Time::DAY));
    CPPUNIT_ASSERT_EQUAL(std::string("2006/07/24"), t1.format (ts::Time::DATE));
    CPPUNIT_ASSERT_EQUAL(std::string("10"), t1.format (ts::Time::HOUR));
    CPPUNIT_ASSERT_EQUAL(std::string("25"), t1.format (ts::Time::MINUTE));
    CPPUNIT_ASSERT_EQUAL(std::string("12"), t1.format (ts::Time::SECOND));
    CPPUNIT_ASSERT_EQUAL(std::string("10:25:12"), t1.format (ts::Time::TIME));
    CPPUNIT_ASSERT_EQUAL(std::string("020"), t1.format (ts::Time::MILLISECOND));
    CPPUNIT_ASSERT_EQUAL(std::string("24 10:25"), t1.format (ts::Time::DAY | ts::Time::HOUR | ts::Time::MINUTE));
}

void TimeTest::testOperators()
{
    ts::Time t1 (2006, 7, 24, 10, 25, 12, 900);
    ts::Time t2 (2006, 7, 24, 10, 25, 12, 901);
    ts::Time t3 (t1);
    ts::Time t4;
    t4 = t2;

    CPPUNIT_ASSERT(t1 == t3);
    CPPUNIT_ASSERT(!(t1 != t3));
    CPPUNIT_ASSERT(t2 == t4);
    CPPUNIT_ASSERT(t1 <= t3);
    CPPUNIT_ASSERT(t1 <= t2);
    CPPUNIT_ASSERT(t1 >= t3);
    CPPUNIT_ASSERT(t2 >= t1);
    CPPUNIT_ASSERT(t1 < t2);
    CPPUNIT_ASSERT(t2 > t1);
    CPPUNIT_ASSERT(t1 + 1 == t2);
    CPPUNIT_ASSERT(t2 - 1 == t1);
    CPPUNIT_ASSERT(t2 - t1 == 1);
    CPPUNIT_ASSERT(t1 - t2 == -1);

    t3 += 1;
    CPPUNIT_ASSERT(t3 == t2);

    t3 -= 1;
    CPPUNIT_ASSERT(t3 == t1);
}

void TimeTest::testLocalTime()
{
    const ts::Time t (2012, 8, 24, 10, 25, 12, 100);

    CPPUNIT_ASSERT(t.localToUTC().UTCToLocal() == t);
    CPPUNIT_ASSERT(t.UTCToLocal().localToUTC() == t);

    const ts::Time nowUtc (ts::Time::CurrentUTC());
    const ts::Time nowLocal (ts::Time::CurrentLocalTime());

    utest::Out() << "TimeTest: Current local time: " << nowLocal << std::endl;
    utest::Out() << "TimeTest: Current UTC time: " << nowUtc << std::endl;
    utest::Out() << "TimeTest: Local time offset: " << (nowLocal - nowUtc) / ts::MilliSecPerSec << " seconds" << std::endl;
    utest::Out() << "TimeTest: Julian Epoch offset: " << ts::Time::JulianEpochOffset / ts::MilliSecPerDay << " days" << std::endl;

    CPPUNIT_ASSERT(nowUtc > ts::Time::Epoch);
    CPPUNIT_ASSERT(nowUtc < ts::Time::Apocalypse);
    CPPUNIT_ASSERT(nowLocal > ts::Time::Epoch);
    CPPUNIT_ASSERT(nowLocal < ts::Time::Apocalypse);
    CPPUNIT_ASSERT(std::abs(nowUtc - nowLocal) < ts::MilliSecPerDay);
}

void TimeTest::testThisNext()
{
    const ts::Time t (2012, 8, 24, 10, 25, 12, 100);

    CPPUNIT_ASSERT(t.thisHour()  == ts::Time (2012, 8, 24, 10, 0, 0, 0));
    CPPUNIT_ASSERT(t.nextHour()  == ts::Time (2012, 8, 24, 11, 0, 0, 0));
    CPPUNIT_ASSERT(t.thisDay()   == ts::Time (2012, 8, 24,  0, 0, 0, 0));
    CPPUNIT_ASSERT(t.nextDay()   == ts::Time (2012, 8, 25,  0, 0, 0, 0));
    CPPUNIT_ASSERT(t.thisMonth() == ts::Time (2012, 8,  1,  0, 0, 0, 0));
    CPPUNIT_ASSERT(t.nextMonth() == ts::Time (2012, 9,  1,  0, 0, 0, 0));
    CPPUNIT_ASSERT(t.thisYear()  == ts::Time (2012, 1,  1,  0, 0, 0, 0));
    CPPUNIT_ASSERT(t.nextYear()  == ts::Time (2013, 1,  1,  0, 0, 0, 0));
}

void TimeTest::testFields()
{
    ts::Time::Fields f1 (2012, 8, 24, 10, 25, 12, 100);

    ts::Time::Fields f2;
    f2.year = 2012;
    f2.month = 8;
    f2.day = 24;
    f2.hour = 10;
    f2.minute = 25;
    f2.second = 12;
    f2.millisecond = 100;

    CPPUNIT_ASSERT(f1 == f2);
    CPPUNIT_ASSERT(!(f1 != f2));

    ts::Time t1 (2012, 8, 24, 10, 25, 12, 100);
    ts::Time t2 (f2);
    CPPUNIT_ASSERT(t1 == t2);

    ts::Time::Fields f3 (t1);
    CPPUNIT_ASSERT(f3 == f1);
}
