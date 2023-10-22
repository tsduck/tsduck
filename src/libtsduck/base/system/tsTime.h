//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::Time class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsException.h"
#include "tsUString.h"

namespace ts {
    //!
    //! The @c Time class implements a basic representation of time.
    //! @ingroup system
    //!
    //! The implementation is based on the operating system representation
    //! of time. This means that the range of representable time values
    //! may vary from one system to another. You may use the constants
    //! @link Epoch @endlink and @link Apocalypse @endlink as first and
    //! last representable time values.
    //!
    //! An instance of @c Time may be indifferently interpreted as a local time
    //! or UTC time value. Methods are provided to convert between local and
    //! UTC values. The accuracy of these conversions depend on the configuration
    //! of the operating system.
    //!
    //! The implementation of the class is designed to be light and fast so that
    //! @c Time objects may be copied without overhead.
    //! The class is not polymorphic, there is no virtual methods and no vtable.
    //! In fact, the actual representation is only a 64-bit integer.
    //!
    class TSDUCKDLL Time final: public StringifyInterface
    {
    public:
        //!
        //! Fatal low-level time error.
        //!
        TS_DECLARE_EXCEPTION(TimeError);

        //!
        //! Default constructor.
        //! The initial value is the Epoch.
        //!
        Time() = default;

        //!
        //! Constructor from broken-down date fields.
        //! @param [in] year Number of years.
        //! @param [in] month Number of months (1 to 12).
        //! @param [in] day Number of days (1 to 31).
        //! @param [in] hour Number of hours (0 to 23).
        //! @param [in] minute Number of minutes (0 to 59).
        //! @param [in] second Number of seconds (0 to 59).
        //! @param [in] millisecond Number of milliseconds (0 to 999).
        //! @throw ts::Time::TimeError If any field is out of range
        //! or if the resulting time is outside the representable range
        //! for the local operating system.
        //!
        Time(int year, int month, int day, int hour, int minute, int second = 0, int millisecond = 0);

        //!
        //! Clear the time value.
        //! The time value becomes the Epoch.
        //!
        void clear()
        {
            _value = 0;
        }

        //!
        //! Operator Time + MilliSecond => Time.
        //! @param [in] duration A number of milliseconds.
        //! @return A @c Time object representing this object plus the
        //! specified number of milliseconds.
        //!
        Time operator+(const MilliSecond& duration) const
        {
            return Time(_value + duration * TICKS_PER_MS);
        }

        //!
        //! Operator Time - MilliSecond => Time.
        //! @param [in] duration A number of milliseconds.
        //! @return A @c Time object representing this object minus the
        //! specified number of milliseconds.
        //!
        Time operator-(const MilliSecond& duration) const
        {
            return Time(_value - duration * TICKS_PER_MS);
        }

        //!
        //! Operator Time += MilliSecond
        //! @param [in] duration A number of milliseconds to add to this object.
        //! @return A reference to this object.
        //!
        Time& operator+=(const MilliSecond& duration)
        {
            _value += duration * TICKS_PER_MS;
            return *this;
        }

        //!
        //! Operator Time -= MilliSecond
        //! @param [in] duration A number of milliseconds to substract from this object.
        //! @return A reference to this object.
        //!
        Time& operator-=(const MilliSecond& duration)
        {
            _value -= duration * TICKS_PER_MS;
            return *this;
        }

        //!
        //! Operator: Time - Time => MilliSecond.
        //! @param [in] other Another time to substract from this object.
        //! @return The duration, in milliseconds, between this object and the @a other object.
        //!
        MilliSecond operator-(const Time& other) const
        {
            return (_value - other._value) / TICKS_PER_MS;
        }

        //!
        //! Equality operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this object is equal to the @a other object,
        //! @c false otherwise.
        //!
        bool operator==(const Time& other) const { return _value == other._value; }
        TS_UNEQUAL_OPERATOR(Time)

        //!
        //! Lower operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is before the @a other object time,
        //! @c false otherwise.
        //!
        bool operator<(const Time& other) const { return _value < other._value; }

        //!
        //! Lower or equal operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is before or equal to the @a other object time,
        //! @c false otherwise.
        //!
        bool operator<=(const Time& other) const { return _value <= other._value; }

        //!
        //! Greater operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is after the @a other object time,
        //! @c false otherwise.
        //!
        bool operator>(const Time& other) const { return _value > other._value; }

