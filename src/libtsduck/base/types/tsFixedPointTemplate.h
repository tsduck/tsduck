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
#include "tsUString.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
constexpr size_t ts::FixedPoint<INT_T,PREC,N>::PRECISION;

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
constexpr INT_T ts::FixedPoint<INT_T,PREC,N>::FACTOR;
#endif

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
const ts::FixedPoint<INT_T,PREC,N> ts::FixedPoint<INT_T,PREC,N>::MIN(std::numeric_limits<INT_T>::min(), true);

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
const ts::FixedPoint<INT_T,PREC,N> ts::FixedPoint<INT_T,PREC,N>::MAX(std::numeric_limits<INT_T>::max(), true);


//----------------------------------------------------------------------------
// Virtual numeric conversions.
//----------------------------------------------------------------------------

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
int64_t ts::FixedPoint<INT_T,PREC,N>::toInt64() const
{
    return bounded_cast<int64_t>(_value / FACTOR);
}

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
double ts::FixedPoint<INT_T,PREC,N>::toDouble() const
{
    return double(_value) / FACTOR;
}

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
bool ts::FixedPoint<INT_T,PREC,N>::inRange(int64_t min, int64_t max) const
{
    const int64_t r = bounded_cast<int64_t>(_value / FACTOR);
    return r >= min && r <= max;
}

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
ts::UString ts::FixedPoint<INT_T,PREC,N>::description() const
{
    return UString::Format(u"%d-bit fixed-point value with up to %d decimals", {8 * sizeof(int_t), PRECISION});
}


//----------------------------------------------------------------------------
// Convert the number to a string object.
//----------------------------------------------------------------------------

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
ts::UString ts::FixedPoint<INT_T,PREC,N>::toString(size_t min_width,
                                                   bool right_justified,
                                                   UChar separator,
                                                   bool force_sign,
                                                   size_t decimals,
                                                   bool force_decimals,
                                                   UChar decimal_dot,
                                                   UChar pad) const
{
    UString str(UString::Decimal(_value / FACTOR, 0, true, UString()));
    str.append(u'.');
    str.append(UString::Decimal(std::abs(_value % FACTOR), PRECISION, true, UString(), false, u'0'));
    Format(str, min_width, right_justified, separator, force_sign && !is_negative(_value), decimals == NPOS ? PRECISION : decimals, force_decimals, decimal_dot, pad);
    return str;
}


//----------------------------------------------------------------------------
// Parse a string and interpret it as a number.
//----------------------------------------------------------------------------

template <typename INT_T, const size_t PREC, typename std::enable_if<std::is_integral<INT_T>::value && std::is_signed<INT_T>::value, int>::type N>
bool ts::FixedPoint<INT_T,PREC,N>::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    UString s(str);
    Deformat(s, separator, decimal_dot);
    return s.toInteger(_value, UString(1, separator), PRECISION, UString(1, decimal_dot));
}
