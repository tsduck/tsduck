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
//!  Template representation of integer numbers as AbstractNumber.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"
#include "tsIntegerUtils.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Template representation of integer numbers as AbstractNumber.
    //! @ingroup cpp
    //!
    //! @tparam INT_T The underlying integer type.
    //!
    template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type = 0>
    class Integer: public AbstractNumber
    {
    private:
        INT_T _value;

        template <typename INT2, typename std::enable_if<std::is_integral<INT2>::value && std::is_signed<INT2>::value, int>::type = 0>
        static inline UString SignedDescription() { return u"a signed"; }

        template <typename INT2, typename std::enable_if<std::is_integral<INT2>::value && std::is_unsigned<INT2>::value, int>::type = 0>
        static inline UString SignedDescription() { return u"an unsigned"; }

    public:
        //!
        //! The underlying signed integer type.
        //!
        typedef INT_T int_t;

        //!
        //! The minimum representable value of this fixed-point type.
        //!
        static const Integer MIN;

        //!
        //! The maximum representable value of this fixed-point type.
        //!
        static const Integer MAX;

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        Integer() : _value(0) {}

        //!
        //! Constructor.
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] i Initial value.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer(INT2 i) : _value(bounded_cast<int_t>(i)) {}

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
        int_t toInt() const { return _value; }

        //!
        //! Get the absolute value.
        //! @return The absolute value of this fixed-point number.
        //!
        Integer abs() const { return Integer(std::abs(_value)); }

        //!
        //! Get the maximum value of two fixed-point numbers.
        //! @param [in] x Another fixed-point number.
        //! @return The maximum value of this fixed-point number and @a x.
        //!
        Integer max(Integer x) const { return Integer(std::max(_value, x._value)); }

        //!
        //! Get the minimum value of two fixed-point numbers.
        //! @param [in] x Another fixed-point number.
        //! @return The minimum value of this fixed-point number and @a x.
        //!
        Integer min(Integer x) const { return Integer(std::min(_value, x._value)); }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        Integer operator-() const { return Integer(- _value); }
        Integer operator+(Integer x) const { return Integer(_value + x._value); }
        Integer operator-(Integer x) const { return Integer(_value - x._value); }
        Integer operator*(Integer x) const { return Integer(_value * x._value); }
        Integer operator/(Integer x) const { return Integer(_value / x._value); }

        Integer& operator+=(Integer x) { _value += x._value; return *this; }
        Integer& operator-=(Integer x) { _value -= x._value; return *this; }
        Integer& operator*=(Integer x) { _value *= x._value; return *this; }
        Integer& operator/=(Integer x) { _value /= x._value; return *this; }

        bool operator==(Integer x) const { return _value == x._value; }
        bool operator!=(Integer x) const { return _value != x._value; }
        bool operator<=(Integer x) const { return _value <= x._value; }
        bool operator>=(Integer x) const { return _value >= x._value; }
        bool operator<(Integer x) const { return _value < x._value; }
        bool operator>(Integer x) const { return _value > x._value; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer operator+(INT2 x) const { return Integer(_value + int_t(x)); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer operator-(INT2 x) const { return Integer(_value - int_t(x)); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer operator*(INT2 x) const { return Integer(_value * int_t(x)); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer operator/(INT2 x) const { return Integer(_value / int_t(x)); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer& operator+=(INT2 x) { _value += int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer& operator-=(INT2 x) { _value -= int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer& operator*=(INT2 x) { _value *= int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        Integer& operator/=(INT2 x) { _value /= int_t(x); return *this; }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator==(INT2 x) const { return _value == int_t(x); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator!=(INT2 x) const { return _value != int_t(x); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<=(INT2 x) const { return _value <= int_t(x); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>=(INT2 x) const { return _value >= int_t(x); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator<(INT2 x) const { return _value < int_t(x); }

        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool operator>(INT2 x) const { return _value > int_t(x); }

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline ts::Integer<INT2> operator+(INT1 x1, ts::Integer<INT2> x2) { return x2 + x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline ts::Integer<INT2> operator-(INT1 x1, ts::Integer<INT2> x2) { return ts::Integer<INT2>(x1) - x2; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline ts::Integer<INT2> operator*(INT1 x1, ts::Integer<INT2> x2) { return x2 * x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline ts::Integer<INT2> operator/(INT1 x1, ts::Integer<INT2> x2) { return ts::Integer<INT2>(x1) / x2; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator==(INT1 x1, ts::Integer<INT2> x2) { return x2 == x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator!=(INT1 x1, ts::Integer<INT2> x2) { return x2 != x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator<=(INT1 x1, ts::Integer<INT2> x2) { return x2 >= x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator>=(INT1 x1, ts::Integer<INT2> x2) { return x2 <= x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator<(INT1 x1, ts::Integer<INT2> x2) { return x2 > x1; }

template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
inline bool operator>(INT1 x1, ts::Integer<INT2> x2) { return x2 < x1; }

//! @endcond

#include "tsIntegerTemplate.h"