        //!
        //! Greater or equal operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is after or equal to the @a other object time,
        //! @c false otherwise.
        //!
        bool operator>=(const Time& other) const { return _value >= other._value; }

        //!
        //! Broken-down fields of a time value.
        //! This inner class is a simple aggregation of date fields with public members.
        //!
        struct TSDUCKDLL Fields
        {
            int year;        //!< Number of years.
            int month;       //!< Number of months (1 to 12).
            int day;         //!< Number of days (1 to 31).
            int hour;        //!< Number of hours (0 to 23).
            int minute;      //!< Number of minutes (0 to 59).
            int second;      //!< Number of seconds (0 to 59).
            int millisecond; //!< Number of milliseconds (0 to 999).

            //!
            //! Constructor.
            //! There is no verification of the field values.
            //! @param [in] year_ Number of years.
            //! @param [in] month_ Number of months (1 to 12).
            //! @param [in] day_ Number of days (1 to 31).
            //! @param [in] hour_ Number of hours (0 to 23).
            //! @param [in] minute_ Number of minutes (0 to 59).
            //! @param [in] second_ Number of seconds (0 to 59).
            //! @param [in] millisecond_ Number of milliseconds (0 to 999).
            //!
            Fields(int year_ = 0, int month_ = 0, int day_ = 0, int hour_ = 0, int minute_ = 0, int second_ = 0, int millisecond_ = 0);

            //!
            //! Equality operator.
            //! @param [in] other Another @c Fields to compare with this object.
            //! @return @c True is this object is equal to the @a other object,
            //! @c false otherwise.
            //!
            bool operator==(const Fields& other) const;
            TS_UNEQUAL_OPERATOR(Fields)

            //!
            //! Validation of the fields.
            //! @return True if the fields are valid, false otherwise.
            //!
            bool isValid() const;
        };

        //!
        //! Constructor from broken-down date fields in one single object.
        //! Can be used as a conversion operator from @c Time::Fields to @c Time.
        //! @param [in] fields The date fields.
        //! @throw ts::Time::TimeError If any field is out of range
        //! or if the resulting time is outside the representable range
        //! for the local operating system.
        //!
        Time(const Fields& fields);

        //!
        //! Conversion operator from @c Time to @c Time::Fields.
        //! @return A @c Time::Fields object containing the broken-down time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        operator Fields() const;

        //!
        //! Convert a local time to UTC time.
        //! @return A UTC time from this object time, interpreted as a local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time localToUTC() const;

        //!
        //! Convert a UTC time to local time.
        //! @return A local time from this object time, interpreted as a UTC time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time UTCToLocal() const;

        //!
        //! Convert a JST (Japan Standard Time) to UTC time.
        //! @return A UTC time from this object time, interpreted as a JST time.
        //!
        Time JSTToUTC() const;

        //!
        //! Convert a UTC time to JST (Japan Standard Time).
        //! @return A JST time from this object time, interpreted as a UTC time.
        //!
        Time UTCToJST() const;

        //!
        //! Offset of a JST (Japan Standard Time) value from UTC in seconds.
        //! JST is defined as UTC+9.
        //!
        static constexpr MilliSecond JSTOffset = +9 * MilliSecPerHour;

        //!
        //! Flags indicating the list of time fields to display.
        //!
        enum FieldMask {
            YEAR        = 0x01,  //!< Display the year.
            MONTH       = 0x02,  //!< Display the month.
            DAY         = 0x04,  //!< Display the day.
            DATE        = YEAR | MONTH | DAY,  //!< Display the year, month and day.
            HOUR        = 0x08,  //!< Display the hours.
            MINUTE      = 0x10,  //!< Display the minutes.
            SECOND      = 0x20,  //!< Display the seconds.
            TIME        = HOUR | MINUTE | SECOND,  //!< Display the hours, minutes and seconds.
            DATETIME    = DATE | TIME,  //!< Display the year, month, day, hours, minutes and seconds.
            MILLISECOND = 0x40,  //!< Display the milliseconds.
            ALL         = DATE | TIME | MILLISECOND,  //!< Display all fields.
        };

