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
//!  Encapsulation of a double floating-point as an AbstractNumber.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"
#include "tsIntegerUtils.h"

namespace ts {
    //!
    //! Encapsulation of a double floating-point as an AbstractNumber.
    //! @ingroup cpp
    //!
    class TSDUCKDLL Double: public AbstractNumber
    {
    private:
        double _value;

    public:
        //!
        //! The underlying floating-point type.
        //!
        typedef double float_t;

        //!
        //! An integer type for conversion to integral values.
        //! Overflow may occur.
        //!
        typedef int64_t int_t;

        //!
        //! The minimum representable value of this fraction type.
        //!
        static const Double MIN;

        //!
        //! The maximum representable value of this fraction type.
        //!
        static const Double MAX;

        //!
        //! Precision of "equal" comparisons.
        //! We cannot test equality between floating point values. Because of rounding issues,
        //! it does not make sense. Instead we check if the absolute value of their difference
        //! is lower than some very small value, this precision.
        //!
        static constexpr double EQUAL_PRECISION = 1000.0 * std::numeric_limits<double>::min();

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        Double() : _value(0.0) {}

        //!
        //! Constructor from a floating-point value.
        //! @param [in] x Initial floating point value.
        //!
        Double(double x) : _value(x) {}

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
        Double abs() const { return Double(std::abs(_value)); }

        //!
        //! Get the maximum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The maximum value of this fraction and @a x.
        //!
        Double max(const Double& x) const { return _value >= x._value ? *this : x; }

        //!
        //! Get the minimum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The minimum value of this fraction and @a x.
        //!
        Double min(const Double& x) const { return _value <= x._value ? *this : x; }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        Double operator-() const { return Double(-_value); }
        Double operator+(const Double& x) const { return Double(_value + x._value); }
        Double operator-(const Double& x) const { return Double(_value - x._value); }
        Double operator*(const Double& x) const { return Double(_value * x._value); }
        Double operator/(const Double& x) const { return Double(_value / x._value); }

        Double& operator+=(const Double& x) { _value += x._value; return *this; }
        Double& operator-=(const Double& x) { _value -= x._value; return *this; }
        Double& operator*=(const Double& x) { _value *= x._value; return *this; }
        Double& operator/=(const Double& x) { _value /= x._value; return *this; }

        bool operator==(const Double& x) const { return std::abs(_value - x._value) < EQUAL_PRECISION; }
        bool operator!=(const Double& x) const { return std::abs(_value - x._value) >= EQUAL_PRECISION; }
        bool operator<=(const Double& x) const { return _value <= x._value; }
        bool operator>=(const Double& x) const { return _value >= x._value; }
        bool operator<(const Double& x) const { return _value < x._value; }
        bool operator>(const Double& x) const { return _value > x._value; }

        Double operator+(double x) const { return Double(_value + x); }
        Double operator-(double x) const { return Double(_value - x); }
        Double operator*(double x) const { return Double(_value * x); }
        Double operator/(double x) const { return Double(_value / x); }

        Double& operator+=(double x) { _value += x; return *this; }
        Double& operator-=(double x) { _value -= x; return *this; }
        Double& operator*=(double x) { _value *= x; return *this; }
        Double& operator/=(double x) { _value /= x; return *this; }

        bool operator==(double x) const { return std::abs(_value - x) < EQUAL_PRECISION; }
        bool operator!=(double x) const { return std::abs(_value - x) >= EQUAL_PRECISION; }
        bool operator<=(double x) const { return _value <= x; }
        bool operator>=(double x) const { return _value >= x; }
        bool operator<(double x) const { return _value < x; }
        bool operator>(double x) const { return _value > x; }

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

inline ts::Double operator+(double x1, const ts::Double& x2) { return x2 + x1; }
inline ts::Double operator-(double x1, const ts::Double& x2) { return -(x2 - x1); }
inline ts::Double operator*(double x1, const ts::Double& x2) { return x2 * x1; }
inline ts::Double operator/(double x1, const ts::Double& x2) { return ts::Double(x1) / x2; }

inline bool operator==(double x1, const ts::Double& x2) { return x2 == x1; }
inline bool operator!=(double x1, const ts::Double& x2) { return x2 != x1; }
inline bool operator<=(double x1, const ts::Double& x2) { return x2 >= x1; }
inline bool operator>=(double x1, const ts::Double& x2) { return x2 <= x1; }
inline bool operator<(double x1, const ts::Double& x2) { return x2 > x1; }
inline bool operator>(double x1, const ts::Double& x2) { return x2 < x1; }

//! @endcond
