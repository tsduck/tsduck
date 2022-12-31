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
const ts::Integer<INT_T,N> ts::Integer<INT_T,N>::MIN(std::numeric_limits<INT_T>::min());

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
const ts::Integer<INT_T,N> ts::Integer<INT_T,N>::MAX(std::numeric_limits<INT_T>::max());


//----------------------------------------------------------------------------
// Virtual numeric conversions.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
int64_t ts::Integer<INT_T,N>::toInt64() const
{
    return bounded_cast<int64_t>(_value);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
double ts::Integer<INT_T,N>::toDouble() const
{
    return double(_value);
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Integer<INT_T,N>::inRange(int64_t min, int64_t max) const
{
    const int64_t r = bounded_cast<int64_t>(_value);
    return r >= min && r <= max;
}

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
ts::UString ts::Integer<INT_T,N>::description() const
{
    return UString::Format(u"%d-bit %s integer value", {8 * sizeof(int_t), SignedDescription<int_t>()});
}


//----------------------------------------------------------------------------
// Convert the number to a string object.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
ts::UString ts::Integer<INT_T,N>::toString(size_t min_width,
                                           bool right_justified,
                                           UChar separator,
                                           bool force_sign,
                                           size_t decimals,
                                           bool force_decimals,
                                           UChar decimal_dot,
                                           UChar pad) const
{
    return UString::Decimal(_value, min_width, right_justified, separator == CHAR_NULL ? UString() : UString(1, separator), force_sign, pad);
}


//----------------------------------------------------------------------------
// Parse a string and interpret it as a number.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value, int>::type N>
bool ts::Integer<INT_T,N>::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    return str.toInteger(_value, separator == CHAR_NULL ? UString() : UString(1, separator));
}
