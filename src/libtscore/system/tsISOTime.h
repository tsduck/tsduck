//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::ISOTime class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"

namespace ts {
    //!
    //! The @c ISOTime class convert dates, times, and durations with ISO 8601 representation.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL ISOTime
    {
    public:
        //!
        //! Type of time data which can be represented in ISO 8601.
        //!
        enum TimeType {
            NONE           = 0x00,  //!< No data, invalid object.
            TIME           = 0x01,  //!< One date & time value.
            DURATION       = 0x02,  //!< A duration not associated with any start or end.
            START_END      = 0x04,  //!< A start and an end.
            START_DURATION = 0x08,  //!< A start and a duration.
            DURATION_END   = 0x10,  //!< A duration and an end.
            RECURRING      = 0x20,  //!< Additional flag: recurring time interval.
            ANY_INTERVAL   = 0x1E,  //!< A bit mask of all time-interval formats (non-recurring).
            ANY            = 0x3F,  //!< A bit mask of all possible formats.
        };

        //!
        //! Number of time-interval recurrences meaning "unbounded".
        //!
        static constexpr size_t UNBOUNDED_RECURRENCES = std::numeric_limits<size_t>::max();

        //!
        //! Default constructor.
        //!
        ISOTime() = default;

        //!
        //! Constructor from a string in ISO 8601 format.
        //! @param [in] str A string in ISO 8601 format.
        //!
        ISOTime(const UString& str) { fromString(str); }

        //!
        //! Constructor using one date & time value.
        //! @param [in] time The date & time to set.
        //!
        ISOTime(const Time& time) :
            _type(TIME),
            _start(time)
        {}

        //!
        //! Constructor using a duration not associated with any start or end.
        //! @param [in] duration Time interval duration.
        //! @param [in] recurrences If non-zero, the time-interval is recurring, with that number of recurrences.
        //!
        template <class Rep, class Period>
        ISOTime(cn::duration<Rep,Period> duration, size_t recurrences = 0) :
            _type(TimeType(DURATION | (recurrences > 0 ? RECURRING : NONE))),
            _duration(duration),
            _recurrences(recurrences)
        {}

        //!
        //! Constructor using a start and end date.
        //! @param [in] start Interval start time.
        //! @param [in] end Interval end time.
        //! @param [in] recurrences If non-zero, the time-interval is recurring, with that number of recurrences.
        //!
        ISOTime(const Time& start, const Time& end, size_t recurrences = 0) :
            _type(TimeType(START_END | (recurrences > 0 ? RECURRING : NONE))),
            _start(start),
            _end(end),
            _recurrences(recurrences)
        {}

        //!
        //! Constructor using a start and a duration.
        //! @param [in] start Interval start time.
        //! @param [in] duration Time interval duration.
        //! @param [in] recurrences If non-zero, the time-interval is recurring, with that number of recurrences.
        //!
        template <class Rep, class Period>
        ISOTime(const Time& start, cn::duration<Rep,Period> duration, size_t recurrences = 0) :
            _type(TimeType(START_DURATION | (recurrences > 0 ? RECURRING : NONE))),
            _start(start),
            _duration(duration),
            _recurrences(recurrences)
        {}

        //!
        //! Constructor using a duration and an end.
        //! @param [in] duration Time interval duration.
        //! @param [in] end Interval end time.
        //! @param [in] recurrences If non-zero, the time-interval is recurring, with that number of recurrences.
        //!
        template <class Rep, class Period>
        ISOTime(cn::duration<Rep,Period> duration, const Time& end, size_t recurrences = 0) :
            _type(TimeType(DURATION_END | (recurrences > 0 ? RECURRING : NONE))),
            _end(end),
            _duration(duration),
            _recurrences(recurrences)
        {}

        //!
        //! Reset the content of the object to an invalid state.
        //!
        void clear();

        //!
        //! Check if this object contains a valid value.
        //! @return True if this object contains a valid value, false otherwise.
        //!
        bool isValid() const { return _type != NONE; }

        //!
        //! Check if this object contains a single date & time value.
        //! @return True if this object contains a single date & time value, false otherwise.
        //!
        bool isSingleTime() const { return _type == TIME; }

        //!
        //! Check if this object contains a time interval (recurring or not).
        //! @return True if this object contains a time interval, false otherwise.
        //!
        bool isInterval() const { return (_type & ANY_INTERVAL) != 0; }

        //!
        //! Check if this object contains a recurring interval.
        //! @return True if this object contains a recurring interval, false otherwise.
        //!
        bool isRecurring() const { return (_type & RECURRING) != 0; }

        //!
        //! Check if this object contains an unbounded recurring interval.
        //! @return True if this object contains an unbounded recurring interval, false otherwise.
        //!
        bool isUnbounded() const { return _recurrences == UNBOUNDED_RECURRENCES; }

        //!
        //! Get the type of ISO 8601 time data.
        //! @return The type of ISO 8601 time data.
        //!
        TimeType type() const { return _type; }

        //!
        //! Format this object as a ISO 8601 string.
        //! @param [in] format Required format. In NONE, use the default format for that object.
        //! @return The corresponding ISO 8601 string.
        //!
        UString toString(TimeType format = NONE) const;

        //!
        //! Set the value of this object from a string in ISO 8601 format.
        //! @param [in] str A string in ISO 8601 format.
        //! @return True on success, false on error (the object is then invalid).
        //!
        bool fromString(const UString& str);

        //!
        //! Conversion to a Time value.
        //! @return The single date and time or the start date for a time interval.
        //! Return ts::Time::Epoch on error.
        //!
        operator Time() const { return start(); }

        //!
        //! Get the time interval start time.
        //! @return The start date for a time interval or single date and time.
        //! Return ts::Time::Epoch on error.
        //!
        Time start() const;

        //!
        //! Get the time interval end time.
        //! @return The end date for a time interval or single date and time.
        //! Return ts::Time::Epoch on error.
        //!
        Time end() const;

        //!
        //! Get the duration of the time interval.
        //! @return The duration of the time interval, zero otherwise.
        //!
        cn::milliseconds duration() const;

        //!
        //! Get the number of recurrences of the time interval.
        //! @return The number of recurrences of the time interval for a recurring interval, zero otherwise.
        //!
        size_t recurrences() const { return _recurrences; }

        //!
        //! Decode a time from an ISO 8601 representation.
        //! Missing date fields default to the current UTC time.
        //! Missing time fields default to zero.
        //! @param [in] str A string describing a date and time in ISO 8601 representation.
        //! @return The converted time or ts::Time::Epoch if the string cannot be decoded.
        //!
        static Time TimeFromISO(const UString& str);

        //!
        //! Decode a duration from an ISO 8601 representation.
        //! @param [in] str A string describing a duration in ISO 8601 representation.
        //! @return The converted duration in milliseconds or a negative value if the string cannot be decoded.
        //!
        static cn::milliseconds DurationFromISO(const UString& str);

        //!
        //! Format a time in ISO 8601 representation.
        //! @param [in] time The date and time to format.
        //! @return A string describing the date and time in ISO 8601 representation.
        //!
        static inline UString ToISO(const Time& time) { return ToIsoWithMinutes(time, 0); }

        //!
        //! Format a time in ISO 8601 representation, including an offset from UTC time.
        //! @param [in] time The date and time to format.
        //! @param [in] utc_offset The offset from UTC time to include in the representation.
        //! @return A string describing the date and time in ISO 8601 representation.
        //!
        template <class Rep, class Period>
        static inline UString ToISO(const Time& time, cn::duration<Rep,Period> utc_offset)
        {
            return ToIsoWithMinutes(time, cn::duration_cast<cn::minutes>(utc_offset).count());
        }

        //!
        //! Format a duration in ISO 8601 representation.
        //! @param [in] duration Time interval duration.
        //! @return A string describing the duration in ISO 8601 representation.
        //!
        template <class Rep, class Period>
        static inline UString ToISO(cn::duration<Rep,Period> duration)
        {
            return MillisecondsToIso(cn::duration_cast<cn::milliseconds>(duration).count());
        }

    private:
        TimeType         _type = NONE;
        Time             _start {};  // Start time or unique date.
        Time             _end {};
        cn::milliseconds _duration {};
        size_t           _recurrences = 0;

        // For the purpose of representing durations, a "month" is arbitrarily a duration of 30 days
        // and all years have 365 days. This is suggested in ISO 8601, although not very clear.
        static constexpr cn::milliseconds::rep DAYS_PER_MONTH = 30;
        static constexpr cn::milliseconds::rep DAYS_PER_YEAR = 365;
        static constexpr cn::milliseconds::rep MS_PER_DAY = 1000 * 60 * 60 * 24;
        static constexpr cn::milliseconds::rep MS_PER_WEEK = MS_PER_DAY * 7;
        static constexpr cn::milliseconds::rep MS_PER_MONTH = MS_PER_DAY * DAYS_PER_MONTH;
        static constexpr cn::milliseconds::rep MS_PER_YEAR = MS_PER_DAY * DAYS_PER_YEAR;

        // Format a time in ISO 8601 representation, including an offset from UTC time.
        static UString ToIsoWithMinutes(const Time& time, intmax_t utc_offset);
        static UString MillisecondsToIso(cn::milliseconds::rep ms);
    };
}
