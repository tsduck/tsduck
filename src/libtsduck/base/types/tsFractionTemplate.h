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

#pragma once

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MIN(std::numeric_limits<INT_T>::min(), 1, true);

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MAX(std::numeric_limits<INT_T>::max(), 1, true);


//----------------------------------------------------------------------------
// Reduce a fraction. Internal operation only. Try to optimize usual cases.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Fraction constructors from integers. Try to optimize usual cases.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
ts::Fraction<INT_T,N>::Fraction(INT1 numerator) :
    _num(int_t(numerator)), _den(1)
{
    debug_throw_bound_check<int_t>(numerator);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type M>
ts::Fraction<INT_T,N>::Fraction(INT1 numerator, INT2 denominator) :
    _num(0), _den(1)
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


//----------------------------------------------------------------------------
// Virtual numeric conversions.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Converts to a proper fraction (a fraction that is less than 1).
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N>::int_t ts::Fraction<INT_T,N>::proper()
{
    const int_t result = _num / _den;
    _num = _num % _den;
    return result;
}


//----------------------------------------------------------------------------
// Basic arithmetic operations.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Comparison operations.
//----------------------------------------------------------------------------

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
    template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>   \
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

//----------------------------------------------------------------------------
// Convert the fraction to a string object.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Parse a string and interpret it as a fraction.
//----------------------------------------------------------------------------

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
