//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    (134774 * TICKS_PER_DAY);
#elif defined(TS_UNIX)
    (0);
#else
    #error "unsupported operating system"
#endif

// This constant is: Julian epoch - Time epoch.
// The Julian epoch is 17 Nov 1858 00:00:00.
// If negative, the Julian epoch cannot be represented as a Time.
const cn::milliseconds ts::Time::JulianEpochOffset
#if defined(TS_WINDOWS)
    // Windows epoch is 1 Jan 1601 00:00:00, 94187 days before Julian epoch.
    (94187 * MS_PER_DAY);
#elif defined(TS_UNIX)
    // UNIX epoch is 1 Jan 1970 00:00:00, 40587 days after Julian epoch
    (-40587 * MS_PER_DAY);
#else
    #error "unsupported operating system"
#endif

// The GPS Epoch.
const ts::Time ts::Time::GPSEpoch(1980, 1, 6, 0, 0);


//----------------------------------------------------------------------------
// Constructors and time set.
//----------------------------------------------------------------------------

ts::Time::Time(int year, int month, int day, int hour, int minute, int second, int millisecond) :
    _value(ToInt64(year, month, day, hour, minute, second, millisecond))
{
}

void ts::Time::set(int year, int month, int day, int hour, int minute, int second, int millisecond)
{
    _value = ToInt64(year, month, day, hour, minute, second, millisecond);
}

ts::Time::Time(const ts::Time::Fields& f) :
    _value(ToInt64(f.year, f.month, f.day, f.hour, f.minute, f.second, f.millisecond))
{
}

void ts::Time::set(const ts::Time::Fields& f)
{
    _value = ToInt64(f.year, f.month, f.day, f.hour, f.minute, f.second, f.millisecond);
}

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
    // Number of days per month. Non-leap years are separately checked.
    static const int dpm[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    return year >= EPOCH_YEAR &&
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
        s.append(UString::Format(u"%4d", f.year));
    }
    if ((fields & MONTH) != 0) {
        if ((fields & YEAR) != 0) {
            s.push_back(u'/');
        }
        s.append(UString::Format(u"%02d", f.month));
    }
    if ((fields & DAY) != 0) {
        if ((fields & (YEAR | MONTH)) != 0) {
            s.push_back(u'/');
        }
        s.append(UString::Format(u"%02d", f.day));
    }
    if ((fields & (YEAR | MONTH | DAY)) != 0 && (fields & (HOUR | MINUTE | SECOND | MILLISECOND)) != 0) {
        s.push_back(u' ');
    }
    if ((fields & HOUR) != 0) {
        s.append(UString::Format(u"%02d", f.hour));
    }
    if ((fields & MINUTE) != 0) {
        if ((fields & HOUR) != 0) {
            s.push_back(u':');
        }
        s.append(UString::Format(u"%02d", f.minute));
    }
    if ((fields & SECOND) != 0) {
        if ((fields & (HOUR | MINUTE)) != 0) {
            s.push_back(u':');
        }
        s.append(UString::Format(u"%02d", f.second));
    }
    if ((fields & MILLISECOND) != 0) {
        if ((fields & (HOUR | MINUTE | SECOND)) != 0) {
            s.push_back(u'.');
        }
        s.append(UString::Format(u"%03d", f.millisecond));
    }
    return s;
}


//----------------------------------------------------------------------------
// Normalize a string. All non-digit characters are converted to space.
// Space sequences are reduced.
//----------------------------------------------------------------------------

namespace {
    void ReduceSpaces(ts::UString& s)
    {
        for (size_t i = 0; i < s.size(); ++i) {
            if (!ts::IsDigit(s[i])) {
                s[i] = u' ';
            }
        }
        s.trim(true, true, true);
    }
}


//----------------------------------------------------------------------------
// Decode a time from a string.
//----------------------------------------------------------------------------

