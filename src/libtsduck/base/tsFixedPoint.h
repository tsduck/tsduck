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

namespace ts {
    //!
    //! Template representation of fixed-precision numbers.
    //! @ingroup cpp
    //!
    //! A fixed-precision number is internally represented by an integer but with a
    //! different representation of a "unit". This concept is inspired by "fixed"
    //! types in Ada.
    //!
    //! @tparam INT The underlying signed integer type.
    //! @tparam PREC The decimal precision in digits.
    //!
    template <typename INT, const size_t PREC, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
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
        //! The precision of the fixed number type.
        //! - When zero, the precision is the same as int_t, meaning that the FixedPoint is just an int_t.
        //! - When positive, this is the number of decimal digits (after the deciman dot). The precision if greater than int_t.
        //! - When negative, its absolute value is the number of non-significant right-most digits. The precision if lower than int_t.
        //!
        static constexpr size_t PRECISION = PREC;

        //!
        //! The multiple or divider factor to convert between FixedPoint and int_t.
        //! - When the precision is zero, the factor is 1.
        //! - When the precision is positive, this is the divider of unit (e.g. 1000 when the precision is 3).
        //! - When the precision is negative, this is the multiple of unit (e.g. 1000 when the precision is -3).
        //!
        static const int_t FACTOR;

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        FixedPoint() : _value(0) {}

        //!
        //! Constructor.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] i Initial value in integral number of units which is converted into the fixed-precision representation.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value>::type* = nullptr>
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

        //!
        //! Negation operator.
        //! @return The opposite of this FixedPoint.
        //!
        FixedPoint operator-() const { return FixedPoint(- _value, true); }

        //!
        //! Addition operator.
        //! @param [in] x Value to add as a FixedPoint.
        //! @return This FixedPoint plus @a x.
        //!
        FixedPoint operator+(FixedPoint x) const { return FixedPoint(_value + x._value, true); }

        //!
        //! Addition operator.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] x Value to add as an integral number of units (to be converted in fixed-precision).
        //! @return This FixedPoint plus @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value>::type* = nullptr>
        FixedPoint operator+(INT2 x) const { return FixedPoint(_value + (int_t(x) * FACTOR), true); }

        //!
        //! Substraction operator.
        //! @param [in] x Value to substract as a FixedPoint.
        //! @return This FixedPoint minus @a x.
        //!
        FixedPoint operator-(FixedPoint x) const { return FixedPoint(_value - x._value, true); }

        //!
        //! Substraction operator.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] x Value to substract as an integral number of units (to be converted in fixed-precision).
        //! @return This FixedPoint minus @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value>::type* = nullptr>
        FixedPoint operator-(INT2 x) const { return FixedPoint(_value - (int_t(x) * FACTOR), true); }

        //!
        //! Multiplication operator.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] x Value to multiply (not converted).
        //! @return This FixedPoint times @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value>::type* = nullptr>
        FixedPoint operator*(INT2 x) const { return FixedPoint(_value * int_t(x), true); }

        //!
        //! Division operator.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] x Value to divide (not converted).
        //! @return This FixedPoint divided by @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value>::type* = nullptr>
        FixedPoint operator/(INT2 x) const { return FixedPoint(_value / int_t(x), true); }

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

//!
//! Addition operator.
//! @tparam INT1 Some integer type (signed or unsigned).
//! @tparam INT2 The underlying signed integer type of a FixedPoint.
//! @tparam PREC The precision in digits of the FixedPoint.
//! @param [in] x1 An integer value (to be converted in fixed-precision).
//! @param [in] x2 A FixedPoint value.
//! @return @a x1 plus @a x2.
//!
template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value>::type* = nullptr,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value>::type* = nullptr>
ts::FixedPoint<INT2, PREC> operator+(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 + x1; }

//!
//! Substraction operator.
//! @tparam INT1 Some integer type (signed or unsigned).
//! @tparam INT2 The underlying signed integer type of a FixedPoint.
//! @tparam PREC The precision in digits of the FixedPoint.
//! @param [in] x1 An integer value (to be converted in fixed-precision).
//! @param [in] x2 A FixedPoint value.
//! @return @a x1 minus @a x2.
//!
template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value>::type* = nullptr,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value>::type* = nullptr>
ts::FixedPoint<INT2, PREC> operator-(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return -(x2 - x1); }

//!
//! Multiplication operator.
//! @tparam INT1 Some integer type (signed or unsigned).
//! @tparam INT2 The underlying signed integer type of a FixedPoint.
//! @tparam PREC The precision in digits of the FixedPoint.
//! @param [in] x1 An integer value to multiply (not converted).
//! @param [in] x2 A FixedPoint value to multiply.
//! @return @a x1 times @a x2.
//!
template <typename INT1, typename INT2, const size_t PREC,
          typename std::enable_if<std::is_integral<INT1>::value>::type* = nullptr,
          typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value>::type* = nullptr>
ts::FixedPoint<INT2, PREC> operator*(INT1 x1, ts::FixedPoint<INT2, PREC> x2) { return x2 * x1; }

#include "tsFixedPointTemplate.h"
