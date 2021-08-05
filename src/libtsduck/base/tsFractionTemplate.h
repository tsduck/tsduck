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

#pragma once

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MIN(std::numeric_limits<INT_T>::min(), 1, true);

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::MAX(std::numeric_limits<INT_T>::max(), 1, true);


//----------------------------------------------------------------------------
// Manipulation on fraction sign.
//----------------------------------------------------------------------------

// Unsigned version.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <bool dummy>
struct ts::Fraction<INT_T,N>::SignWrapper<false, dummy> {
    static inline void reduce(INT_T& num, INT_T& den) {}
    static inline INT_T abs(INT_T num) { return num; }
};

// Signed version.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
template <bool dummy>
struct ts::Fraction<INT_T,N>::SignWrapper<true, dummy> {
    static inline void reduce(INT_T& num, INT_T& den) { if (den < 0) { num = -num; den = -den; } }
    static inline INT_T abs(INT_T num) { return num < 0 ? -num : num; }
};

// Public version of abs() on fraction.
template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::abs() const
{
    return Fraction(SignWrapper<std::is_signed<int_t>::value>::abs(_num), _den, true);
}


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
        SignWrapper<std::is_signed<int_t>::value>::reduce(_num, _den);
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
        SignWrapper<std::is_signed<int_t>::value>::reduce(_num, _den);
        if (_den != 1) {
            const int_t gcd = GCD(_num, _den);
            _num /= gcd;
            _den /= gcd;
        }
    }
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
    SignWrapper<std::is_signed<int_t>::value>::reduce(num, den);
    return Fraction(num, den, true);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
typename ts::Fraction<INT_T,N> ts::Fraction<INT_T,N>::operator/(const Fraction& x) const
{
    debug_throw_div_zero(x._num);
    return operator*(Fraction(x._den, x._num, true));
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
// Implementation of interfaces from/to string.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
ts::UString ts::Fraction<INT_T,N>::toString() const
{
    return _den == 1 ? UString::Decimal(_num) : UString::Decimal(_num) + u'/' + UString::Decimal(_den);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Fraction<INT_T,N>::fromString(const UString& str)
{
    const size_t slash = str.find(u'/');
    if (slash == NPOS) {
        _den = 1;
        return str.toInteger(_num, UString::DEFAULT_THOUSANDS_SEPARATOR);
    }
    else {
        if (str.substr(0, slash).toInteger(_num, UString::DEFAULT_THOUSANDS_SEPARATOR) &&
            str.substr(slash + 1).toInteger(_den, UString::DEFAULT_THOUSANDS_SEPARATOR) &&
            _den != 0)
        {
            reduce();
            return true;
        }
        else {
            _den = 1; // enforce != 0
            return false;
        }
    }
}
