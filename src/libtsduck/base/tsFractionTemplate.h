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


//----------------------------------------------------------------------------
// Manipulation on fraction sign.
//----------------------------------------------------------------------------

// Unsigned version.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <bool dummy>
struct ts::Fraction<INT,N>::SignWrapper<false, dummy> {
    static inline void reduce(INT& num, INT& den) {}
    static inline INT abs(INT num) { return num; }
};

// Signed version.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <bool dummy>
struct ts::Fraction<INT,N>::SignWrapper<true, dummy> {
    static inline void reduce(INT& num, INT& den) { if (den < 0) { num = -num; den = -den; } }
    static inline INT abs(INT num) { return num < 0 ? -num : num; }
};

// Public version of abs() on fraction.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::abs() const
{
    return Fraction(SignWrapper<std::is_signed<INT>::value>::abs(_num), _den, true);
}


//----------------------------------------------------------------------------
// Reduce a fraction. Internal operation only. Try to optimize usual cases.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
void ts::Fraction<INT,N>::reduce()
{
    if (_num == 0) {
        _den = 1;
    }
    else {
        SignWrapper<std::is_signed<INT>::value>::reduce(_num, _den);
        if (_den != 1) {
            const INT gcd = GCD(_num, _den);
            if (gcd != 1) {
                _num /= gcd;
                _den /= gcd;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Fraction constructors from integers. Try to optimize usual cases.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
ts::Fraction<INT,N>::Fraction(INT1 numerator) :
    _num(INT(numerator)), _den(1)
{
    if (!ts::bound_check<INT>(numerator)) {
        throw std::overflow_error("integer too large for fraction");
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type M>
ts::Fraction<INT,N>::Fraction(INT1 numerator, INT2 denominator) :
    _num(0), _den(1)
{
    if (denominator == 0) {
        throw std::underflow_error("zero denominator in fraction");
    }
    if (numerator != 0) {
        if (!ts::bound_check<INT>(numerator) || !ts::bound_check<INT>(denominator)) {
            throw std::overflow_error("integer too large for fraction");
        }
        _num = INT(numerator);
        _den = INT(denominator);
        SignWrapper<std::is_signed<INT>::value>::reduce(_num, _den);
        if (_den != 1) {
            const INT gcd = GCD(_num, _den);
            if (gcd != 1) {
                _num /= gcd;
                _den /= gcd;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Basic arithmetic operations.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator+(const Fraction& x) const
{
    Fraction res;
    if (_den == x._den) {
        res._num = _num + x._num;
        res._den = _den;
    }
    else {
        res._num = _num * x._den + x._num * _den;
        res._den = _den * x._den;
    }
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator-(const Fraction& x) const
{
    Fraction res;
    if (_den == x._den) {
        res._num = _num - x._num;
        res._den = _den;
    }
    else {
        res._num = _num * x._den - x._num * _den;
        res._den = _den * x._den;
    }
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator*(const Fraction& x) const
{
    Fraction res(_num * x._num, _den * x._den, true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator/(const Fraction& x) const
{
    if (x._num == 0) {
        throw std::underflow_error("divide fraction by zero");
    }
    Fraction res(_num * x._den, _den * x._num, true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator+(INT1 x) const
{
    Fraction res(_num + INT(x) * _den, _den, true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator-(INT1 x) const
{
    Fraction res(_num - INT(x) * _den, _den, true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator*(INT1 x) const
{
    Fraction res(_num * INT(x), _den, true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
template <typename INT1, typename std::enable_if<std::is_integral<INT1>::value, int>::type M>
typename ts::Fraction<INT,N> ts::Fraction<INT,N>::operator/(INT1 x) const
{
    if (x == 0) {
        throw std::underflow_error("divide fraction by zero");
    }
    Fraction res(_num, _den * INT(x), true);
    res.reduce();
    return res;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::Fraction<INT,N>::operator<=(const Fraction& x) const
{
    return (_den == x._den) ? (_num <= x._num) : (_num * x._den <= x._num * _den);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::Fraction<INT,N>::operator>=(const Fraction& x) const
{
    return (_den == x._den) ? (_num >= x._num) : (_num * x._den >= x._num * _den);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::Fraction<INT,N>::operator<(const Fraction& x) const
{
    return (_den == x._den) ? (_num < x._num) : (_num * x._den < x._num * _den);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::Fraction<INT,N>::operator>(const Fraction& x) const
{
    return (_den == x._den) ? (_num > x._num) : (_num * x._den > x._num * _den);
}


//----------------------------------------------------------------------------
// Implementation of interfaces from/to string.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
ts::UString ts::Fraction<INT,N>::toString() const
{
    return _den == 1 ? UString::Decimal(_num) : UString::Decimal(_num) + u'/' + UString::Decimal(_den);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::Fraction<INT,N>::fromString(const UString& str)
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
