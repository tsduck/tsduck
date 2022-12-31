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
//!
//!  @file
//!  Template representation of fixed-precision numbers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"
#include "tsIntegerUtils.h"

namespace ts {
    //!
    //! Template representation of fixed-precision numbers.
    //! @ingroup cpp
    //!
    //! A fixed-precision number is internally represented by an integer but with a different
    //! representation of a "unit". This concept is inspired by "fixed" types in Ada.
    //!
    //! All arithmetic and comparison operators are defined between fixed-point type values
    //! and between a fixed-point value and an integer value, both directions.
    //!
    //! @tparam INT_T The underlying signed integer type.
    //! @tparam PREC The decimal precision in digits.
    //!
    template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type = 0>
    class FixedPoint: public AbstractNumber
    {
    private:
        INT_T _value;

    public:
        //!
        //! The underlying signed integer type.
        //!
        typedef INT_T int_t;

        //!
        //! The precision of the fixed number type (number of decimal digits).
        //!
        static constexpr size_t PRECISION = PREC;

        //!
        //! The factor to convert between FixedPoint and int_t (10**PRECISION).
        //!
        static constexpr int_t FACTOR = ts::static_power10<int_t, PRECISION>::value;

        //!
        //! The minimum representable value of this fixed-point type.
        //!
        static const FixedPoint MIN;

        //!
        //! The maximum representable value of this fixed-point type.
        //!
        static const FixedPoint MAX;

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        FixedPoint() : _value(0) {}

        //!
        //! Constructor.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] i Initial value in integral number of units which is converted into the fixed-precision representation.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint(INT2 i) : _value(bounded_cast<int_t>(int64_t(i) * FACTOR)) {}

        //!
        //! Constructor.
        //! @param [in] i Initial value. Interpreted according to @a raw.
        //! @param [in] raw If true, the value @a i is a raw underlying internal value, without conversion.
        //! If false, the value @a i is an integral number of units which is converted into the fixed-precision representation.
        //!
        FixedPoint(int_t i, bool raw) : _value(raw ? i : i * FACTOR) {}

        // Implementation of interfaces.
        virtual ts::UString description() const override;
        virtual bool inRange(int64_t min, int64_t max) const override;
        virtual int64_t toInt64() const override;
        virtual double toDouble() const override;
        virtual bool fromString(const UString& str, UChar separator = COMMA, UChar decimal_dot = FULL_STOP) override;
        virtual UString toString(size_t min_width = 0,
                                 bool right_justified = true,
                                 UChar separator = COMMA,
                                 bool force_sign = false,
                                 size_t decimals = NPOS,
                                 bool force_decimals = false,
                                 UChar decimal_dot = FULL_STOP,
                                 UChar pad = SPACE) const override;

        //!
        //! Conversion to integral units.
        //! @return The value in integral units. Underflow or overflow rounding is applied when necessary.
        //!
        int_t toInt() const { return _value / FACTOR; }

        //!
        //! Get the internal unconverted integer value.
        //! @return The internal unconverted integer value.
        //!
        int_t raw() const { return _value; }

        //!
        //! Get the absolute value.
        //! @return The absolute value of this fixed-point number.
        //!
        FixedPoint abs() const { return _value >= 0 ? *this : FixedPoint(- _value, true); }

        //!
        //! Get the maximum value of two fixed-point numbers.
        //! @param [in] x Another fixed-point number.
        //! @return The maximum value of this fixed-point number and @a x.
        //!
        FixedPoint max(const FixedPoint& x) const { return FixedPoint(std::max(_value, x._value), true); }

        //!
        //! Get the minimum value of two fixed-point numbers.
        //! @param [in] x Another fixed-point number.
        //! @return The minimum value of this fixed-point number and @a x.
        //!
        FixedPoint min(const FixedPoint& x) const { return FixedPoint(std::min(_value, x._value), true); }

