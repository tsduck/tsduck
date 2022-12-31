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
//  TSUnit test suite for class ts::Time
//
//----------------------------------------------------------------------------

#include "tsTime.h"
#include "tsCASDate.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TimeTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testTime();
    void testFormat();
    void testOperators();
    void testLocalTime();
    void testThisNext();
    void testFields();
    void testFieldsValid();
    void testDecode();
    void testEpoch();
    void testUnixTime();
    void testDaylightSavingTime();
    void testCAS();
    void testJST();
    void testLeapSeconds();

    TSUNIT_TEST_BEGIN(TimeTest);
    TSUNIT_TEST(testTime);
    TSUNIT_TEST(testFormat);
    TSUNIT_TEST(testOperators);
    TSUNIT_TEST(testLocalTime);
    TSUNIT_TEST(testThisNext);
    TSUNIT_TEST(testFields);
    TSUNIT_TEST(testFieldsValid);
    TSUNIT_TEST(testDecode);
    TSUNIT_TEST(testEpoch);
    TSUNIT_TEST(testUnixTime);
    TSUNIT_TEST(testDaylightSavingTime);
    TSUNIT_TEST(testCAS);
    TSUNIT_TEST(testJST);
    TSUNIT_TEST(testLeapSeconds);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(TimeTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TimeTest::beforeTest()
{
}

// Test suite cleanup method.
void TimeTest::afterTest()
{
}

//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void TimeTest::testTime()
{
    ts::Time t1;
    debug() << "TimeTest: Default constructor: " << t1 << std::endl;
    TSUNIT_ASSERT(t1 == ts::Time::Epoch);
}

void TimeTest::testFormat()
{
    ts::Time t1(2006, 7, 24, 10, 25, 12, 20);
    TSUNIT_EQUAL(u"2006/07/24 10:25:12.020", t1.format());
    TSUNIT_EQUAL(u"2006/07/24 10:25:12.020", t1.format(ts::Time::ALL));
    TSUNIT_EQUAL(u"2006", t1.format(ts::Time::YEAR));
    TSUNIT_EQUAL(u"07", t1.format(ts::Time::MONTH));
    TSUNIT_EQUAL(u"24", t1.format(ts::Time::DAY));
    TSUNIT_EQUAL(u"2006/07/24", t1.format(ts::Time::DATE));
    TSUNIT_EQUAL(u"10", t1.format(ts::Time::HOUR));
    TSUNIT_EQUAL(u"25", t1.format(ts::Time::MINUTE));
    TSUNIT_EQUAL(u"12", t1.format(ts::Time::SECOND));
    TSUNIT_EQUAL(u"10:25:12", t1.format(ts::Time::TIME));
    TSUNIT_EQUAL(u"020", t1.format(ts::Time::MILLISECOND));
    TSUNIT_EQUAL(u"24 10:25", t1.format(ts::Time::DAY | ts::Time::HOUR | ts::Time::MINUTE));
}

void TimeTest::testOperators()
{
    ts::Time t1(2006, 7, 24, 10, 25, 12, 900);
    ts::Time t2(2006, 7, 24, 10, 25, 12, 901);
    ts::Time t3(t1);
    ts::Time t4;
    t4 = t2;

    TSUNIT_ASSERT(t1 == t3);
    TSUNIT_ASSERT(!(t1 != t3));
    TSUNIT_ASSERT(t2 == t4);
    TSUNIT_ASSERT(t1 <= t3);
    TSUNIT_ASSERT(t1 <= t2);
    TSUNIT_ASSERT(t1 >= t3);
    TSUNIT_ASSERT(t2 >= t1);
    TSUNIT_ASSERT(t1 < t2);
    TSUNIT_ASSERT(t2 > t1);
    TSUNIT_ASSERT(t1 + 1 == t2);
    TSUNIT_ASSERT(t2 - 1 == t1);
    TSUNIT_ASSERT(t2 - t1 == 1);
    TSUNIT_ASSERT(t1 - t2 == -1);

    t3 += 1;
    TSUNIT_ASSERT(t3 == t2);

    t3 -= 1;
    TSUNIT_ASSERT(t3 == t1);
}

void TimeTest::testLocalTime()
{
    const ts::Time t(2012, 8, 24, 10, 25, 12, 100);

    TSUNIT_ASSERT(t.localToUTC().UTCToLocal() == t);
    TSUNIT_ASSERT(t.UTCToLocal().localToUTC() == t);

    const ts::Time nowUtc(ts::Time::CurrentUTC());
    const ts::Time nowLocal(ts::Time::CurrentLocalTime());

    debug() << "TimeTest: Current local time: " << nowLocal << std::endl;
    debug() << "TimeTest: Current UTC time: " << nowUtc << std::endl;
    debug() << "TimeTest: Local time offset: " << (nowLocal - nowUtc) / ts::MilliSecPerSec << " seconds" << std::endl;
    debug() << "TimeTest: Julian Epoch offset: " << ts::Time::JulianEpochOffset / ts::MilliSecPerDay << " days" << std::endl;

    TSUNIT_ASSERT(nowUtc > ts::Time::Epoch);
    TSUNIT_ASSERT(nowUtc < ts::Time::Apocalypse);
    TSUNIT_ASSERT(nowLocal > ts::Time::Epoch);
    TSUNIT_ASSERT(nowLocal < ts::Time::Apocalypse);
    TSUNIT_ASSERT(std::abs(nowUtc - nowLocal) < ts::MilliSecPerDay);
}

void TimeTest::testThisNext()
{
    const ts::Time t(2012, 8, 24, 10, 25, 12, 100);

    TSUNIT_ASSERT(t.thisHour()  == ts::Time(2012, 8, 24, 10, 0, 0, 0));
    TSUNIT_ASSERT(t.nextHour()  == ts::Time(2012, 8, 24, 11, 0, 0, 0));
    TSUNIT_ASSERT(t.thisDay()   == ts::Time(2012, 8, 24,  0, 0, 0, 0));
    TSUNIT_ASSERT(t.nextDay()   == ts::Time(2012, 8, 25,  0, 0, 0, 0));
    TSUNIT_ASSERT(t.thisMonth() == ts::Time(2012, 8,  1,  0, 0, 0, 0));
    TSUNIT_ASSERT(t.nextMonth() == ts::Time(2012, 9,  1,  0, 0, 0, 0));
    TSUNIT_ASSERT(t.thisYear()  == ts::Time(2012, 1,  1,  0, 0, 0, 0));
    TSUNIT_ASSERT(t.nextYear()  == ts::Time(2013, 1,  1,  0, 0, 0, 0));
}

void TimeTest::testFields()
{
    ts::Time::Fields f1(2012, 8, 24, 10, 25, 12, 100);

    ts::Time::Fields f2;
    f2.year = 2012;
    f2.month = 8;
    f2.day = 24;
    f2.hour = 10;
    f2.minute = 25;
    f2.second = 12;
    f2.millisecond = 100;

    TSUNIT_ASSERT(f1 == f2);
    TSUNIT_ASSERT(!(f1 != f2));

    ts::Time t1(2012, 8, 24, 10, 25, 12, 100);
    ts::Time t2(f2);
    TSUNIT_ASSERT(t1 == t2);

    ts::Time::Fields f3(t1);
    TSUNIT_ASSERT(f3 == f1);
}

void TimeTest::testFieldsValid()
{
    TSUNIT_ASSERT(ts::Time::Fields(2012, 8, 24, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(2017, 2, 28, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2017, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(1996, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(2000, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2100, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(1960, 8, 24, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 13, 24, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 4, 31, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 24, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 66, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 25, 89, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 25, 12, 1100).isValid());
}

void TimeTest::testDecode()
{
    ts::Time t;
    ts::Time::Fields f;

    TSUNIT_ASSERT(t.decode(u" 2017-12-02 17:28:46"));
    f = ts::Time::Fields(t);
    TSUNIT_EQUAL(2017, f.year);
    TSUNIT_EQUAL(12, f.month);
    TSUNIT_EQUAL(2, f.day);
    TSUNIT_EQUAL(17, f.hour);
    TSUNIT_EQUAL(28, f.minute);
    TSUNIT_EQUAL(46, f.second);
    TSUNIT_EQUAL(0, f.millisecond);

    TSUNIT_ASSERT(!t.decode(u" 2017-00-02 17:28:46"));
    TSUNIT_ASSERT(!t.decode(u" 2017-12-40 17:28:46"));
    TSUNIT_ASSERT(!t.decode(u" 2017-12-02 46:28:46"));
    TSUNIT_ASSERT(!t.decode(u" 2017-12-02 17:67:46"));
    TSUNIT_ASSERT(!t.decode(u" 2017-12-02 17:28:345"));
    TSUNIT_ASSERT(!t.decode(u" 2017-12-02 17:28:46", ts::Time::YEAR | ts::Time::MONTH));

    TSUNIT_ASSERT(t.decode(u" 2017 / 12 x 02 ", ts::Time::YEAR | ts::Time::MINUTE | ts::Time::MILLISECOND));
    f = ts::Time::Fields(t);
    TSUNIT_EQUAL(2017, f.year);
    TSUNIT_EQUAL(1, f.month);
    TSUNIT_EQUAL(1, f.day);
    TSUNIT_EQUAL(0, f.hour);
    TSUNIT_EQUAL(12, f.minute);
    TSUNIT_EQUAL(0, f.second);
    TSUNIT_EQUAL(2, f.millisecond);

    // ISO 8601 date is implicitly accepted.
    TSUNIT_ASSERT(t.decode(u"2018-10-03T18:27:47.234Z", ts::Time::ALL));
    f = ts::Time::Fields(t);
    TSUNIT_EQUAL(2018, f.year);
    TSUNIT_EQUAL(10, f.month);
    TSUNIT_EQUAL(3, f.day);
    TSUNIT_EQUAL(18, f.hour);
    TSUNIT_EQUAL(27, f.minute);
    TSUNIT_EQUAL(47, f.second);
    TSUNIT_EQUAL(234, f.millisecond);
}

void TimeTest::testEpoch()
{
    TSUNIT_EQUAL(u"1970/01/01 00:00:00.000", ts::Time::UnixEpoch.format());
}

void TimeTest::testUnixTime()
{
    debug()
        << "TimeTest: UNIX Epoch at " << ((ts::Time::UnixEpoch - ts::Time::Epoch) / ts::MilliSecPerDay) << " days from Epoch" << std::endl
        << "TimeTest: UNIX Epoch: " << ts::Time::UnixEpoch << std::endl
        << "TimeTest: " << ts::Time(2018, 4, 13, 12, 54, 34) << " is "
        << ((ts::Time(2018, 4, 13, 12, 54, 34) - ts::Time::Epoch) / ts::MilliSecPerDay) << " days from Epoch" << std::endl
        << "          and " <<(1523624074 / (24 * 3600)) << " days from UNIX Epoch" << std::endl;

    TSUNIT_ASSERT(ts::Time::UnixTimeToUTC(0) == ts::Time::UnixEpoch);
    TSUNIT_EQUAL(u"2018/04/13 12:54:34.000", ts::Time::UnixTimeToUTC(1523624074).format());
}

void TimeTest::testDaylightSavingTime()
{
    // Daylight saving time boundaries:
    //
    // Non-existent range (UTC) : 2018-03-25 01:00:00 -> 01:59:59
    // Duplicated range (UTC)   : 2017-10-29 00:00:00 -> 00:59:59, then 01:00:00 -> 01:59:59
    //
    // On a system @UTC+1 (+2 in summer):
    //
    // Non-existent range:
    //    $ date --date="2018-03-25 01:59:59"
    //    Sun Mar 25 01:59:59 CET 2018
    //    $ date --date="2018-03-25 02:00:00"
    //    date: invalid date ‘2018-03-25 02:00:00’
    //    $ date --date="2018-03-25 02:59:59"
    //    date: invalid date ‘2018-03-25 02:59:59’
    //    $ date --date="2018-03-25 03:00:00"
    //    Sun Mar 25 03:00:00 CEST 2018
    //
    // Duplicated range:
    //    $ date --date="2017-10-29 00:30:00 UTC"
    //    Sun Oct 29 02:30:00 CEST 2017
    //    $ date --date="2017-10-29 01:30:00 UTC"
    //    Sun Oct 29 02:30:00 CET 2017   <-- same local time
    //
    // At some point, some of these tests used to fail, depending on the system local time.
    // The bug has been fixed but we now try all times in these two critical days.

    TSUNIT_EQUAL(u"2018/03/25 00:00:00.000", ts::Time(2018, 3, 25,  0,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 00:30:00.000", ts::Time(2018, 3, 25,  0, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 01:00:00.000", ts::Time(2018, 3, 25,  1,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 01:30:00.000", ts::Time(2018, 3, 25,  1, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 01:59:59.000", ts::Time(2018, 3, 25,  1, 59, 59).format());
    TSUNIT_EQUAL(u"2018/03/25 02:00:00.000", ts::Time(2018, 3, 25,  2,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 02:30:00.000", ts::Time(2018, 3, 25,  2, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 03:00:00.000", ts::Time(2018, 3, 25,  3,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 03:30:00.000", ts::Time(2018, 3, 25,  3, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 04:00:00.000", ts::Time(2018, 3, 25,  4,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 04:30:00.000", ts::Time(2018, 3, 25,  4, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 05:00:00.000", ts::Time(2018, 3, 25,  5,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 05:30:00.000", ts::Time(2018, 3, 25,  5, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 06:00:00.000", ts::Time(2018, 3, 25,  6,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 06:30:00.000", ts::Time(2018, 3, 25,  6, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 07:00:00.000", ts::Time(2018, 3, 25,  7,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 07:30:00.000", ts::Time(2018, 3, 25,  7, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 08:00:00.000", ts::Time(2018, 3, 25,  8,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 08:30:00.000", ts::Time(2018, 3, 25,  8, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 09:00:00.000", ts::Time(2018, 3, 25,  9,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 09:30:00.000", ts::Time(2018, 3, 25,  9, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 10:00:00.000", ts::Time(2018, 3, 25, 10,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 10:30:00.000", ts::Time(2018, 3, 25, 10, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 11:00:00.000", ts::Time(2018, 3, 25, 11,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 11:30:00.000", ts::Time(2018, 3, 25, 11, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 12:00:00.000", ts::Time(2018, 3, 25, 12,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 12:30:00.000", ts::Time(2018, 3, 25, 12, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 13:00:00.000", ts::Time(2018, 3, 25, 13,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 13:30:00.000", ts::Time(2018, 3, 25, 13, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 14:00:00.000", ts::Time(2018, 3, 25, 14,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 14:30:00.000", ts::Time(2018, 3, 25, 14, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 15:00:00.000", ts::Time(2018, 3, 25, 15,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 15:30:00.000", ts::Time(2018, 3, 25, 15, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 16:00:00.000", ts::Time(2018, 3, 25, 16,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 16:30:00.000", ts::Time(2018, 3, 25, 16, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 17:00:00.000", ts::Time(2018, 3, 25, 17,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 17:30:00.000", ts::Time(2018, 3, 25, 17, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 18:00:00.000", ts::Time(2018, 3, 25, 18,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 18:30:00.000", ts::Time(2018, 3, 25, 18, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 19:00:00.000", ts::Time(2018, 3, 25, 19,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 19:30:00.000", ts::Time(2018, 3, 25, 19, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 20:00:00.000", ts::Time(2018, 3, 25, 20,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 20:30:00.000", ts::Time(2018, 3, 25, 20, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 21:00:00.000", ts::Time(2018, 3, 25, 21,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 21:30:00.000", ts::Time(2018, 3, 25, 21, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 22:00:00.000", ts::Time(2018, 3, 25, 22,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 22:30:00.000", ts::Time(2018, 3, 25, 22, 30,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 23:00:00.000", ts::Time(2018, 3, 25, 23,  0,  0).format());
    TSUNIT_EQUAL(u"2018/03/25 23:30:00.000", ts::Time(2018, 3, 25, 23, 30,  0).format());

    TSUNIT_EQUAL(u"2017/10/29 00:00:00.000", ts::Time(2017, 10, 29,  0,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 00:30:00.000", ts::Time(2017, 10, 29,  0, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 01:00:00.000", ts::Time(2017, 10, 29,  1,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 01:30:00.000", ts::Time(2017, 10, 29,  1, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 01:59:59.000", ts::Time(2017, 10, 29,  1, 59, 59).format());
    TSUNIT_EQUAL(u"2017/10/29 02:00:00.000", ts::Time(2017, 10, 29,  2,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 02:30:00.000", ts::Time(2017, 10, 29,  2, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 03:00:00.000", ts::Time(2017, 10, 29,  3,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 03:30:00.000", ts::Time(2017, 10, 29,  3, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 04:00:00.000", ts::Time(2017, 10, 29,  4,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 04:30:00.000", ts::Time(2017, 10, 29,  4, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 05:00:00.000", ts::Time(2017, 10, 29,  5,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 05:30:00.000", ts::Time(2017, 10, 29,  5, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 06:00:00.000", ts::Time(2017, 10, 29,  6,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 06:30:00.000", ts::Time(2017, 10, 29,  6, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 07:00:00.000", ts::Time(2017, 10, 29,  7,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 07:30:00.000", ts::Time(2017, 10, 29,  7, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 08:00:00.000", ts::Time(2017, 10, 29,  8,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 08:30:00.000", ts::Time(2017, 10, 29,  8, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 09:00:00.000", ts::Time(2017, 10, 29,  9,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 09:30:00.000", ts::Time(2017, 10, 29,  9, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 10:00:00.000", ts::Time(2017, 10, 29, 10,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 10:30:00.000", ts::Time(2017, 10, 29, 10, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 11:00:00.000", ts::Time(2017, 10, 29, 11,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 11:30:00.000", ts::Time(2017, 10, 29, 11, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 12:00:00.000", ts::Time(2017, 10, 29, 12,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 12:30:00.000", ts::Time(2017, 10, 29, 12, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 13:00:00.000", ts::Time(2017, 10, 29, 13,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 13:30:00.000", ts::Time(2017, 10, 29, 13, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 14:00:00.000", ts::Time(2017, 10, 29, 14,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 14:30:00.000", ts::Time(2017, 10, 29, 14, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 15:00:00.000", ts::Time(2017, 10, 29, 15,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 15:30:00.000", ts::Time(2017, 10, 29, 15, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 16:00:00.000", ts::Time(2017, 10, 29, 16,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 16:30:00.000", ts::Time(2017, 10, 29, 16, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 17:00:00.000", ts::Time(2017, 10, 29, 17,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 17:30:00.000", ts::Time(2017, 10, 29, 17, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 18:00:00.000", ts::Time(2017, 10, 29, 18,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 18:30:00.000", ts::Time(2017, 10, 29, 18, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 19:00:00.000", ts::Time(2017, 10, 29, 19,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 19:30:00.000", ts::Time(2017, 10, 29, 19, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 20:00:00.000", ts::Time(2017, 10, 29, 20,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 20:30:00.000", ts::Time(2017, 10, 29, 20, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 21:00:00.000", ts::Time(2017, 10, 29, 21,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 21:30:00.000", ts::Time(2017, 10, 29, 21, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 22:00:00.000", ts::Time(2017, 10, 29, 22,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 22:30:00.000", ts::Time(2017, 10, 29, 22, 30,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 23:00:00.000", ts::Time(2017, 10, 29, 23,  0,  0).format());
    TSUNIT_EQUAL(u"2017/10/29 23:30:00.000", ts::Time(2017, 10, 29, 23, 30,  0).format());
}

void TimeTest::testCAS()
{
    TSUNIT_EQUAL(1980, ts::ViaccessDate::MIN_YEAR);
    TSUNIT_EQUAL(1990, ts::MediaGuardDate::MIN_YEAR);
    TSUNIT_EQUAL(2000, ts::SafeAccessDate::MIN_YEAR);

    TSUNIT_EQUAL(2107, ts::ViaccessDate::MAX_YEAR);
    TSUNIT_EQUAL(2117, ts::MediaGuardDate::MAX_YEAR);
    TSUNIT_EQUAL(2127, ts::SafeAccessDate::MAX_YEAR);

    ts::MediaGuardDate md(1999, 2, 28);
    TSUNIT_ASSERT(md.isValid());
    TSUNIT_EQUAL(1999, md.year());
    TSUNIT_EQUAL(2, md.month());
    TSUNIT_EQUAL(28, md.day());
    TSUNIT_EQUAL(u"1999-02-28", md.toString());

    ts::Time t(md);
    ts::Time::Fields f(t);
    TSUNIT_EQUAL(1999, f.year);
    TSUNIT_EQUAL(2, f.month);
    TSUNIT_EQUAL(28, f.day);

    md.invalidate();
    TSUNIT_ASSERT(!md.isValid());
}

void TimeTest::testJST()
{
    TSUNIT_EQUAL(u"2020/04/30 21:00:00.000", ts::Time(2020, 4, 30,  12,  0,  0).UTCToJST().format());
    TSUNIT_EQUAL(u"2020/04/30 03:00:00.000", ts::Time(2020, 4, 30,  12,  0,  0).JSTToUTC().format());
    TSUNIT_EQUAL(u"2020/05/01 05:00:00.000", ts::Time(2020, 4, 30,  20,  0,  0).UTCToJST().format());
    TSUNIT_EQUAL(u"2020/04/29 19:00:00.000", ts::Time(2020, 4, 30,   4,  0,  0).JSTToUTC().format());
}

void TimeTest::testLeapSeconds()
{
    TSUNIT_EQUAL(0, ts::Time::Epoch.leapSecondsTo(ts::Time::Epoch));
    TSUNIT_EQUAL(0, ts::Time::Epoch.leapSecondsTo(ts::Time(1971, 1, 1, 0, 0, 0)));
    TSUNIT_EQUAL(0, ts::Time(1971, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1972, 1, 1, 0, 0, 0)));
    TSUNIT_EQUAL(11, ts::Time(1971, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1972, 7, 1, 0, 0, 0)));
    TSUNIT_EQUAL(37, ts::Time::Epoch.leapSecondsTo(ts::Time(2021, 1, 1, 0, 0, 0)));
    TSUNIT_EQUAL(18, ts::Time::GPSEpoch.leapSecondsTo(ts::Time(2021, 1, 1, 0, 0, 0)));
    TSUNIT_EQUAL(5, ts::Time(1985, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1992, 7, 1, 0, 0, 0)));

    // Check that system time does NOT include leap seconds.
    const ts::MilliSecond year = 365 * ts::MilliSecPerDay;
    const ts::MilliSecond leap_year = 366 * ts::MilliSecPerDay;
    TSUNIT_EQUAL(year, ts::Time(1987, 1, 1, 0, 0) - ts::Time(1986, 1, 1, 0, 0));
    TSUNIT_EQUAL(year, ts::Time(1988, 1, 1, 0, 0) - ts::Time(1987, 1, 1, 0, 0));
    TSUNIT_EQUAL(leap_year, ts::Time(1989, 1, 1, 0, 0) - ts::Time(1988, 1, 1, 0, 0));
    TSUNIT_EQUAL(year, ts::Time(1990, 1, 1, 0, 0) - ts::Time(1989, 1, 1, 0, 0));
    TSUNIT_EQUAL(year, ts::Time(1991, 1, 1, 0, 0) - ts::Time(1990, 1, 1, 0, 0));
    TSUNIT_EQUAL(year, ts::Time(1992, 1, 1, 0, 0) - ts::Time(1991, 1, 1, 0, 0));
}
