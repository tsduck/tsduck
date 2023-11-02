//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTime.h"
#include "tsMemory.h"
#include "tsTimeConfigurationFile.h"

#if defined(TS_UNIX)
    #include "tsBeforeStandardHeaders.h"
    #include <sys/time.h>
    #include <time.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Epochs
//----------------------------------------------------------------------------

// These constants represent the time epoch and the end of times.
const ts::Time ts::Time::Epoch(0);
const ts::Time ts::Time::Apocalypse(0x7FFFFFFFFFFFFFFF);

// Portable representation of the UNIX epoch.
const ts::Time ts::Time::UnixEpoch
#if defined(TS_WINDOWS)
    // Windows epoch is 1 Jan 1601 00:00:00, 134774 days before UNIX epoch.
    (134774 * MilliSecPerDay * TICKS_PER_MS);
#elif defined(TS_UNIX)
    (0);
#else
    #error "unsupported operating system"
#endif

// This constant is: Julian epoch - Time epoch.
// The Julian epoch is 17 Nov 1858 00:00:00.
// If negative, the Julian epoch cannot be represented as a Time.
const ts::MilliSecond ts::Time::JulianEpochOffset =
#if defined(TS_WINDOWS)
    // Windows epoch is 1 Jan 1601 00:00:00, 94187 days before Julian epoch.
    94187 * MilliSecPerDay;
#elif defined(TS_UNIX)
    // UNIX epoch is 1 Jan 1970 00:00:00, 40587 days after Julian epoch
    -40587 * MilliSecPerDay;
#else
    #error "unsupported operating system"
#endif