        //!
        //! Format a string representation of a time.
        //! @param [in] fields A combination of option flags indicating which fields
        //! should be included. This is typically the result of or'ed values from the enum
        //! type FieldMask.
        //! @return A string containing the formatted date.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        UString format(int fields = ALL) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //!
        //! Decode a time from a string.
        //! The resulting decoded time is stored in this object.
        //! @param [in] str A string describing a date and time in a rudimentary format.
        //! The string shall contain integer values describing the various fields
        //! in descending order, from year to millisecond. The integer fields can
        //! be separated by any combination of non-digit characters. The list of
        //! expected fields is given by the parameter @a fields.
        //! @param [in] fields A combination of option flags indicating which fields
        //! should be included. This is typically the result of or'ed values from the enum
        //! type FieldMask.
        //! @return True on success, false if the string cannot be decoded.
        //!
        bool decode(const UString& str, int fields = DATE | TIME);

        //!
        //! Get the number of leap seconds between two UTC dates.
        //!
        //! Wikipedia: << A leap second is a one-second adjustment that is occasionally applied to Coordinated Universal Time (UTC),
        //! to accommodate the difference between precise time (as measured by atomic clocks) and imprecise observed solar time
        //! (known as UT1 and which varies due to irregularities and long-term slowdown in the Earth's rotation).
        //!
        //! The UTC time standard, widely used for international timekeeping and as the reference for civil time in most countries,
        //! uses precise atomic time and consequently would run ahead of observed solar time unless it is reset to UT1 as needed.
        //! The leap second facility exists to provide this adjustment.
        //!
        //! Because the Earth's rotation speed varies in response to climatic and geological events, UTC leap seconds are irregularly
        //! spaced and unpredictable. Insertion of each UTC leap second is usually decided about six months in advance by the
        //! International Earth Rotation and Reference Systems Service (IERS), to ensure that the difference between the UTC and UT1
        //! readings will never exceed 0.9 seconds. >>
        //!
        //! TSDuck uses a configuration file (tsduck.time.xml) to define the list of known leap seconds.
        //!
        //! The TAI (International Atomic Time) starts Jan 1st 1958. Before this date, there is no leap second.
        //! Between 1958 and 1972, there are globally 10 leap seconds but they are not precisely allocated.
        //! On June 30 1972, the first leap second was officially allocated at a precise date and time.
        //!
        //! If @a start and @a end date are both before 1958 or both after 1972, this function returns a precise result.
        //! If one of the dates is between 1958 and 1972, the initial 10 leap seconds may be included (or not).
        //!
        //! @param [in] end End UTC date. The time is this object is used as start date.
        //! @return The number of leap seconds between this object and @a end. Return zero if this object is after @a end.
        //!
        //! @see https://en.wikipedia.org/wiki/Leap_second
        //! @see https://en.wikipedia.org/wiki/International_Atomic_Time
        //!
        Second leapSecondsTo(const Time& end) const;

        //!
        //! Static method returning the current UTC time.
        //! @return The current UTC time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static Time CurrentUTC();

        //!
        //! Static method returning the current local time.
        //! @return The current local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static Time CurrentLocalTime() {return CurrentUTC().UTCToLocal();}

        //!
        //! Get the beginning of the current hour.
        //! @return The time for the beginning of the current hour from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time thisHour() const;

        //!
        //! Get the beginning of the next hour.
        //! @return The time for the beginning of the next hour from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time nextHour() const
        {
            return thisHour() + MilliSecPerHour;
        }

        //!
        //! Get the beginning of the current day.
        //! @return The time for the beginning of the current day from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time thisDay() const;

        //!
        //! Get the beginning of the next day.
        //! @return The time for the beginning of the next day from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time nextDay() const
        {
            return thisDay() + MilliSecPerDay;
        }

        //!
        //! Get the beginning of the current month.
        //! @return The time for the beginning of the current month from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time thisMonth() const;

        //!
        //! Get the beginning of the next month.
        //! @return The time for the beginning of the next month from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time nextMonth() const;

        //!
        //! Get the beginning of the current year.
        //! @return The time for the beginning of the current year from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time thisYear() const;

        //!
        //! Get the beginning of the next year.
        //! @return The time for the beginning of the next year from this object time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        Time nextYear() const;

        //!
        //! Get the beginning of the current hour, UTC.
        //! @return The time for the beginning of the current hour, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisHourUTC()
        {
            return CurrentUTC().thisHour();
        }

