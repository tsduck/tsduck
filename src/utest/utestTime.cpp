//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Time
//
//----------------------------------------------------------------------------

#include "tsTime.h"
#include "tsCASDate.h"
#include "tsMJD.h"
#include "tsByteBlock.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TimeTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Time);
    TSUNIT_DECLARE_TEST(Format);
    TSUNIT_DECLARE_TEST(Operators);
    TSUNIT_DECLARE_TEST(LocalTime);
    TSUNIT_DECLARE_TEST(ThisNext);
    TSUNIT_DECLARE_TEST(Fields);
    TSUNIT_DECLARE_TEST(FieldsValid);
    TSUNIT_DECLARE_TEST(Decode);
    TSUNIT_DECLARE_TEST(Epoch);
    TSUNIT_DECLARE_TEST(UnixTime);
    TSUNIT_DECLARE_TEST(DaylightSavingTime);
    TSUNIT_DECLARE_TEST(CAS);
    TSUNIT_DECLARE_TEST(MJD);
    TSUNIT_DECLARE_TEST(JST);
    TSUNIT_DECLARE_TEST(LeapSeconds);
};

TSUNIT_REGISTER(TimeTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Time)
{
    ts::Time t1;
    debug() << "TimeTest: Default constructor: " << t1 << std::endl;
    TSUNIT_ASSERT(t1 == ts::Time::Epoch);
}

TSUNIT_DEFINE_TEST(Format)
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

TSUNIT_DEFINE_TEST(Operators)
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
    TSUNIT_ASSERT(t1 + cn::milliseconds(1) == t2);
    TSUNIT_ASSERT(t2 - cn::milliseconds(1) == t1);
    TSUNIT_ASSERT(t2 - t1 == cn::milliseconds(1));
    TSUNIT_ASSERT(t1 - t2 == cn::milliseconds(-1));

    TSUNIT_ASSERT(t1 + cn::milliseconds(1) == t2);
    TSUNIT_ASSERT(t2 - cn::milliseconds(1) == t1);
    TSUNIT_ASSERT(t1 + cn::microseconds(1000) == t2);
    TSUNIT_ASSERT(t2 - cn::microseconds(1000) == t1);

    t3 += cn::milliseconds(1);
    TSUNIT_ASSERT(t3 == t2);

    t3 -= cn::milliseconds(1);
    TSUNIT_ASSERT(t3 == t1);
}

TSUNIT_DEFINE_TEST(LocalTime)
{
    const ts::Time t(2012, 8, 24, 10, 25, 12, 100);

    TSUNIT_ASSERT(t.localToUTC().UTCToLocal() == t);
    TSUNIT_ASSERT(t.UTCToLocal().localToUTC() == t);

    const ts::Time nowUtc(ts::Time::CurrentUTC());
    const ts::Time nowLocal(ts::Time::CurrentLocalTime());

    debug() << "TimeTest: Current local time: " << nowLocal << std::endl;
    debug() << "TimeTest: Current UTC time: " << nowUtc << std::endl;
    debug() << "TimeTest: Local time offset: " << ts::UString::Chrono(cn::duration_cast<cn::seconds>(nowLocal - nowUtc)) << std::endl;
    debug() << "TimeTest: Julian Epoch offset: " << ts::UString::Chrono(cn::duration_cast<cn::days>(ts::Time::JulianEpochOffset)) << std::endl;

    TSUNIT_ASSERT(nowUtc > ts::Time::Epoch);
    TSUNIT_ASSERT(nowUtc < ts::Time::Apocalypse);
    TSUNIT_ASSERT(nowLocal > ts::Time::Epoch);
    TSUNIT_ASSERT(nowLocal < ts::Time::Apocalypse);
    TSUNIT_ASSERT(cn::abs(nowUtc - nowLocal) < cn::days(1));
}

TSUNIT_DEFINE_TEST(ThisNext)
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

TSUNIT_DEFINE_TEST(Fields)
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

