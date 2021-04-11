//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
#include "tsUString.h"
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
    //! @tparam INT The underlying signed integer type.
    //! @tparam PREC The decimal precision in digits.
    //!
    template <typename INT, const size_t PREC, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    class FixedPoint final
    {
    private:
        INT _value;

    public:
        //!
        //! The underlying signed integer type.
        //!
        typedef INT int_t;

        //!
        //! The precision of the fixed number type (number of decimal digits).
        //!
        static constexpr size_t PRECISION = PREC;

        //!
        //! The factor to convert between FixedPoint and int_t (10**PRECISION).
        //!
        static constexpr int_t FACTOR = ts::static_power10<int_t, PRECISION>::value;

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
        FixedPoint(INT2 i) : _value(int_t(i) * FACTOR) {}

        //!
        //! Constructor.
        //! @param [in] i Initial value. Interpreted according to @a raw.
        //! @param [in] raw If true, the value @a i is a raw underlying internal value, without conversion.
        //! If false, the value @a i is an integral number of units which is converted into the fixed-precision representation.
        //!
        FixedPoint(int_t i, bool raw) : _value(raw ? i : i * FACTOR) {}

        //!
        //! Constructor from a string.
        //! @param [in] str The string to decode.
        //! @see FromString()
        //!
        FixedPoint(const UString& str) : _value(0) { FromString(*this, str); }

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

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        FixedPoint operator-() const { return FixedPoint(- _value, true); }
        FixedPoint operator+(FixedPoint x) const { return FixedPoint(_value + x._value, true); }
        FixedPoint operator-(FixedPoint x) const { return FixedPoint(_value - x._value, true); }
        bool operator==(FixedPoint x) const { return _value == x._value; }
        bool operator!=(FixedPoint x) const { return _value != x._value; }
        bool operator<=(FixedPoint x) const { return _value <= x._value; }
        bool operator>=(FixedPoint x) const { return _value >= x._value; }
        bool operator<(FixedPoint x) const { return _value < x._value; }
        bool operator>(FixedPoint x) const { return _value > x._value; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator+(INT2 x) const { return FixedPoint(_value + (int_t(x) * FACTOR), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator-(INT2 x) const { return FixedPoint(_value - (int_t(x) * FACTOR), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator*(INT2 x) const { return FixedPoint(_value * int_t(x), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        FixedPoint operator/(INT2 x) const { return FixedPoint(_value / int_t(x), true); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator==(INT2 x) const { return _value == int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator!=(INT2 x) const { return _value != int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<=(INT2 x) const { return _value <= int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>=(INT2 x) const { return _value >= int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<(INT2 x) const { return _value < int_t(x) * FACTOR; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>(INT2 x) const { return _value > int_t(x) * FACTOR; }

        //! @endcond

        //!
        //! Format a string containing the value of this FixedPoint.
        //! @param [in] min_width Minimum width of the returned string.
        //! Padded with spaces if larger than the number of characters in the formatted number.
        //! @param [in] right_justified If true (the default), return a right-justified string.
        //! When false, return a left-justified string. Ignored if @a min_width is lower than
        //! the number of characters in the formatted number.
        //! @param [in] separator Separator string for groups of thousands, a comma by default.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @param [in] force_decimals If true, with positive precision, force a decimal dot
        //! and the number of decimal digits of the precision. By default, skip non
        //! significant decimal digits.
        //! @param [in] pad The padding character to adjust the width.
        //! @return The formatted string.
        //!
        UString toString(size_t min_width = 0,
                         bool right_justified = true,
                         const UString& separator = UString::DEFAULT_THOUSANDS_SEPARATOR,
                         bool force_sign = false,
                         bool force_decimals = false,
                         UChar pad = SPACE) const;

        //!
        //! Convert a string into a FixedPoint.
        //! @param [out] value The returned decoded value. On error (invalid string), @a value
        //! contains what could be decoded up to the first invalid character.
        //! @param [in] str The string to decode. It must contain the representation of an integer
        //! value in decimal or hexadecimal (prefix @c 0x). Hexadecimal values are case-insensitive,
        //! including the @c 0x prefix. Leading and trailing spaces are ignored. Optional thousands
        //! separators are ignored. For FixedPoint with a positive precision, a decimal
        //! dot and decimal digits are accepted. A decimal dot is allowed only in base 10.
        //! @return True on success, false on error (invalid string).
        //!
        static bool FromString(FixedPoint& value, const UString& str)
        {
            return str.toInteger(value._value, u",", PRECISION);
        }
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
ts::FixedPoint<INT2, PREC> operator+(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 + x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
ts::FixedPoint<INT2, PREC> operator-(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return -(x2 - x1); }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
ts::FixedPoint<INT2, PREC> operator*(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 * x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator==(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 == x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator!=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 != x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator<=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 >= x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator>=(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 <= x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator<(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 > x1; }

template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
bool operator>(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 < x1; }

//! @endcond

#include "tsFixedPointTemplate.h"
