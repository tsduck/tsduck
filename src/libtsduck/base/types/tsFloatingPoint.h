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
//!  Encapsulation of a floating-point type as an AbstractNumber.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"

namespace ts {
    //!
    //! Encapsulation of a double floating-point as an AbstractNumber.
    //! @ingroup cpp
    //!
    //! @tparam FLOAT_T The underlying floating-point type.
    //! @tparam PREC The number of decimal digits to display by default. This is a display
    //! attribute only, it does not alter the binary representation of floating-point values.
    //! The default is to display 6 digits.
    //!
    template <typename FLOAT_T, const size_t PREC = 6, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type = 0>
    class FloatingPoint: public AbstractNumber
    {
    private:
        FLOAT_T _value;

    public:
        //!
        //! The underlying floating-point type.
        //!
        typedef FLOAT_T float_t;

        //!
        //! An integer type for conversion to integral values.
        //! Overflow may occur.
        //!
        typedef int64_t int_t;

        //!
        //! The displayed precision of the floating-point type (number of decimal digits).
        //! This is a display attribute only, it does not alter the binary representation of floating-point values.
        //!
        static constexpr size_t DISPLAY_PRECISION = PREC;

        //!
        //! The minimum representable value of this fraction type.
        //!
        static const FloatingPoint MIN;

        //!
        //! The maximum representable value of this fraction type.
        //!
        static const FloatingPoint MAX;

        //!
        //! Precision of "equal" comparisons.
        //! We cannot test equality between floating point values. Because of rounding issues,
        //! it does not make sense. Instead we check if the absolute value of their difference
        //! is lower than some very small value, this precision.
        //!
        static constexpr float_t EQUAL_PRECISION = 100.0 * std::numeric_limits<float_t>::min();

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        FloatingPoint() : _value(0.0) {}

        //!
        //! Constructor from an integer or floating-point value.
        //! @tparam NUM_T Some integer or floating-point type.
        //! @param [in] x Initial value.
        //!
        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint(NUM_T x) : _value(float_t(x)) {}

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
        //! Conversion to integral value.
        //! @return The value in integral units. Underflow or overflow rounding is applied when necessary.
        //!
        int_t toInt() const { return int_t(std::round(_value)); }

        //!
        //! Get the absolute value.
        //! @return The absolute value of this fraction number.
        //!
        FloatingPoint abs() const { return FloatingPoint(std::abs(_value)); }

        //!
        //! Get the maximum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The maximum value of this fraction and @a x.
        //!
        FloatingPoint max(const FloatingPoint& x) const { return _value >= x._value ? *this : x; }

        //!
        //! Get the minimum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The minimum value of this fraction and @a x.
        //!
        FloatingPoint min(const FloatingPoint& x) const { return _value <= x._value ? *this : x; }

        //!
        //! Check if this FloatingPoint number generates an overflow when multiplied by an integer.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @tparam INT2 Another integer type.
        //! @param [in] x An integer of type @a INT2.
        //! @return Always false. A better idea?
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool mulOverflow(INT2 x) const { return false; }

        //!
        //! Check if this FloatingPoint number generates an overflow when multiplied by another FloatingPoint.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @param [in] x Another FloatingPoint number.
        //! @return Always false. A better idea?
        //!
        bool mulOverflow(const FloatingPoint& x) const { return false; }

        //!
        //! Check if this FloatingPoint number generates an overflow when divided by another FloatingPoint.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @param [in] x Another FloatingPoint.
        //! @return Always false. There is no possible division overflow with FloatingPoint.
        //!
        bool divOverflow(const FloatingPoint& x) const { return false; }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        FloatingPoint operator-() const { return FloatingPoint(-_value); }
        FloatingPoint operator+(const FloatingPoint& x) const { return FloatingPoint(_value + x._value); }
        FloatingPoint operator-(const FloatingPoint& x) const { return FloatingPoint(_value - x._value); }
        FloatingPoint operator*(const FloatingPoint& x) const { return FloatingPoint(_value * x._value); }
        FloatingPoint operator/(const FloatingPoint& x) const { return FloatingPoint(_value / x._value); }

        FloatingPoint& operator+=(const FloatingPoint& x) { _value += x._value; return *this; }
        FloatingPoint& operator-=(const FloatingPoint& x) { _value -= x._value; return *this; }
        FloatingPoint& operator*=(const FloatingPoint& x) { _value *= x._value; return *this; }
        FloatingPoint& operator/=(const FloatingPoint& x) { _value /= x._value; return *this; }

        bool operator==(const FloatingPoint& x) const { return std::abs(_value - x._value) < EQUAL_PRECISION; }
#if defined(TS_NEED_UNEQUAL_OPERATOR)
        bool operator!=(const FloatingPoint& x) const { return std::abs(_value - x._value) >= EQUAL_PRECISION; }
#endif
        bool operator<=(const FloatingPoint& x) const { return _value <= x._value; }
        bool operator>=(const FloatingPoint& x) const { return _value >= x._value; }
        bool operator<(const FloatingPoint& x) const { return _value < x._value; }
        bool operator>(const FloatingPoint& x) const { return _value > x._value; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator+(NUM_T x) const { return FloatingPoint(_value + float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator-(NUM_T x) const { return FloatingPoint(_value - float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator*(NUM_T x) const { return FloatingPoint(_value * float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator/(NUM_T x) const { return FloatingPoint(_value / float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator+=(NUM_T x) { _value += float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator-=(NUM_T x) { _value -= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator*=(NUM_T x) { _value *= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator/=(NUM_T x) { _value /= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator==(NUM_T x) const { return std::abs(_value - float_t(x)) < EQUAL_PRECISION; }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator!=(NUM_T x) const { return std::abs(_value - float_t(x)) >= EQUAL_PRECISION; }
#endif

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator<=(NUM_T x) const { return _value <= float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator>=(NUM_T x) const { return _value >= float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator<(NUM_T x) const { return _value < float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator>(NUM_T x) const { return _value > float_t(x); }

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator+(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 + x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator-(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return -(x2 - x1); }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator*(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 * x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator/(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return ts::FloatingPoint<FLOAT_T,PREC>(x1) / x2; }

// This one explicitly calls the operator== method because without it, you get an error when built with C++20
// error: in C++20 this comparison calls the current function recursively with reversed arguments
template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator==(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2.operator==(x1); }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator!=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return !x2.operator==(x1); }
#endif

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator<=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 >= x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator>=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 <= x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator<(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 > x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator>(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 < x1; }

//! @endcond

#include "tsFloatingPointTemplate.h"