TSUNIT_DEFINE_TEST(FieldsValid)
{
    TSUNIT_ASSERT(ts::Time::Fields(2012, 8, 24, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(2017, 2, 28, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2017, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(1996, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(ts::Time::Fields(2000, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2100, 2, 29, 0, 0, 0, 0).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(ts::Time::Fields(ts::Time::Epoch).year - 1, 1, 1, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 13, 24, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 4, 31, 10, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 24, 25, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 66, 12, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 25, 89, 100).isValid());
    TSUNIT_ASSERT(!ts::Time::Fields(2012, 8, 24, 10, 25, 12, 1100).isValid());
}

TSUNIT_DEFINE_TEST(Decode)
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

TSUNIT_DEFINE_TEST(Epoch)
{
    TSUNIT_EQUAL(u"1970/01/01 00:00:00.000", ts::Time::UnixEpoch.format());

    debug() << "TimeTest: Epoch:            " << ts::Time::Epoch << std::endl;
    debug() << "TimeTest: Epoch + 1ms:      " << (ts::Time::Epoch + cn::milliseconds(1)) << std::endl;
    debug() << "TimeTest: Apocalypse - 1ms: " << (ts::Time::Apocalypse - cn::milliseconds(1)) << std::endl;
    debug() << "TimeTest: Apocalypse:       " << ts::Time::Apocalypse << std::endl;
}

TSUNIT_DEFINE_TEST(UnixTime)
{
    debug()
        << "TimeTest: UNIX Epoch at " << ts::UString::Chrono(cn::duration_cast<cn::days>(ts::Time::UnixEpoch - ts::Time::Epoch)) << " from Epoch" << std::endl
        << "TimeTest: UNIX Epoch: " << ts::Time::UnixEpoch << std::endl
        << "TimeTest: " << ts::Time(2018, 4, 13, 12, 54, 34) << " is "
        << ts::UString::Chrono(cn::duration_cast<cn::days>(ts::Time(2018, 4, 13, 12, 54, 34) - ts::Time::Epoch)) << " from Epoch" << std::endl
        << "          and " <<(1523624074 / (24 * 3600)) << " days from UNIX Epoch" << std::endl;

    TSUNIT_ASSERT(ts::Time::UnixTimeToUTC(0) == ts::Time::UnixEpoch);
    TSUNIT_EQUAL(u"2018/04/13 12:54:34.000", ts::Time::UnixTimeToUTC(1523624074).format());
}

TSUNIT_DEFINE_TEST(DaylightSavingTime)
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

TSUNIT_DEFINE_TEST(CAS)
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

TSUNIT_DEFINE_TEST(MJD)
{
    TSUNIT_EQUAL(5, ts::MJDSize(ts::MJD_FULL));
    TSUNIT_EQUAL(2, ts::MJDSize(ts::MJD_DATE));

    ts::Time time;
    uint8_t mjd[5];
    uint8_t out[6];

    // Standard example from DVB specs.
    ts::PutUInt40(mjd, 0xC079'124527);
    TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_FULL, time));
    TSUNIT_EQUAL(u"1993/10/13 12:45:27.000", time.format());

    TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_DATE, time));
    TSUNIT_EQUAL(u"1993/10/13 00:00:00.000", time.format());

    ::memset(out, 0xA5, sizeof(out));
    TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(1993, 10, 13, 12, 45, 27), out, ts::MJD_FULL));
    TSUNIT_EQUAL(0xC079'124527, ts::GetUInt40(out));
    TSUNIT_EQUAL(0xA5, out[5]); // detect overflow

    ::memset(out, 0xA5, sizeof(out));
    TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(1993, 10, 13, 12, 45, 27), out, ts::MJD_DATE));
    TSUNIT_EQUAL(0xC079, ts::GetUInt16(out));
    TSUNIT_EQUAL(0xA5, out[2]); // detect overflow

    // Limit values after March 2025 trick to extend dates after 2038.
    // UNIX systems cannot represent dates before 1970.
    if (ts::Time::JulianEpochOffset >= cn::milliseconds::zero()) {
        debug() << "TimeTest::MJD: testing lower limit in 1948" << std::endl;

        ts::PutUInt40(mjd, 0x8000'000000);
        TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_FULL, time));
        TSUNIT_EQUAL(u"1948/08/05 00:00:00.000", time.format());

        TSUNIT_ASSERT(!ts::EncodeMJD(ts::Time(1948, 8, 4, 10, 11, 12), out, ts::MJD_FULL));

        ::memset(out, 0xA5, sizeof(out));
        TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(1948, 8, 5, 10, 11, 12), out, ts::MJD_FULL));
        TSUNIT_EQUAL(0x8000'101112, ts::GetUInt40(out));
        TSUNIT_EQUAL(0xA5, out[5]); // detect overflow
    }

    ts::PutUInt40(mjd, 0xFFFF'000000);
    TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_FULL, time));
    TSUNIT_EQUAL(u"2038/04/22 00:00:00.000", time.format());

    ::memset(out, 0xA5, sizeof(out));
    TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(2038, 4, 22, 10, 11, 12), out, ts::MJD_FULL));
    TSUNIT_EQUAL(0xFFFF'101112, ts::GetUInt40(out));
    TSUNIT_EQUAL(0xA5, out[5]); // detect overflow

    ts::PutUInt40(mjd, 0x0000'000000);
    TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_FULL, time));
    TSUNIT_EQUAL(u"2038/04/23 00:00:00.000", time.format());

    ::memset(out, 0xA5, sizeof(out));
    TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(2038, 4, 23, 10, 11, 12), out, ts::MJD_FULL));
    TSUNIT_EQUAL(0x0000'101112, ts::GetUInt40(out));
    TSUNIT_EQUAL(0xA5, out[5]); // detect overflow

    ts::PutUInt40(mjd, 0x7FFF'000000);
    TSUNIT_ASSERT(ts::DecodeMJD(mjd, ts::MJD_FULL, time));
    TSUNIT_EQUAL(u"2128/01/09 00:00:00.000", time.format());

    ::memset(out, 0xA5, sizeof(out));
    TSUNIT_ASSERT(ts::EncodeMJD(ts::Time(2128, 1, 9, 10, 11, 12), out, ts::MJD_FULL));
    TSUNIT_EQUAL(0x7FFF'101112, ts::GetUInt40(out));
    TSUNIT_EQUAL(0xA5, out[5]); // detect overflow

    TSUNIT_ASSERT(!ts::EncodeMJD(ts::Time(2128, 1, 10, 10, 11, 12), out, ts::MJD_FULL));
}

TSUNIT_DEFINE_TEST(JST)
{
    TSUNIT_EQUAL(u"2020/04/30 21:00:00.000", ts::Time(2020, 4, 30,  12,  0,  0).UTCToJST().format());
    TSUNIT_EQUAL(u"2020/04/30 03:00:00.000", ts::Time(2020, 4, 30,  12,  0,  0).JSTToUTC().format());
    TSUNIT_EQUAL(u"2020/05/01 05:00:00.000", ts::Time(2020, 4, 30,  20,  0,  0).UTCToJST().format());
    TSUNIT_EQUAL(u"2020/04/29 19:00:00.000", ts::Time(2020, 4, 30,   4,  0,  0).JSTToUTC().format());
}

TSUNIT_DEFINE_TEST(LeapSeconds)
{
    TSUNIT_EQUAL(0, ts::Time::Epoch.leapSecondsTo(ts::Time::Epoch).count());
    TSUNIT_EQUAL(0, ts::Time::Epoch.leapSecondsTo(ts::Time(1971, 1, 1, 0, 0, 0)).count());
    TSUNIT_EQUAL(0, ts::Time(1971, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1972, 1, 1, 0, 0, 0)).count());
    TSUNIT_EQUAL(11, ts::Time(1971, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1972, 7, 1, 0, 0, 0)).count());
    TSUNIT_EQUAL(37, ts::Time::Epoch.leapSecondsTo(ts::Time(2021, 1, 1, 0, 0, 0)).count());
    TSUNIT_EQUAL(18, ts::Time::GPSEpoch.leapSecondsTo(ts::Time(2021, 1, 1, 0, 0, 0)).count());
    TSUNIT_EQUAL(5, ts::Time(1985, 1, 1, 0, 0, 0).leapSecondsTo(ts::Time(1992, 7, 1, 0, 0, 0)).count());

    // Check that system time does NOT include leap seconds.
    const cn::milliseconds year = cn::days(365);
    const cn::milliseconds leap_year = cn::days(366);
    TSUNIT_ASSERT(year == ts::Time(1987, 1, 1, 0, 0) - ts::Time(1986, 1, 1, 0, 0));
    TSUNIT_ASSERT(year == ts::Time(1988, 1, 1, 0, 0) - ts::Time(1987, 1, 1, 0, 0));
    TSUNIT_ASSERT(leap_year == ts::Time(1989, 1, 1, 0, 0) - ts::Time(1988, 1, 1, 0, 0));
    TSUNIT_ASSERT(year == ts::Time(1990, 1, 1, 0, 0) - ts::Time(1989, 1, 1, 0, 0));
    TSUNIT_ASSERT(year == ts::Time(1991, 1, 1, 0, 0) - ts::Time(1990, 1, 1, 0, 0));
    TSUNIT_ASSERT(year == ts::Time(1992, 1, 1, 0, 0) - ts::Time(1991, 1, 1, 0, 0));
}
