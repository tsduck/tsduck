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
//!  Template representation of fractional numbers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"
#include "tsIntegerUtils.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Template representation of fractional numbers.
    //! @ingroup cpp
    //!
    //! Fraction are always reduced so that the numerator and denominator have no common divisor other than 1.
    //! For signed integer types, the signed is carried by the numerator and the denominator is always positive.
    //!
    //! All arithmetic and comparison operators are defined between fraction values
    //! and between a fraction value and an integer value, both directions.
    //!
    //! In debug mode (when the macro @c DEBUG is defined), the arithmetic operators throw an exception
    //! in case of overflow. Without debug mode, the arithmetic overflow are ignored, which may lead to
    //! inconsistent values after a large number of operations (typically additions and substractions with
    //! distinct denominators).
    //!
    //! @tparam INT_T The integer type for numerator and denominator.
    //!
    template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type = 0>
    class Fraction: public AbstractNumber
    {
    private:
        // Numerator and denominator. Always reduced. Only _num can be negative.
        INT_T _num, _den;

        // Fast private unchecked constructor. The parameters must be valid.
        // The dummy parameter is here to disambiguate with the public constructor.
        Fraction(INT_T num, INT_T den, bool dummy) : _num(num), _den(den) {}

        // Reduce the fraction.
        void reduce();

        // Manipulation on fraction sign: different versions for signed and unsigned integers.
        // This structure looks hacky (and it is) but it is the only way to have valid C++
        // code which is correctly handled by gcc, clang and msvc.
        template <bool is_signed, bool dummy = true>
        struct SignWrapper {
            // Reduce the sign of elements of a fraction.
            static inline void reduce(INT_T& num, INT_T& den);
            // Absolute value of the numerator.
            static inline INT_T abs(INT_T num);
        };

    public:
        //!
        //! The underlying integer type.
        //!
        typedef INT_T int_t;

        //!
        //! The minimum representable value of this fraction type.
        //!
        static const Fraction MIN;

        //!
        //! The maximum representable value of this fraction type.
        //!
        static const Fraction MAX;

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        Fraction() : _num(0), _den(1) {}

        //!
        //! Constructor from an integer value.
        //! @tparam INT1 Some other integer type (signed or unsigned).
        //! @param [in] numerator Initial numerator value.
        //! @throw std::overflow_error When @a numerator is out of range.
        //!
        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction(INT1 numerator);

        //!
        //! Constructor from integer values.
        //! @tparam INT1 Some other integer type (signed or unsigned).
        //! @tparam INT2 Some other integer type (signed or unsigned).
        //! @param [in] numerator Initial numerator value.
        //! @param [in] denominator Initial denominator value.
        //! @throw std::underflow_error When @a denominator is zero.
        //! @throw std::overflow_error When @a numerator or @a denominator are out of range.
        //!
        template<typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
        Fraction(INT1 numerator, INT2 denominator);

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
        //! Get the numerator part of the fraction.
        //! @return The numerator part.
        //!
        int_t numerator() const { return _num; }

        //!
        //! Get the denominator part of the fraction.
        //! @return The denominator part.
        //!
        int_t denominator() const { return _den; }

        //!
        //! Conversion to integral value.
        //! @return The value in integral units. Underflow or overflow rounding is applied when necessary.
        //!
        int_t toInt() const { return _num / _den; }

        //!
        //! Get the absolute value.
        //! @return The absolute value of this fraction number.
        //!
        Fraction abs() const;

        //!
        //! Converts to a proper fraction (a fraction that is less than 1).
        //! The previous integer part is returned. This means that:
        //! @code
        //!   typedef Fraction<...> Frac;
        //!   Frac f(..);
        //!   Frac old(f);
        //!   Frac::int_t i = f.proper();
        //!   // then old == i + f, with f.abs() < 1
        //! @endcode
        //! @returns The previous integer part of the fraction.
        //!
        int_t proper();

        //!
        //! Get the maximum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The maximum value of this fraction and @a x.
        //!
        Fraction max(const Fraction& x) const { return *this >= x ? *this : x; }

        //!
        //! Get the minimum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The minimum value of this fraction and @a x.
        //!
        Fraction min(const Fraction& x) const { return *this <= x ? *this : x; }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        Fraction operator-() const;
        Fraction operator+(const Fraction& x) const;
        Fraction operator-(const Fraction& x) const;
        Fraction operator*(const Fraction& x) const;
        Fraction operator/(const Fraction& x) const;

        Fraction& operator+=(const Fraction& x) { return *this = *this + x; }
        Fraction& operator-=(const Fraction& x) { return *this = *this - x; }
        Fraction& operator*=(const Fraction& x) { return *this = *this * x; }
        Fraction& operator/=(const Fraction& x) { return *this = *this / x; }

        bool operator==(const Fraction& x) const { return _num == x._num && _den == x._den; }
        bool operator!=(const Fraction& x) const { return _num != x._num || _den != x._den; }
        bool operator<=(const Fraction& x) const;
        bool operator>=(const Fraction& x) const;
        bool operator<(const Fraction& x) const;
        bool operator>(const Fraction& x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction operator+(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction operator-(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction operator*(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction operator/(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction& operator+=(INT1 x) { return *this = *this + x; }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction& operator-=(INT1 x) { return *this = *this - x; }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction& operator*=(INT1 x) { return *this = *this * x; }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        Fraction& operator/=(INT1 x) { return *this = *this / x; }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator==(INT1 x) const { return _den == 1 && bound_check<int_t>(x) && _num == int_t(x); }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator!=(INT1 x) const { return _den != 1 || !bound_check<int_t>(x) || _num != int_t(x); }

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator<=(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator>=(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator<(INT1 x) const;

        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator>(INT1 x) const;

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline ts::Fraction<INT2> operator+(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 + x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline ts::Fraction<INT2> operator-(INT1 x1, const ts::Fraction<INT2>& x2) { return -(x2 - x1); }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline ts::Fraction<INT2> operator*(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 * x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline ts::Fraction<INT2> operator/(INT1 x1, const ts::Fraction<INT2>& x2) { return ts::Fraction<INT2>(x1) / x2; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator==(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 == x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator!=(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 != x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator<=(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 >= x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator>=(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 <= x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator<(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 > x1; }

template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator>(INT1 x1, const ts::Fraction<INT2>& x2) { return x2 < x1; }

//! @endcond

#include "tsFractionTemplate.h"
