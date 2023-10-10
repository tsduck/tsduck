//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        INT_T _num = 0;
        INT_T _den = 1;

        // Fast private unchecked constructor. The parameters must be valid.
        // The dummy parameter is here to disambiguate with the public constructor.
        Fraction(INT_T num, INT_T den, bool dummy) : _num(num), _den(den) {}

        // Reduce the fraction.
        void reduce();

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
        Fraction() = default;

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
        Fraction abs() const { return Fraction(ts::abs(_num), _den, true); }

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

        //!
        //! Check if this Fraction generates an overflow when multiplied by an integer.
        //! @tparam INT2 Another integer type.
        //! @param [in] x An integer of type @a INT2.
        //! @return True if this Fraction generates an overflow when multiplied by @a x.
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool mulOverflow(INT2 x) const;

        //!
        //! Check if this Fraction generates an overflow when multiplied by another Fraction.
        //! @param [in] x Another Fraction.
        //! @return True if this Fraction generates an overflow when multiplied by @a x.
        //!
        bool mulOverflow(const Fraction& x) const;

        //!
        //! Check if this Fraction generates an overflow when divided by another Fraction.
        //! @param [in] x Another Fraction.
        //! @return True if this Fraction generates an overflow when divided by @a x.
        //!
        bool divOverflow(const Fraction& x) const;

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
        TS_UNEQUAL_OPERATOR(Fraction)

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

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0>
        bool operator!=(INT1 x) const { return _den != 1 || !bound_check<int_t>(x) || _num != int_t(x); }
#endif

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

// The operators are not extensively documented with doxygen (obvious, verbose and redundant).
#if !defined(DOXYGEN)

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
inline bool operator==(INT1 x1, const ts::Fraction<INT2>& x2) { return x2.operator==(x1); }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template <typename INT1, typename INT2,
          typename std::enable_if<std::is_integral<INT1>::value, int>::type = 0,
          typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
inline bool operator!=(INT1 x1, const ts::Fraction<INT2>& x2) { return !x2.operator==(x1); }
#endif

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
#endif // DOXYGEN


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MIN(std::numeric_limits<INT_T>::min(), 1, true);

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MAX(std::numeric_limits<INT_T>::max(), 1, true);

// Reduce a fraction. Internal operation only. Try to optimize usual cases.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
void ts::Fraction<INT_T,N>::reduce()
{
    if (_num == 0) {
        _den = 1;
    }
    else {
        sign_reduce(_num, _den);
        if (_den != 1) {
            const int_t gcd = GCD(_num, _den);
            _num /= gcd;
            _den /= gcd;
        }
    }
}