        //!
        //! Get the beginning of the current hour, local time.
        //! @return The time for the beginning of the current hour, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisHourLocalTime()
        {
            return CurrentLocalTime().thisHour();
        }

        //!
        //! Get the beginning of the next hour, UTC.
        //! @return The time for the beginning of the next hour, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextHourUTC()
        {
            return CurrentUTC().nextHour();
        }

        //!
        //! Get the beginning of the next hour, local time.
        //! @return The time for the beginning of the next hour, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextHourLocalTime()
        {
            return CurrentLocalTime().nextHour();
        }

        //!
        //! Get the beginning of the current day, UTC.
        //! @return The time for the beginning of the current day, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time TodayUTC()
        {
            return CurrentUTC().thisDay();
        }

        //!
        //! Get the beginning of the current day, local time.
        //! @return The time for the beginning of the current day, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time TodayLocalTime()
        {
            return CurrentLocalTime().thisDay();
        }

        //!
        //! Get the beginning of the next day, UTC.
        //! @return The time for the beginning of the next day, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time TomorrowUTC()
        {
            return CurrentUTC().nextDay();
        }

        //!
        //! Get the beginning of the next day, local time.
        //! @return The time for the beginning of the next day, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time TomorrowLocalTime()
        {
            return CurrentLocalTime().nextDay();
        }

        //!
        //! Get the beginning of the current month, UTC.
        //! @return The time for the beginning of the current month, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisMonthUTC()
        {
            return CurrentUTC().thisMonth();
        }

        //!
        //! Get the beginning of the current month, local time.
        //! @return The time for the beginning of the current month, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisMonthLocalTime()
        {
            return CurrentLocalTime().thisMonth();
        }

        //!
        //! Get the beginning of the next month, UTC.
        //! @return The time for the beginning of the next month, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextMonthUTC()
        {
            return CurrentUTC().nextMonth();
        }

        //!
        //! Get the beginning of the next month, local time.
        //! @return The time for the beginning of the next month, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextMonthLocalTime()
        {
            return CurrentLocalTime().nextMonth();
        }

        //!
        //! Get the beginning of the current year, UTC.
        //! @return The time for the beginning of the current year, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisYearUTC()
        {
            return CurrentUTC().thisYear();
        }

        //!
        //! Get the beginning of the current year, local time.
        //! @return The time for the beginning of the current year, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time ThisYearLocalTime()
        {
            return CurrentLocalTime().thisYear();
        }

        //!
        //! Get the beginning of the next year, UTC.
        //! @return The time for the beginning of the next year, UTC.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextYearUTC()
        {
            return CurrentUTC().nextYear();
        }

        //!
        //! Get the beginning of the next year, local time.
        //! @return The time for the beginning of the next year, local time.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        static inline Time NextYearLocalTime()
        {
            return CurrentLocalTime().nextYear();
        }

        //!
        //! Check if a year is a leap year (29 days in February).
        //! @param [in] year The year to check.
        //! @return True if @a year is a leap year.
        //!
        static bool IsLeapYear(int year);

        //!
        //! Constant representing the Epoch, ie the first representable time
        //! on this operating system.
        //!
        static const Time Epoch;

        //!
        //! Constant representing the Epoch on UNIX operating systems.
        //! The UNIX epoch is 1 Jan 1970 00:00:00.
        //! It is assumed to be representable in all operating systems.
        //!
        static const Time UnixEpoch;

        //!
        //! Constant representing the GPS Epoch, 1980-01-06.
        //! It is assumed to be representable in all operating systems.
        //!
        static const Time GPSEpoch;

        //!
        //! Constant representing the End Of Time (at least on this operating system).
        //!
        static const Time Apocalypse;

        //!
        //! Constant representing the offset of the Julian Epoch from the
        //! time Epoch of the operating system (Julian epoch - Time epoch)
        //! in milliseconds.
        //!
        //! The Julian epoch is 17 Nov 1858 00:00:00.
        //! If this constant is negative, the Julian epoch if before the time
        //! epoch and cannot be represented as a @c Time object.
        //!
        static const MilliSecond JulianEpochOffset;

        //!
        //! Number of seconds between 1970-01-01 and 1980-01-06.
        //! This is the difference between ATSC time and Unix time.
        //! The ATSC system time is the number of GPS seconds since 00:00:00 UTC, January 6th, 1980.
        //! This value can be displayed on a Linux system using the command: `date +%s --date 1980-01-06utc`
        //!
        static const Second UnixEpochToGPS = 315964800;

