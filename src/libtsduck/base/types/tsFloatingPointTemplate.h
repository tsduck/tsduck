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
#include "tsIntegerUtils.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
constexpr FLOAT_T ts::FloatingPoint<FLOAT_T,PREC,N>::EQUAL_PRECISION;
#endif

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
const ts::FloatingPoint<FLOAT_T,PREC,N> ts::FloatingPoint<FLOAT_T,PREC,N>::MIN(std::numeric_limits<FLOAT_T>::lowest());

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
const ts::FloatingPoint<FLOAT_T,PREC,N> ts::FloatingPoint<FLOAT_T,PREC,N>::MAX(std::numeric_limits<FLOAT_T>::max());


//----------------------------------------------------------------------------
// Virtual numeric conversions.
//----------------------------------------------------------------------------

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
int64_t ts::FloatingPoint<FLOAT_T,PREC,N>::toInt64() const
{
    return int64_t(std::round(_value));
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
double ts::FloatingPoint<FLOAT_T,PREC,N>::toDouble() const
{
    return double(_value);
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
bool ts::FloatingPoint<FLOAT_T,PREC,N>::inRange(int64_t min, int64_t max) const
{
    return _value >= float_t(min) && _value <= float_t(max);
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
ts::UString ts::FloatingPoint<FLOAT_T,PREC,N>::description() const
{
    return UString::Format(u"%d-bit floating-point value", {8 * sizeof(float_t)});
}


//----------------------------------------------------------------------------
// Convert the number to a string object.
//----------------------------------------------------------------------------

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
ts::UString ts::FloatingPoint<FLOAT_T,PREC,N>::toString(size_t min_width,
                                                        bool right_justified,
                                                        UChar separator,
                                                        bool force_sign,
                                                        size_t decimals,
                                                        bool force_decimals,
                                                        UChar decimal_dot,
                                                        UChar pad) const
{
    // 6 decimal digits by default.
    if (decimals == NPOS) {
        decimals = DISPLAY_PRECISION;
    }

    // Format the floating point number in a slightly oversized UTF-8 buffer.
    std::string str8(std::numeric_limits<float_t>::max_digits10 + decimals + 10, '\0');
    std::snprintf(&str8[0], str8.length() - 1, "%.*lf", int(decimals), double(_value));

    // Work on UString from now on.
    UString str;
    str.assignFromUTF8(str8.c_str());
    Format(str, min_width, right_justified, separator, force_sign && _value >= 0,  decimals, force_decimals, decimal_dot, pad);
    return str;
}


//----------------------------------------------------------------------------
// Parse a string and interpret it as a number.
//----------------------------------------------------------------------------

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
bool ts::FloatingPoint<FLOAT_T,PREC,N>::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    UString str16(str);
    Deformat(str16, separator, decimal_dot);
    const std::string str8(str16.toUTF8());

    int len = 0;
    double d = 0.0;
    TS_PUSH_WARNING()
    TS_MSC_NOWARNING(4996) // 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead.
    const int count = std::sscanf(str8.c_str(), "%lf%n", &d, &len);
    TS_POP_WARNING()
    _value = float_t(d);
    return count == 1 && len == int(str8.length());
}