        //!
        //! Check if this fixed-point number generates an overflow when multiplied by an integer.
        //! @tparam INT2 Another integer type.
        //! @param [in] x An integer of type @a INT2.
        //! @return True if this fixed-point number generates an overflow when multiplied by @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool mulOverflow(INT2 x) const { return !bound_check<int_t>(x) || mul_overflow(_value, int_t(x)); }

        //!
        //! Check if this fixed-point number generates an overflow when multiplied by another fixed-point.
        //! @param [in] x Another fixed-point number.
        //! @return True if this fixed-point number generates an overflow when multiplied by @a x.
        //!
        bool mulOverflow(const FixedPoint& x) const { return mul_overflow(_value, x._value); }

        //!
        //! Check if this fixed-point number generates an overflow when divided by another fixed-point.
        //! @param [in] x Another fixed-point.
        //! @return True if this fixed-point generates an overflow when divided by @a x.
        //!
        bool divOverflow(const FixedPoint& x) const { return mul_overflow(_value, FACTOR); }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        FixedPoint operator-() const { return FixedPoint(- _value, true); }
        FixedPoint operator+(const FixedPoint& x) const { return FixedPoint(_value + x._value, true); }
        FixedPoint operator-(const FixedPoint& x) const { return FixedPoint(_value - x._value, true); }
        FixedPoint operator*(const FixedPoint& x) const { return FixedPoint((_value * x._value) / FACTOR, true); }
        FixedPoint operator/(const FixedPoint& x) const { return FixedPoint((_value * FACTOR) / x._value, true); }

        FixedPoint& operator+=(const FixedPoint& x) { _value += x._value; return *this; }
        FixedPoint& operator-=(const FixedPoint& x) { _value -= x._value; return *this; }
        FixedPoint& operator*=(const FixedPoint& x) { _value *= x._value; _value /= FACTOR; return *this; }
        FixedPoint& operator/=(const FixedPoint& x) { _value *= FACTOR; _value /= x._value; return *this; }

        bool operator==(const FixedPoint& x) const { return _value == x._value; }
#if defined(TS_NEED_UNEQUAL_OPERATOR)
        bool operator!=(const FixedPoint& x) const { return _value != x._value; }
#endif
        bool operator<=(const FixedPoint& x) const { return _value <= x._value; }
        bool operator>=(const FixedPoint& x) const { return _value >= x._value; }
        bool operator<(const FixedPoint& x) const { return _value < x._value; }
        bool operator>(const FixedPoint& x) const { return _value > x._value; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator+(INT2 x) const { return FixedPoint(_value + (int_t(x) * FACTOR), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator-(INT2 x) const { return FixedPoint(_value - (int_t(x) * FACTOR), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator*(INT2 x) const { return FixedPoint(_value * int_t(x), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator/(INT2 x) const { return FixedPoint(_value / int_t(x), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint& operator+=(INT2 x) { _value += int_t(x) * FACTOR; return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint& operator-=(INT2 x) { _value -= int_t(x) * FACTOR; return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint& operator*=(INT2 x) { _value *= int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint& operator/=(INT2 x) { _value /= int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator==(INT2 x) const { return _value == int_t(x) * FACTOR; }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator!=(INT2 x) const { return _value != int_t(x) * FACTOR; }
#endif

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<=(INT2 x) const { return _value <= int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>=(INT2 x) const { return _value >= int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<(INT2 x) const { return _value < int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>(INT2 x) const { return _value > int_t(x) * FACTOR; }

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline ts::FixedPoint<INT2, PREC> operator+(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 + x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline ts::FixedPoint<INT2, PREC> operator-(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return -(x2 - x1); }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline ts::FixedPoint<INT2, PREC> operator*(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 * x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline ts::FixedPoint<INT2, PREC> operator/(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return ts::FixedPoint<INT2, PREC>(x1) / x2; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator==(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2.operator==(x1); }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator!=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return !x2.operator==(x1); }
#endif

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator<=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 >= x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator>=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 <= x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator<(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 > x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
inline bool operator>(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 < x1; }

//! @endcond

#include "tsFixedPointTemplate.h"