        //!
        //! This static routine converts a UNIX @c time_t to a UTC time.
        //!
        //! @param [in] unixTime A UNIX @c time_t value. Must be unsigned. Can be 32 or 64 bits.
        //! @return The corresponding UTC time.
        //!
        static Time UnixTimeToUTC(uint64_t unixTime);

        //!
        //! Convert this time in a UNIX @c time_t.
        //!
        //! @return The corresponding Unix time on 64 bits.
        //!
        uint64_t toUnixTime() const;

        //!
        //! This static routine converts a number of GPS seconds to a UTC time.
        //!
        //! @param [in] gps The number of seconds since the GPS Epoch (1980-01-06).
        //! @return The corresponding UTC time.
        //!
        static Time GPSSecondsToUTC(Second gps);

        //!
        //! Convert this time to a number of seconds since 1980-01-06, the GPS epoch.
        //!
        //! @return The corresponding number of seconds on 64 bits.
        //!
        Second toGPSSeconds() const;

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! This static routine converts a Win32 @c FILETIME to @c MilliSecond (Microsoft Windows only).
        //!
        //! This function is available on Microsoft Windows systems only
        //! and should not be used on portable software.
        //!
        //! @param [in] fileTime A Win32 @c FILETIME value.
        //! @return The corresponding number of milliseconds.
        //!
        static MilliSecond Win32FileTimeToMilliSecond(const ::FILETIME& fileTime);

        //!
        //! This static routine converts a Win32 @c FILETIME to a UTC time (Microsoft Windows only).
        //!
        //! This function is available on Microsoft Windows systems only
        //! and should not be used on portable software.
        //!
        //! @param [in] fileTime A Win32 @c FILETIME value.
        //! @return The corresponding UTC time.
        //!
        static Time Win32FileTimeToUTC(const ::FILETIME& fileTime);
#endif

#if defined(TS_UNIX) || defined(DOXYGEN)
        //!
        //! This static routine gets a system clock and adds a delay in milliseconds (UNIX systems only).
        //!
        //! This function ensures that no overflow is possible.
        //! This function is available on UNIX systems only and should not be used on portable software.
        //!
        //! @param [in] clock Clock id, usually @c CLOCK_REALTIME or @c CLOCK_MONOTONIC.
        //! @param [in] delay Number of milliseconds to add to the current clock.
        //! @return Absolute time in nanoseconds according to @a clock.
        //!
        static NanoSecond UnixClockNanoSeconds(clockid_t clock, const MilliSecond& delay = 0);

        //!
        //! This static routine gets a system clock and adds a delay in milliseconds (UNIX systems only).
        //!
        //! This function ensures that no overflow is possible.
        //! This function is available on UNIX systems only and should not be used on portable software.
        //!
        //! @param [out] result A returned UNIX @c timespec value.
        //! @param [in] clock Clock id, usually @c CLOCK_REALTIME or @c CLOCK_MONOTONIC.
        //! @param [in] delay Number of milliseconds to add to the current real time clock.
        //!
        static void GetUnixClock(::timespec& result, clockid_t clock, const MilliSecond& delay = 0);
#endif

    private:
        // A time is a 64-bit value. The resolution depends on the operating system.
        int64_t _value = 0;

        // Private constructor from a 64-bit value
        Time(const int64_t& value) : _value(value) {}

        // Static private routine: Build the 64-bit value from fields
        static int64_t ToInt64(int year, int month, int day, int hour, int minute, int second, int millisecond);

        // Number of clock ticks per millisecond:
        static const int64_t TICKS_PER_MS =
#if defined(TS_WINDOWS)
            // On Win32, a FILETIME is a 64-bit value representing the number
            // of 100-nanosecond intervals since January 1, 1601.
            // Number of FILETIME ticks per millisecond:
            10000;
#else
            // On UNIX, a struct timeval is a 64-bit value representing the number
            // of microseconds since January 1, 1970
            1000;
#endif

        // On Win32, a the FILETIME structure is binary-compatible with a 64-bit integer.
#if defined(TS_WINDOWS)
        union FileTime {
            ::FILETIME ft;
            int64_t i;
        };
#endif
    };
}