// Fraction constructors from integers. Try to optimize usual cases.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
ts::Fraction<INT_T,N>::Fraction(INT1 numerator) :
    _num(int_t(numerator))
{
    debug_throw_bound_check<int_t>(numerator);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type M>
ts::Fraction<INT_T,N>::Fraction(INT1 numerator, INT2 denominator)
{
    debug_throw_div_zero(denominator);
    if (numerator != 0) {
        debug_throw_bound_check<int_t>(numerator);
        debug_throw_bound_check<int_t>(denominator);
        _num = int_t(numerator);
        _den = int_t(denominator);
        sign_reduce(_num, _den);
        if (_den != 1) {
            const int_t gcd = GCD(_num, _den);
            _num /= gcd;
            _den /= gcd;
        }
    }
}

// Virtual numeric conversions.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
int64_t ts::Fraction<INT_T,N>::toInt64() const
{
    return bounded_cast<int64_t>(_num / _den);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
double ts::Fraction<INT_T,N>::toDouble() const
{
    return double(_num) / double(_den);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Fraction<INT_T,N>::inRange(int64_t min, int64_t max) const
{
    const int64_t r = bounded_cast<int64_t>(_num / _den);
    return r >= min && r <= max;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
ts::UString ts::Fraction<INT_T,N>::description() const
{
    return UString::Format(u"fraction of two %d-bit integers", {8 * sizeof(int_t)});
}

// Converts to a proper fraction (a fraction that is less than 1).
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N>::int_t ts::Fraction<INT_T,N>::proper()
{
    const int_t result = _num / _den;
    _num = _num % _den;
    return result;
}

// Basic arithmetic operations.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator-() const
{
    debug_thow_neg_overflow<int_t>(_num);
    return Fraction(-_num, _den, true);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator+(const Fraction& x) const
{
    Fraction res;
    if (_den == x._den) {
        res._num = _num + x._num;
        res._den = _den;
        debug_throw_add_overflow(_num, x._num, res._num);
    }
    else {
        const int_t num1 = _num * x._den;
        const int_t num2 = x._num * _den;
        res._num = num1 + num2;
        res._den = _den * x._den;
        debug_throw_mul_overflow(_num, x._den, num1);
        debug_throw_mul_overflow(x._num, _den, num2);
        debug_throw_add_overflow(num1, num2, res._num);
        debug_throw_mul_overflow(_den, x._den, res._den);
    }
    res.reduce();
    return res;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator-(const Fraction& x) const
{
    Fraction res;
    if (_den == x._den) {
        res._num = _num - x._num;
        res._den = _den;
        debug_throw_sub_overflow(_num, x._num, res._num);
    }
    else {
        const int_t num1 = _num * x._den;
        const int_t num2 = x._num * _den;
        res._num = num1 - num2;
        res._den = _den * x._den;
        debug_throw_mul_overflow(_num, x._den, num1);
        debug_throw_mul_overflow(x._num, _den, num2);
        debug_throw_sub_overflow(num1, num2, res._num);
        debug_throw_mul_overflow(_den, x._den, res._den);
    }
    res.reduce();
    return res;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator*(const Fraction& x) const
{
    // Cross-reduce first to limit the risk of overflow.
    int_t gcd = GCD(_num, x._den);
    const int_t num1 = _num / gcd;
    const int_t den1 = x._den / gcd;
    gcd = GCD(x._num, _den);
    const int_t num2 = x._num / gcd;
    const int_t den2 = _den / gcd;
    // There is no need to reduce the result again since the original fractions were reduced.
    // Just do zero and sign cleanup.
    int_t num = num1 * num2;
    debug_throw_mul_overflow(num1, num2, num);
    int_t den = 1;
    if (num != 0) {
        den = den1 * den2;
        debug_throw_mul_overflow(den1, den2, den);
    }
    sign_reduce(num, den);
    return Fraction(num, den, true);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Fraction<INT_T,N>::mulOverflow(const Fraction& x) const
{
    // Cross-reduce first to limit the risk of overflow.
    int_t gcd = GCD(_num, x._den);
    const int_t num1 = _num / gcd;
    const int_t den1 = x._den / gcd;
    gcd = GCD(x._num, _den);
    const int_t num2 = x._num / gcd;
    const int_t den2 = _den / gcd;
    return mul_overflow(num1, num2) || mul_overflow(den1, den2);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator/(const Fraction& x) const
{
    debug_throw_div_zero(x._num);
    return operator*(Fraction(x._den, x._num, true));
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Fraction<INT_T,N>::divOverflow(const Fraction& x) const
{
    return x._num == 0 || mulOverflow(Fraction(x._den, x._num, true));
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator+(INT1 x) const
{
    const int_t num1 = int_t(x) * _den;
    Fraction res(_num + num1, _den, true);
    debug_throw_bound_check<int_t>(x);
    debug_throw_mul_overflow(int_t(x), _den, num1);
    debug_throw_add_overflow(_num, num1, res._num);
    res.reduce();
    return res;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator-(INT1 x) const
{
    const int_t num1 = int_t(x) * _den;
    Fraction res(_num - num1, _den, true);
    debug_throw_bound_check<int_t>(x);
    debug_throw_mul_overflow(int_t(x), _den, num1);
    debug_throw_sub_overflow(_num, num1, res._num);
    res.reduce();
    return res;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator*(INT1 x) const
{
    Fraction res(_num * int_t(x), _den, true);
    debug_throw_bound_check<int_t>(x);
    debug_throw_mul_overflow(_num, int_t(x), res._num);
    res.reduce();
    return res;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template<typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
bool ts::Fraction<INT_T,N>::mulOverflow(INT1 x) const
{
    return !bound_check<int_t>(x) || mul_overflow(_num, int_t(x));
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator/(INT1 x) const
{
    Fraction res(_num, _den * int_t(x), true);
    debug_throw_div_zero(x);
    debug_throw_bound_check<int_t>(x);
    debug_throw_mul_overflow(_den, int_t(x), res._den);
    res.reduce();
    return res;
}

// Comparison operations.
#define _TS_FRACTION_COMPARE(op)                       \
    template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N> \
    bool ts::Fraction<INT_T,N>::operator op(const Fraction& x) const \
    {                                                  \
        if (_den == x._den) {                          \
            return _num op x._num;                     \
        }                                              \
        else {                                         \
            const int_t a = _num * x._den;             \
            const int_t b = x._num * _den;             \
            debug_throw_mul_overflow(_num, x._den, a); \
            debug_throw_mul_overflow(x._num, _den, b); \
            return a op b;                             \
        }                                              \
    }

_TS_FRACTION_COMPARE(<=)
_TS_FRACTION_COMPARE(>=)
_TS_FRACTION_COMPARE(<)
_TS_FRACTION_COMPARE(>)

#undef _TS_FRACTION_COMPARE

#define _TS_FRACTION_COMPARE(op)                          \
    template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N> \
    template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M> \
    bool ts::Fraction<INT_T,N>::operator op(INT1 x) const \
    {                                                     \
        debug_throw_bound_check<int_t>(x);                \
        const int_t num1 = int_t(x) * _den;               \
        debug_throw_mul_overflow(int_t(x), _den, num1);   \
        return _num op num1;                              \
    }

_TS_FRACTION_COMPARE(<=)
_TS_FRACTION_COMPARE(>=)
_TS_FRACTION_COMPARE(<)
_TS_FRACTION_COMPARE(>)

#undef _TS_FRACTION_COMPARE

// Convert the fraction to a string object.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
ts::UString ts::Fraction<INT_T,N>::toString(size_t min_width,
                                             bool right_justified,
                                             UChar separator,
                                             bool force_sign,
                                             size_t decimals,
                                             bool force_decimals,
                                             UChar decimal_dot,
                                             UChar pad) const
{
    UString sep;
    if (separator != CHAR_NULL) {
        sep.append(separator);
    }
    UString str(UString::Decimal(_num, 0, true, sep, force_sign));
    if (_den != 1) {
        str.append(u'/');
        str.append(UString::Decimal(_den, 0, true, sep));
    }
    const size_t str_len = str.length();
    if (str_len < min_width) {
        str.insert(right_justified ? 0 : str_len, min_width - str_len, pad);
    }
    return str;
}

// Parse a string and interpret it as a fraction.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Fraction<INT_T,N>::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    const UString sep(1, separator);
    const size_t slash = str.find(u'/');
    if (slash == NPOS) {
        _den = 1;
        return str.toInteger(_num, sep);
    }
    else {
        if (str.substr(0, slash).toInteger(_num, sep) && str.substr(slash + 1).toInteger(_den, sep) && _den != 0) {
            reduce();
            return true;
        }
        else {
            _den = 1; // enforce != 0
            return false;
        }
    }
}

#endif // DOXYGEN
