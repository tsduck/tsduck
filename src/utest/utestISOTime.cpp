//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ISOTime
//
//----------------------------------------------------------------------------

#include "tsISOTime.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ISOTimeTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ToISO);
    TSUNIT_DECLARE_TEST(TimeFromISO);
    TSUNIT_DECLARE_TEST(TimeFromMPEG7);
    TSUNIT_DECLARE_TEST(DurationFromISO);
    TSUNIT_DECLARE_TEST(FromString);

private:
    static constexpr int64_t ms_per_second = 1'000;
    static constexpr int64_t ms_per_minute = ms_per_second * 60;
    static constexpr int64_t ms_per_hour = ms_per_minute * 60;
    static constexpr int64_t ms_per_day = ms_per_hour * 24;
    static constexpr int64_t ms_per_week = ms_per_day * 7;
    static constexpr int64_t ms_per_month = ms_per_day * 30;  // ISO 8601 : 30 days per month
    static constexpr int64_t ms_per_year = ms_per_day * 365;
};

TSUNIT_REGISTER(ISOTimeTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------


TSUNIT_DEFINE_TEST(ToISO)
{
    TSUNIT_EQUAL(u"2025-11-03T12:46:57.845Z", ts::ISOTime::ToISO(ts::Time(2025, 11, 3, 12, 46, 57, 845)));
    TSUNIT_EQUAL(u"2025-11-03T12:46:57Z", ts::ISOTime::ToISO(ts::Time(2025, 11, 3, 12, 46, 57)));
    TSUNIT_EQUAL(u"2025-11-03T12:46:57.845Z", ts::ISOTime::ToISO(ts::Time(2025, 11, 3, 12, 46, 57, 845), cn::seconds(0)));
    TSUNIT_EQUAL(u"2025-11-03T12:46:57.845+02:04", ts::ISOTime::ToISO(ts::Time(2025, 11, 3, 12, 46, 57, 845), cn::seconds(7445)));
    TSUNIT_EQUAL(u"2025-11-03T12:46:57.845-02:04", ts::ISOTime::ToISO(ts::Time(2025, 11, 3, 12, 46, 57, 845), cn::seconds(-7445)));
}

TSUNIT_DEFINE_TEST(TimeFromISO)
{
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", ts::ISOTime::TimeFromISO(u"2007-03-01T13:45:56Z").toString());
    TSUNIT_EQUAL(u"2034/07/28 13:45:56.500", ts::ISOTime::TimeFromISO(u"20340728T134556.5Z").toString());
    TSUNIT_EQUAL(u"1998/03/04 00:00:00.000", ts::ISOTime::TimeFromISO(u"1998 3 4").toString());
    TSUNIT_EQUAL(u"2007/03/01 12:43:56.000", ts::ISOTime::TimeFromISO(u"2007-03-01T13:45:56-01:02").toString());
    TSUNIT_EQUAL(u"2025/01/10 11:36:58.000", ts::ISOTime::TimeFromISO(u"2025-01-10T11:36:58Z").toString());
}

TSUNIT_DEFINE_TEST(TimeFromMPEG7)
{
    // Analyze a mpeg7:timePointType string.
    TSUNIT_EQUAL(u"2026/09/04 07:13:35.000", ts::ISOTime::TimeFromISO(u"2026-09-04T07:13:35:000F1000").toString());
    TSUNIT_EQUAL(u"2026/09/04 07:13:35.987", ts::ISOTime::TimeFromISO(u"2026-09-04T07:13:35:987F1000").toString());
    TSUNIT_EQUAL(u"2026/09/04 07:13:35.050", ts::ISOTime::TimeFromISO(u"2026-09-04T07:13:35:5F100").toString());
}

TSUNIT_DEFINE_TEST(DurationFromISO)
{
    TSUNIT_EQUAL(-1, ts::ISOTime::DurationFromISO(u"").count());
    TSUNIT_EQUAL(-1, ts::ISOTime::DurationFromISO(u"1S").count());
    TSUNIT_EQUAL(1'000, ts::ISOTime::DurationFromISO(u"P1S").count());
    TSUNIT_EQUAL(1'567, ts::ISOTime::DurationFromISO(u"P1.567S").count());
    TSUNIT_EQUAL(1'567, ts::ISOTime::DurationFromISO(u"P1,567S").count());
    TSUNIT_EQUAL(1 * ms_per_year +
                 2 * ms_per_month +
                 15 * ms_per_day +
                 12 * ms_per_hour +
                 30 * ms_per_minute,
                 ts::ISOTime::DurationFromISO(u"P1Y2M15DT12H30M0S").count());
    TSUNIT_EQUAL(2 * ms_per_year +
                 2 * ms_per_week + ms_per_week / 2,
                 ts::ISOTime::DurationFromISO(u"P2Y2,5W").count());
}

TSUNIT_DEFINE_TEST(FromString)
{
    ts::ISOTime t(u"2007-03-01T13:45:56Z");
    TSUNIT_ASSERT(t.isValid());
    TSUNIT_ASSERT(t.isSingleTime());
    TSUNIT_ASSERT(!t.isInterval());
    TSUNIT_ASSERT(!t.isRecurring());
    TSUNIT_ASSERT(!t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::TIME, t.type());
    TSUNIT_EQUAL(u"2007-03-01T13:45:56Z", t.toString());
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", ts::Time(t).toString());
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", t.start().toString());
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", t.end().toString());
    TSUNIT_EQUAL(0, t.duration().count());
    TSUNIT_EQUAL(0, t.recurrences());

    t.fromString(u"abc");
    TSUNIT_ASSERT(!t.isValid());
    TSUNIT_ASSERT(!t.isSingleTime());
    TSUNIT_ASSERT(!t.isInterval());
    TSUNIT_ASSERT(!t.isRecurring());
    TSUNIT_ASSERT(!t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::NONE, t.type());
    TSUNIT_EQUAL(u"", t.toString());
    TSUNIT_ASSERT(ts::Time::Epoch == ts::Time(t));
    TSUNIT_ASSERT(ts::Time::Epoch == t.start());
    TSUNIT_ASSERT(ts::Time::Epoch == t.end());
    TSUNIT_EQUAL(0, t.duration().count());
    TSUNIT_EQUAL(0, t.recurrences());

    t.fromString(u"2007  03-01T13:45:56Z/20070301T14:45:56Z");
    TSUNIT_ASSERT(t.isValid());
    TSUNIT_ASSERT(!t.isSingleTime());
    TSUNIT_ASSERT(t.isInterval());
    TSUNIT_ASSERT(!t.isRecurring());
    TSUNIT_ASSERT(!t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::START_END, t.type());
    TSUNIT_EQUAL(u"2007-03-01T13:45:56Z/2007-03-01T14:45:56Z", t.toString());
    TSUNIT_EQUAL(u"2007-03-01T13:45:56Z/P0Y0M0DT1H0M0S", t.toString(ts::ISOTime::START_DURATION));
    TSUNIT_EQUAL(u"P0Y0M0DT1H0M0S/2007-03-01T14:45:56Z", t.toString(ts::ISOTime::DURATION_END));
    TSUNIT_EQUAL(u"P0Y0M0DT1H0M0S", t.toString(ts::ISOTime::DURATION));
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", ts::Time(t).toString());
    TSUNIT_EQUAL(u"2007/03/01 13:45:56.000", t.start().toString());
    TSUNIT_EQUAL(u"2007/03/01 14:45:56.000", t.end().toString());
    TSUNIT_EQUAL(3'600'000, t.duration().count());
    TSUNIT_EQUAL(0, t.recurrences());

    t.fromString(u"2008 03-01T13:45:56Z/P1H");
    TSUNIT_ASSERT(t.isValid());
    TSUNIT_ASSERT(!t.isSingleTime());
    TSUNIT_ASSERT(t.isInterval());
    TSUNIT_ASSERT(!t.isRecurring());
    TSUNIT_ASSERT(!t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::START_DURATION, t.type());
    TSUNIT_EQUAL(u"2008-03-01T13:45:56Z/P0Y0M0DT1H0M0S", t.toString());
    TSUNIT_EQUAL(u"2008-03-01T13:45:56Z/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::START_END));
    TSUNIT_EQUAL(u"P0Y0M0DT1H0M0S/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::DURATION_END));
    TSUNIT_EQUAL(u"P0Y0M0DT1H0M0S", t.toString(ts::ISOTime::DURATION));
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", ts::Time(t).toString());
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", t.start().toString());
    TSUNIT_EQUAL(u"2008/03/01 14:45:56.000", t.end().toString());
    TSUNIT_EQUAL(3'600'000, t.duration().count());
    TSUNIT_EQUAL(0, t.recurrences());

    t.fromString(u"R4/2008 03-01T13:45:56Z/P1H");
    TSUNIT_ASSERT(t.isValid());
    TSUNIT_ASSERT(!t.isSingleTime());
    TSUNIT_ASSERT(t.isInterval());
    TSUNIT_ASSERT(t.isRecurring());
    TSUNIT_ASSERT(!t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::START_DURATION | ts::ISOTime::RECURRING, t.type());
    TSUNIT_EQUAL(u"R4/2008-03-01T13:45:56Z/P0Y0M0DT1H0M0S", t.toString());
    TSUNIT_EQUAL(u"R4/2008-03-01T13:45:56Z/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::START_END));
    TSUNIT_EQUAL(u"R4/P0Y0M0DT1H0M0S/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::DURATION_END));
    TSUNIT_EQUAL(u"R4/P0Y0M0DT1H0M0S", t.toString(ts::ISOTime::DURATION));
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", ts::Time(t).toString());
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", t.start().toString());
    TSUNIT_EQUAL(u"2008/03/01 14:45:56.000", t.end().toString());
    TSUNIT_EQUAL(3'600'000, t.duration().count());
    TSUNIT_EQUAL(4, t.recurrences());

    t.fromString(u"R/2008 03-01T13:45:56Z/P1H");
    TSUNIT_ASSERT(t.isValid());
    TSUNIT_ASSERT(!t.isSingleTime());
    TSUNIT_ASSERT(t.isInterval());
    TSUNIT_ASSERT(t.isRecurring());
    TSUNIT_ASSERT(t.isUnbounded());
    TSUNIT_EQUAL(ts::ISOTime::START_DURATION | ts::ISOTime::RECURRING, t.type());
    TSUNIT_EQUAL(u"R/2008-03-01T13:45:56Z/P0Y0M0DT1H0M0S", t.toString());
    TSUNIT_EQUAL(u"R/2008-03-01T13:45:56Z/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::START_END));
    TSUNIT_EQUAL(u"R/P0Y0M0DT1H0M0S/2008-03-01T14:45:56Z", t.toString(ts::ISOTime::DURATION_END));
    TSUNIT_EQUAL(u"R/P0Y0M0DT1H0M0S", t.toString(ts::ISOTime::DURATION));
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", ts::Time(t).toString());
    TSUNIT_EQUAL(u"2008/03/01 13:45:56.000", t.start().toString());
    TSUNIT_EQUAL(u"2008/03/01 14:45:56.000", t.end().toString());
    TSUNIT_EQUAL(3'600'000, t.duration().count());
    TSUNIT_EQUAL(ts::ISOTime::UNBOUNDED_RECURRENCES, t.recurrences());
}
