//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Basic monotonic clock & timer class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsException.h"

namespace ts {
    //!
    //! Basic monotonic clock & timer.
    //! @ingroup system
    //!
    class TSDUCKDLL Monotonic
    {
    public:
        //!
        //! Low-level monotonic clock error.
        //!
        TS_DECLARE_EXCEPTION(MonotonicError);

        //!
        //! Default constructor.
        //! @param [in] systemTime If true, initialize with current system time.
        //! @see getSystemTime()
        //!
        Monotonic(bool systemTime = false);

        //!
        //! Copy constructor.
        //! @param [in] t Another instance to copy.
        //!
        Monotonic(const Monotonic& t);

        //!
        //! Destructor.
        //!
        ~Monotonic();

        //!
        //! Load this object with the current system time.
        //!
        void getSystemTime();

        //!
        //! Assigment operator.
        //! @param [in] t Another instance to copy.
        //! @return A reference to this object.
        //!
        Monotonic& operator=(const Monotonic& t)
        {
            _value = t._value;
            return *this;
        }

        //!
        //! Comparison operator.
        //! @param [in] t Another instance to compare.
        //! @return True if this object == @a t.
        //!
        bool operator==(const Monotonic& t) const { return _value == t._value; }
        TS_UNEQUAL_OPERATOR(Monotonic)

        //!
        //! Comparison operator.
        //! @param [in] t Another instance to compare.
        //! @return True if this object < @a t.
        //!
        bool operator< (const Monotonic& t) const { return _value <  t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t Another instance to compare.
        //! @return True if this object <= @a t.
        //!
        bool operator<=(const Monotonic& t) const { return _value <= t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t Another instance to compare.
        //! @return True if this object > @a t.
        //!
        bool operator> (const Monotonic& t) const { return _value >  t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t Another instance to compare.
        //! @return True if this object >= @a t.
        //!
        bool operator>=(const Monotonic& t) const { return _value >= t._value; }

        //!
        //! Increment operator.
        //! @param [in] ns A number of nanoseconds to add.
        //! @return A reference to this object.
        //!
        Monotonic& operator+=(const NanoSecond& ns)
        {
            _value += ns / NS_PER_TICK;
            return *this;
        }

        //!
        //! Decrement operator.
        //! @param [in] ns A number of nanoseconds to substract.
        //! @return A reference to this object.
        //!
        Monotonic& operator-=(const NanoSecond& ns)
        {
            _value -= ns / NS_PER_TICK;
            return *this;
        }

        //!
        //! Difference operator.
        //! @param [in] t Another instance.
        //! @return The number of nanoseconds between this object and @a t. Can be negative.
        //!
        NanoSecond operator-(const Monotonic& t) const
        {
            return (_value - t._value) * NS_PER_TICK;
        }

        //!
        //! Wait until the time of the monotonic clock.
        //!
        void wait();

        //!
        //! This static method requests a minimum resolution, in nano-seconds, for the timers.
        //! @param [in] precision Requested minimum resolution in nano-seconds.
        //! @return The guaranteed precision value (can be equal to or greater than the requested value).
        //! The default system resolution is 20 ms on Win32, which can be too long for applications.
        //!
        static NanoSecond SetPrecision(const NanoSecond& precision);

    private:
        // Monotonic clock value in system ticks
        int64_t _value = 0;

#if defined(TS_WINDOWS)
        // Timer handle
        ::HANDLE _handle {INVALID_HANDLE_VALUE};

        // On Win32, a FILETIME is a 64-bit value representing the number
        // of 100-nanosecond intervals since January 1, 1601.
        // Number of nanoseconds per FILETIME ticks:
        static const int64_t NS_PER_TICK = 100;
#else
        // On POSIX systems, the clock unit is the nanosecond.
        static const int64_t NS_PER_TICK = 1;
#endif
    };
}