bool ts::Time::decode(const ts::UString& str, int fields)
{
    // Replace all non-digit character by spaces.
    UString s(str);
    ReduceSpaces(s);

    // Decode up to 7 integer fields.
    int f[7];
    size_t count = 0;
    size_t end = 0;
    s.scan(count, end, u"%d %d %d %d %d %d %d", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6]);

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
        set(t);
        return true;
    }
    catch (...) {
        clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Find an integer in a string at given 'pos' (update pos).
// - Skip non digits.
// - Stop on any character from 'delimiters'.
// - Consume at most 'char_count' digits and return the result in 'value'.
// - Return true if ok, false if no value found.
// - Also update the 'width' of the integer field.
//----------------------------------------------------------------------------

namespace {
    bool IntFromString(const ts::UString& str, size_t& pos, const ts::UString& delimiters, size_t char_count, int& value, size_t& width)
    {
        while (pos < str.size() && !ts::IsDigit(str[pos])) {
            if (delimiters.contains(str[pos])) {
                return false;  // reached end of allowed string
            }
            pos++;
        }
        const size_t start = pos;
        while (pos < str.size() && (pos - start) < char_count && ts::IsDigit(str[pos])) {
            pos++;
        }
        width = pos - start;
        return str.substr(start, width).toInteger(value);
    }
}


//----------------------------------------------------------------------------
// Decode a time from an ISO 8601 representation.
//----------------------------------------------------------------------------

bool ts::Time::fromISO(const UString& str)
{
    // Elements to collect.
    Fields fields;

    // Collect date fields.
    static const UString end_date(u"tT");
    size_t count = 0;
    size_t pos = 0;
    size_t width = 0;
    if (IntFromString(str, pos, end_date, 4, fields.year, width)) {
        count++;
        if (IntFromString(str, pos, end_date, 2, fields.month, width)) {
            count++;
            if (IntFromString(str, pos, end_date, 2, fields.day, width)) {
                count++;
            }
        }
    }

    // Fill missing date fields with current UTC time.
    if (count < 3) {
        // Fetch current UTC *only* if some fields are missing.
        const Fields now(CurrentUTC());
        fields.day = now.day;
        if (count < 2) {
            fields.month = now.month;
            if (count < 1) {
                fields.year = now.year;
            }
        }
    }

    // Move after the time delimiter.
    pos = std::min(str.find_first_of(end_date, pos), str.size()) + 1;

    // Analyze the time part.
    cn::minutes utc_offset = cn::minutes::zero();
    if (pos < str.length()) {
        static const UString end_time(u".,+-" + UString::Range('A', 'Z') + UString::Range('a', 'z'));
        if (IntFromString(str, pos, end_time, 2, fields.hour, width) && IntFromString(str, pos, end_time, 2, fields.minute, width)) {
            IntFromString(str, pos, end_time, 2, fields.second, width);
        }
        if (pos < str.length() && (str[pos] == '.' || str[pos] == ',')) {
            pos++;
            if (IntFromString(str, pos, end_time, 3, fields.millisecond, width)) {
                while (width++ < 3) {
                    fields.millisecond *= 10;
                }
            }
        }
        if (pos < str.length() && (str[pos] == '+' || str[pos] == '-')) {
            int sign = str[pos++] == '-' ? -1 : 1;
            int off_hour = 0;
            int off_minute = 0;
            IntFromString(str, pos, end_time, 2, off_hour, width);
            IntFromString(str, pos, end_time, 2, off_minute, width);
            utc_offset = cn::minutes(sign * ((60 * off_hour) + off_minute));
        }
    }

    // Check that all provided fields are correct.
    if (!fields.isValid()) {
        return false;
    }

    // Build the time value.
    try {
        set(fields);
        *this += utc_offset;
        return true;
    }
    catch (...) {
        clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Format the time in ISO 8601 representation, including offset from UTC time.
//----------------------------------------------------------------------------

ts::UString ts::Time::toIsoWithMinutes(intmax_t utc_offset) const
{
    const Fields f(*this);
    UString s;
    s.format(u"%04d-%02d-%02dT%02d:%02d:%02d", f.year, f.month, f.day, f.hour, f.minute, f.second);
    if (f.millisecond > 0) {
        s.format(u".%03d", f.millisecond);
    }
    if (utc_offset == 0) {
        s += u'Z';
    }
    else {
        s += utc_offset < 0 ? u'-' : u'+';
        utc_offset = std::abs(utc_offset) % (24 * 60);
        if (utc_offset % 60 == 0) {
            s.format(u"%02d", utc_offset / 60);
        }
        else {
            s.format(u"%02d:%02d", utc_offset / 60, utc_offset % 60);
        }
    }
    return s;
}


//----------------------------------------------------------------------------
// Get the number of leap seconds between two UTC dates.
//----------------------------------------------------------------------------

cn::seconds ts::Time::leapSecondsTo(const Time& end) const
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
        return *this - JSTOffset;
    }
}

ts::Time ts::Time::UTCToJST() const
{
    if (_value == Epoch._value || _value == Apocalypse._value) {
        return *this;
    }
    else {
        return *this + JSTOffset;
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
// Windows time conversions
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

// Converts a Win32 FILETIME to cn::milliseconds.
cn::milliseconds ts::Time::Win32FileTimeToMilliSecond(const ::FILETIME& ft)
{
    FileTime ftime;
    ftime.ft = ft;
    return cn::duration_cast<cn::milliseconds>(Ticks(ftime.i));
}

// Converts a Win32 FILETIME to a UTC time.
ts::Time ts::Time::Win32FileTimeToUTC(const ::FILETIME& ft)
{
    FileTime ftime;
    ftime.ft = ft;
    return Time(ftime.i);
}

// Convert the time to a Win32 @c FILETIME.
void ts::Time::toWin32FileTime(::FILETIME& ft) const
{
    FileTime ftime;
    ftime.i = _value;
    ft = ftime.ft;
}
#endif


//----------------------------------------------------------------------------
// Converts with UNIX time_t and GPS time
//----------------------------------------------------------------------------

ts::Time ts::Time::UnixTimeToUTC(const uint64_t t)
{
    // The value t is a number of seconds since Jan 1st 1970.
    return UnixEpoch + cn::seconds(cn::seconds::rep(t));
}

uint64_t ts::Time::toUnixTime() const
{
    return std::max(cn::seconds::zero(), cn::duration_cast<cn::seconds>(*this - UnixEpoch)).count();
}

cn::seconds ts::Time::toGPSSeconds() const
{
    return std::max(cn::seconds::zero(), cn::duration_cast<cn::seconds>(*this - GPSEpoch));
}


//----------------------------------------------------------------------------
// These static routines get the current real time clock and add a delay.
//----------------------------------------------------------------------------

#if defined(TS_UNIX)

cn::nanoseconds ts::Time::UnixClockNanoSeconds(clockid_t clock, const cn::milliseconds& delay)
{
    // Get current time using the specified clock.
    // Minimum resolution is a nanosecond, but much more in fact.
    ::timespec result;
    if (::clock_gettime(clock, &result) != 0) {
        throw TimeError(u"clock_gettime error", errno);
    }

    // Current time in nano-seconds:
    const cn::nanoseconds nanoseconds = cn::nanoseconds(cn::nanoseconds::rep(result.tv_nsec) + cn::nanoseconds::rep(result.tv_sec) * std::nano::den);

    // Last possible integer time value:
    constexpr cn::nanoseconds MAX = cn::nanoseconds::max();

    // Delay in nano-seconds:
    const cn::nanoseconds nsDelay = cn::duration_cast<cn::nanoseconds>(delay);

    // Current time + delay in nano-seconds:
    return (nanoseconds < MAX - nsDelay) ? nanoseconds + nsDelay : MAX;
}

void ts::Time::GetUnixClock(::timespec& result, clockid_t clock, const cn::milliseconds& delay)
{
    const cn::nanoseconds nanoseconds = UnixClockNanoSeconds(clock, delay);
    result.tv_nsec = long(nanoseconds.count() % std::nano::den);
    result.tv_sec = time_t(nanoseconds.count() / std::nano::den);
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
        throw TimeError(UString::Format(u"mktime_z error (%d, %d, %d, %d, %d, %d, %d)", year, month, day, hour, minute, second, millisecond));
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
        throw TimeError(UString::Format(u"mktime error (%d, %d, %d, %d, %d, %d, %d)", year, month, day, hour, minute, second, millisecond));
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
    seconds += int64_t(hour - stime.tm_hour) * 3600;

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