// The GPS Epoch.
const ts::Time ts::Time::GPSEpoch(1980, 1, 6, 0, 0);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Time::Time(int year, int month, int day, int hour, int minute, int second, int millisecond) :
    _value(ToInt64(year, month, day, hour, minute, second, millisecond))
{
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Time::Time(const ts::Time::Fields& f) :
    _value(ToInt64(f.year, f.month, f.day, f.hour, f.minute, f.second, f.millisecond))
{
}


//----------------------------------------------------------------------------
// Fields constructor
//----------------------------------------------------------------------------

ts::Time::Fields::Fields(int year_, int month_, int day_, int hour_, int minute_, int second_, int millisecond_) :
    year(year_),
    month(month_),
    day(day_),
    hour(hour_),
    minute(minute_),
    second(second_),
    millisecond(millisecond_)
{
}


//----------------------------------------------------------------------------
// Fields comparison
//----------------------------------------------------------------------------

bool ts::Time::Fields::operator==(const Fields& f) const
{
    return year == f.year && month == f.month && day == f.day &&
           hour == f.hour && minute == f.minute && second == f.second &&
           millisecond == f.millisecond;
}


//----------------------------------------------------------------------------
// Check if a year is a leap year (29 days in February).
//----------------------------------------------------------------------------

bool ts::Time::IsLeapYear(int year)
{
    // https://en.wikipedia.org/wiki/Leap_year : "Every year that is exactly
    // divisible by four is a leap year, except for years that are exactly
    // divisible by 100, but these centurial years are leap years if they are
    // exactly divisible by 400. For example, the years 1700, 1800, and 1900
    // were not leap years, but the years 1600 and 2000 were."

    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}


//----------------------------------------------------------------------------
// Validation of the fields.
//----------------------------------------------------------------------------

bool ts::Time::Fields::isValid() const
{
    // Number of days per month.
    static const int dpm[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // We don't accept pre-UNIX years to make sure it works everywhere.
    return year >= 1970 &&
        month >= 1 && month <= 12 &&
        day >= 1 && day <= dpm[month - 1] &&
        (month != 2 || IsLeapYear(year) || day <= 28) &&
        hour >= 0 && hour <= 23 &&
        minute >= 0 && minute <= 59 &&
        second >= 0 && second <= 59 &&
        millisecond >= 0 && millisecond <= 999;
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::Time::toString() const
{
    return format(ALL);
}


//----------------------------------------------------------------------------
// Basic string representation
//----------------------------------------------------------------------------

ts::UString ts::Time::format(int fields) const
{
    UString s;
    s.reserve(25); // to avoid reallocs
    Fields f(*this);

    if ((fields & YEAR) != 0) {
        s.append(UString::Format(u"%4d", {f.year}));
    }
    if ((fields & MONTH) != 0) {
        if ((fields & YEAR) != 0) {
            s.push_back(u'/');
        }
        s.append(UString::Format(u"%02d", {f.month}));
    }
    if ((fields & DAY) != 0) {
        if ((fields & (YEAR | MONTH)) != 0) {
            s.push_back(u'/');
        }
        s.append(UString::Format(u"%02d", {f.day}));
    }
    if ((fields & (YEAR | MONTH | DAY)) != 0 && (fields & (HOUR | MINUTE | SECOND | MILLISECOND)) != 0) {
        s.push_back(u' ');
    }
    if ((fields & HOUR) != 0) {
        s.append(UString::Format(u"%02d", {f.hour}));
    }
    if ((fields & MINUTE) != 0) {
        if ((fields & HOUR) != 0) {
            s.push_back(u':');
        }
        s.append(UString::Format(u"%02d", {f.minute}));
    }
    if ((fields & SECOND) != 0) {
        if ((fields & (HOUR | MINUTE)) != 0) {
            s.push_back(u':');
        }
        s.append(UString::Format(u"%02d", {f.second}));
    }
    if ((fields & MILLISECOND) != 0) {
        if ((fields & (HOUR | MINUTE | SECOND)) != 0) {
            s.push_back(u'.');
        }
        s.append(UString::Format(u"%03d", {f.millisecond}));
    }
    return s;
}


//----------------------------------------------------------------------------
// Decode a time from a string.
//----------------------------------------------------------------------------

bool ts::Time::decode(const ts::UString& str, int fields)
{
    // Replace all non-digit character by spaces.
    UString s(str);
    for (size_t i = 0; i < s.size(); ++i) {
        if (!IsDigit(s[i])) {
            s[i] = u' ';
        }
    }

    // Trim spaces to normalize the string.
    s.trim(true, true, true);

    // Decode up to 7 integer fields.
    int f[7];
    size_t count = 0;
    size_t end = 0;
    s.scan(count, end, u"%d %d %d %d %d %d %d", {&f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6]});

    // Count how many fields are expected.
    size_t expected = 0;
    for (int i = 0; i < 7; ++i) {
        if ((fields & (1 << i)) != 0) {
            ++expected;
        }
    }

    // The complete string must have been decoded.
    if (expected == 0 || count != expected || end < s.length()) {
        return false;
    }

    // Preset time fields with default values.
    Fields t(0, 1, 1, 0, 0, 0, 0);

    // Distribute fields according to user-supplied flags.
    size_t index = 0;
    if ((fields & YEAR) != 0) {
        t.year = f[index++];
    }
    if ((fields & MONTH) != 0) {
        t.month = f[index++];
    }
    if ((fields & DAY) != 0) {
        t.day = f[index++];
    }
    if ((fields & HOUR) != 0) {
        t.hour = f[index++];
    }
    if ((fields & MINUTE) != 0) {
        t.minute = f[index++];
    }
    if ((fields & SECOND) != 0) {
        t.second = f[index++];
    }
    if ((fields & MILLISECOND) != 0) {
        t.millisecond = f[index++];
    }

    // The default year is this year.
    if (t.year == 0) {
        Fields now(CurrentLocalTime());
        t.year = now.year;
    }

    // Check that all provided fields are correct.
    if (!t.isValid()) {
        return false;
    }

    // Build the time value.
    try {
        *this = Time(t);
    }
    catch (TimeError&) {
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Get the number of leap seconds between two UTC dates.
//----------------------------------------------------------------------------

ts::Second ts::Time::leapSecondsTo(const Time& end) const
{
    return TimeConfigurationFile::Instance().leapSeconds(*this, end);
}


//----------------------------------------------------------------------------
// Convert a local time to an UTC time
//----------------------------------------------------------------------------

ts::Time ts::Time::localToUTC() const
{
    // Don't convert specific values.
    if (_value == Epoch._value || _value == Apocalypse._value) {
        return *this;
    }

#if defined(TS_WINDOWS)

    FileTime local, utc;
    local.i = _value;
    if (::LocalFileTimeToFileTime(&local.ft, &utc.ft) == 0) {
        throw TimeError(::GetLastError());
    }
    return Time(utc.i);

#else

    time_t seconds = time_t(_value / (1000 * TICKS_PER_MS));
    ::tm stime;
    TS_ZERO(stime);
    if (::localtime_r(&seconds, &stime) == nullptr) {
        throw TimeError(u"localtime_r error");
    }

#if defined(__sun)
    const int gmt_offset = ::gmtoffset(stime.tm_isdst);
#elif defined(__hpux) || defined(_AIX)
    const int gmt_offset = ::gmtoffset(seconds);
#else
    const long gmt_offset = stime.tm_gmtoff;
#endif

    return Time(_value - int64_t(gmt_offset) * 1000 * TICKS_PER_MS);

#endif
}

//----------------------------------------------------------------------------
// Convert an UTC time to a local time
//----------------------------------------------------------------------------

ts::Time ts::Time::UTCToLocal() const
{
    // Don't convert specific values.
    if (_value == Epoch._value || _value == Apocalypse._value) {
        return *this;
    }

#if defined(TS_WINDOWS)

    FileTime local, utc;
    utc.i = _value;
    if (::FileTimeToLocalFileTime(&utc.ft, &local.ft) == 0) {
        throw TimeError(::GetLastError());
    }
    return Time(local.i);

#else

    time_t seconds = time_t(_value / (1000 * TICKS_PER_MS));
    ::tm stime;
    TS_ZERO(stime);
    if (::localtime_r(&seconds, &stime) == nullptr) {
        throw TimeError(u"localtime_r error");
    }

#if defined(__sun)
    const int gmt_offset = ::gmtoffset(stime.tm_isdst);
#elif defined(__hpux) || defined(_AIX)
    const int gmt_offset = ::gmtoffset(seconds);
#else
    const long gmt_offset = stime.tm_gmtoff;
#endif

    return Time(_value + int64_t(gmt_offset) * 1000 * TICKS_PER_MS);

#endif
}


//----------------------------------------------------------------------------
// Convert between UTC and JST (Japan Standard Time).
// Don't convert specific values. JST is 9 hours ahead from UTC.
//----------------------------------------------------------------------------

ts::Time ts::Time::JSTToUTC() const
{
    if (_value == Epoch._value || _value == Apocalypse._value) {
        return *this;
    }
    else {
        return Time(_value - JSTOffset * TICKS_PER_MS);
    }
}

ts::Time ts::Time::UTCToJST() const
{
    if (_value == Epoch._value || _value == Apocalypse._value) {
        return *this;
    }
    else {
        return Time(_value + JSTOffset * TICKS_PER_MS);
    }
}


//----------------------------------------------------------------------------
// This static routine returns the current UTC time.
//----------------------------------------------------------------------------

ts::Time ts::Time::CurrentUTC()
{
#if defined(TS_WINDOWS)

    FileTime result;
    ::GetSystemTimeAsFileTime(&result.ft);
    return Time(result.i);

#else

    ::timeval result;
    if (::gettimeofday(&result, nullptr) < 0) {
        throw TimeError(u"gettimeofday error", errno);
    }
    return Time(int64_t(result.tv_usec) + 1000000 * int64_t(result.tv_sec));

#endif
}


//----------------------------------------------------------------------------
// This static routine converts a Win32 FILETIME to MilliSecond
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
ts::MilliSecond ts::Time::Win32FileTimeToMilliSecond(const ::FILETIME& ft)
{
    FileTime ftime;
    ftime.ft = ft;
    return ftime.i / TICKS_PER_MS;
}
#endif


//----------------------------------------------------------------------------
// This static routine converts a Win32 FILETIME to a UTC time
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
ts::Time ts::Time::Win32FileTimeToUTC(const ::FILETIME& ft)
{
    FileTime ftime;
    ftime.ft = ft;
    return Time(ftime.i);
}
#endif


//----------------------------------------------------------------------------
// Converts with UNIX time_t
//----------------------------------------------------------------------------

ts::Time ts::Time::UnixTimeToUTC(const uint64_t t)
{
    // The value t is a number of seconds since Jan 1st 1970.
    return Time(UnixEpoch._value + (Second(t) * 1000 * TICKS_PER_MS));
}

uint64_t ts::Time::toUnixTime() const
{
    return _value < UnixEpoch._value ? 0 : (_value - UnixEpoch._value) / (1000 * TICKS_PER_MS);
}


//----------------------------------------------------------------------------
// Converts with GPS time.
//----------------------------------------------------------------------------

ts::Time ts::Time::GPSSecondsToUTC(Second gps)
{
    // The value t is a number of seconds since Jan 6th 1980.
    return Time(GPSEpoch._value + (gps * 1000 * TICKS_PER_MS));
}

ts::Second ts::Time::toGPSSeconds() const
{
    return _value < GPSEpoch._value ? 0 : (_value - GPSEpoch._value) / (1000 * TICKS_PER_MS);
}


//----------------------------------------------------------------------------
// These static routines get the current real time clock and add a delay.
//----------------------------------------------------------------------------

#if defined(TS_UNIX)

ts::NanoSecond ts::Time::UnixClockNanoSeconds(clockid_t clock, const MilliSecond& delay)
{
    // Get current time using the specified clock.
    // Minimum resolution is a nanosecond, but much more in fact.
    ::timespec result;
    if (::clock_gettime(clock, &result) != 0) {
        throw TimeError(u"clock_gettime error", errno);
    }

    // Current time in nano-seconds:
    const NanoSecond nanoseconds = NanoSecond(result.tv_nsec) + NanoSecond(result.tv_sec) * NanoSecPerSec;

    // Delay in nano-seconds:
    const NanoSecond nsDelay = (delay < Infinite / NanoSecPerMilliSec) ? delay * NanoSecPerMilliSec : Infinite;

    // Current time + delay in nano-seconds:
    return (nanoseconds < Infinite - nsDelay) ? nanoseconds + nsDelay : Infinite;
}

void ts::Time::GetUnixClock(::timespec& result, clockid_t clock, const MilliSecond& delay)
{
    const NanoSecond nanoseconds = UnixClockNanoSeconds(clock, delay);
    result.tv_nsec = long(nanoseconds % NanoSecPerSec);
    result.tv_sec = time_t(nanoseconds / NanoSecPerSec);
}

#endif


//----------------------------------------------------------------------------
// Static private routine: Convert 7 fields to a 64-bit time value.
//----------------------------------------------------------------------------

int64_t ts::Time::ToInt64(int year, int month, int day, int hour, int minute, int second, int millisecond)
{
#if defined(TS_WINDOWS)

    ::SYSTEMTIME stime;
    FileTime ftime;

    stime.wYear = ::WORD(year);
    stime.wMonth = ::WORD(month);
    stime.wDay = ::WORD(day);
    stime.wHour = ::WORD(hour);
    stime.wMinute = ::WORD(minute);
    stime.wSecond = ::WORD(second);
    stime.wMilliseconds = ::WORD(millisecond);

    if (::SystemTimeToFileTime(&stime, &ftime.ft) == 0) {
        throw TimeError(::GetLastError());
    }

    return ftime.i;

#elif defined(TS_NETBSD)

    // On NetBSD, mktime() fails in the daylight saving time switch periods.
    // We use the system-specific mktime_z() which uses UTC (or any specified TZ).
    // Convert using mktime.
    ::tm stime;
    TS_ZERO(stime);
    stime.tm_year = year - 1900;
    stime.tm_mon = month - 1; // 0..11
    stime.tm_mday = day;
    stime.tm_hour = hour;
    stime.tm_min = minute;
    stime.tm_sec = second;
    stime.tm_isdst = -1;

    // Using nullptr as time zone means UTC.
    int64_t seconds = ::mktime_z(nullptr, &stime);

    if (seconds == time_t(-1)) {
        throw TimeError(UString::Format(u"mktime_z error (%d, %d, %d, %d, %d, %d, %d)", {year, month, day, hour, minute, second, millisecond}));
    }

    // Convert to 64-bit time value
    return (seconds * 1000 + int64_t(millisecond)) * TICKS_PER_MS;

#else

    // Convert using mktime.
    ::tm stime;
    TS_ZERO(stime);
    stime.tm_year = year - 1900;
    stime.tm_mon = month - 1; // 0..11
    stime.tm_mday = day;
    stime.tm_hour = hour;
    stime.tm_min = minute;
    stime.tm_sec = second;
    stime.tm_isdst = -1;

    int64_t seconds = ::mktime(&stime);

    if (seconds == time_t(-1)) {
        throw TimeError(UString::Format(u"mktime error (%d, %d, %d, %d, %d, %d, %d)", {year, month, day, hour, minute, second, millisecond}));
    }

    // Add the GMT offset since mktime() uses stime as a local time
#if defined(__sun)
    const int gmt_offset = ::gmtoffset(stime.tm_isdst);
#elif defined(__hpux) || defined(_AIX)
    const int gmt_offset = ::gmtoffset(seconds);
#else
    const long gmt_offset = stime.tm_gmtoff;
#endif
    seconds += gmt_offset;

    // stime is modified on output with actual time.
    // Again, the problem is that mktime() works with local time.
    // In rare cases, at the point of daylight saving time switch (twice a year),
    // the hour is modified because the corresponding local time does not exist
    // (especially in spring where one hour "disappears"). Here, we just want
    // to convert time, regardless of local time considerations. So, we
    // compensate here.
    seconds += (hour - stime.tm_hour) * 3600;

    // Convert to 64-bit time value
    return (seconds * 1000 + int64_t(millisecond)) * TICKS_PER_MS;

#endif
}

//----------------------------------------------------------------------------
// Convert a time into 7 fields
//----------------------------------------------------------------------------

ts::Time::operator Fields() const
{
#if defined(TS_WINDOWS)

    ::SYSTEMTIME st;
    FileTime ft;
    ft.i = _value;
    if (::FileTimeToSystemTime(&ft.ft, &st) == 0) {
        throw TimeError(::GetLastError());
    }
    return Fields(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

#else

    time_t seconds = time_t(_value / (1000 * TICKS_PER_MS));
    ::tm st;
    if (::gmtime_r(&seconds, &st) == nullptr) {
        throw TimeError(u"gmtime_r error");
    }
    return Fields(st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (_value / TICKS_PER_MS) % 1000);

#endif
}


//----------------------------------------------------------------------------
// These methods return the time for the beginning of hour, day, month, year.
//----------------------------------------------------------------------------

ts::Time ts::Time::thisHour() const
{
    Fields f(*this);
    f.minute = f.second = f.millisecond = 0;
    return Time(f);
}

ts::Time ts::Time::thisDay() const
{
    Fields f(*this);
    f.hour = f.minute = f.second = f.millisecond = 0;
    return Time(f);
}

ts::Time ts::Time::thisMonth() const
{
    Fields f(*this);
    f.day = 1;
    f.hour = f.minute = f.second = f.millisecond = 0;
    return Time(f);
}

ts::Time ts::Time::nextMonth() const
{
    Fields f(*this);
    f.day = 1;
    f.hour = f.minute = f.second = f.millisecond = 0;
    if (f.month++ == 12) {
        f.month = 1;
        f.year++;
    }
    return Time(f);
}

ts::Time ts::Time::thisYear() const
{
    Fields f(*this);
    f.month = f.day = 1;
    f.hour = f.minute = f.second = f.millisecond = 0;
    return Time(f);
}

ts::Time ts::Time::nextYear() const
{
    Fields f(*this);
    f.year++;
    f.month = f.day = 1;
    f.hour = f.minute = f.second = f.millisecond = 0;
    return Time(f);
}
