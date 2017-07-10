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
//!
//!  @file
//!  Declare the ts::Time class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsException.h"

namespace ts {
    //!
    //! The @c Time class implements a basic representation of time.
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
    class TSDUCKDLL Time
    {
    public:
        //!
        //! Fatal low-level time error.
        //!
        tsDeclareException(TimeError);

        //!
        //! Default constructor.
        //! The initial value is the Epoch.
        //!
        Time() : _value(0) {}

        //!
        //! Copy constructor.
        //! @param [in] other Another time from which to initialize this object.
        //!
        Time(const Time& other) : _value(other._value) {}

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
        //! Assigment operator.
        //! @param [in] other Another time from which to initialize this object.
        //! @return A reference to this object.
        //!
        Time& operator=(const Time& other)
        {
            _value = other._value;
            return *this;
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
        bool operator==(const Time& other) const
        {
            return _value == other._value;
        }

        //!
        //! Unequality operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this object is different from the @a other object,
        //! @c false otherwise.
        //!
        bool operator!=(const Time& other) const
        {
            return _value != other._value;
        }

        //!
        //! Lower operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is before the @a other object time,
        //! @c false otherwise.
        //!
        bool operator<(const Time& other) const
        {
            return _value < other._value;
        }

        //!
        //! Lower or equal operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is before or equal to the @a other object time,
        //! @c false otherwise.
        //!
        bool operator<=(const Time& other) const
        {
            return _value <= other._value;
        }

        //!
        //! Greater operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is after the @a other object time,
        //! @c false otherwise.
        //!
        bool operator>(const Time& other) const
        {
            return _value > other._value;
        }

        //!
        //! Greater or equal operator.
        //! @param [in] other Another time to compare with this object.
        //! @return @c True is this time is after or equal to the @a other object time,
        //! @c false otherwise.
        //!
        bool operator>=(const Time& other) const
        {
            return _value >= other._value;
        }

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

            //!
            //! Unequality operator.
            //! @param [in] other Another @c Fields to compare with this object.
            //! @return @c True is this object is different from the @a other object,
            //! @c false otherwise.
            //!
            bool operator!=(const Fields& other) const;
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
            MILLISECOND = 0x40,  //!< Display the milliseconds.
            ALL         = DATE | TIME | MILLISECOND,  //!< Display all fields.
        };

        //!
        //! Format a string representation of a time.
        //! @param [in] fields A combination of option flags indicating which fields
        //! should be included. This is typically the result of or'ed values from the enum
        //! type @link FieldMask @endlink.
        //! @return A string containing the formatted date.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        std::string format(int fields = ALL) const;

        //!
        //! Conversion operator from @c Time to @c std::string.
        //! Equivalent to <code>format (ALL)</code>.
        //! @return A string containing the formatted date.
        //! @throw ts::Time::TimeError In case of operating system time error.
        //!
        operator std::string() const
        {
            return format(ALL);
        }

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
        //! Constant representing the Epoch, ie the first representable time
        //! on this operating system.
        //!
        static const Time Epoch;

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

#if defined(__windows) || defined(DOXYGEN)
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

        //!
        //! This static routine converts a UNIX @c time_t to a UTC time.
        //!
        //! @param [in] unixTime A UNIX @c time_t value.
        //! @return The corresponding UTC time.
        //!
        static Time UnixTimeToUTC(const uint32_t unixTime);

#if defined(__unix) || defined(DOXYGEN)
        //!
        //! This static routine gets the current real time clock and adds a delay in milliseconds (UNIX systems only).
        //!
        //! This function ensures that no overflow is possible.
        //! This function is available on UNIX systems only
        //! and should not be used on portable software.
        //!
        //! @param [in] delay Number of milliseconds to add to the current real time clock.
        //! @return Absolute time in nanoseconds.
        //!
        static NanoSecond UnixRealTimeClockNanoSeconds(const MilliSecond& delay = 0);

        //!
        //! This static routine gets the current real time clock and adds a delay in milliseconds (UNIX systems only).
        //!
        //! This function ensures that no overflow is possible.
        //! This function is available on UNIX systems only
        //! and should not be used on portable software.
        //!
        //! @param [out] result A returned UNIX @c timespec value.
        //! @param [in] delay Number of milliseconds to add to the current real time clock.
        //!
        static void UnixRealTimeClock(::timespec& result, const MilliSecond& delay = 0);
#endif

    private:
        // A time is a 64-bit value. The resolution depends on the operating system.
        int64_t _value;

        // Private constructor from a 64-bit value
        Time(const int64_t& value) : _value(value) {}

        // Static private routine: Build the 64-bit value from fields
        static int64_t ToInt64(int year, int month, int day, int hour, int minute, int second, int millisecond);

        // Number of clock ticks per millisecond:
        static const int64_t TICKS_PER_MS =
#if defined(__windows)
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
#if defined(__windows)
        union FileTime {
            ::FILETIME ft;
            int64_t i;
        };
#endif
    };
}

//!
//! Output operator for the class @link ts::Time @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] time A @link ts::Time @endlink object.
//! @return A reference to the @a strm object.
//! @throw ts::Time::TimeError In case of operating system time error.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::Time& time)
{
    return strm << std::string(time);
}
