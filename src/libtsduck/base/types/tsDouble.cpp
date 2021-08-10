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

#include "tsDouble.h"
#include "tsUString.h"
TSDUCK_SOURCE;

const ts::Double ts::Double::MIN(std::numeric_limits<float_t>::lowest());
const ts::Double ts::Double::MAX(std::numeric_limits<float_t>::max());

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr double ts::Double::EQUAL_PRECISION;
#endif


//----------------------------------------------------------------------------
// Virtual numeric conversions.
//----------------------------------------------------------------------------

int64_t ts::Double::toInt64() const
{
    return int64_t(std::round(_value));
}

double ts::Double::toDouble() const
{
    return _value;
}

bool ts::Double::inRange(int64_t min, int64_t max) const
{
    return _value >= double(min) && _value <= double(max);
}

ts::UString ts::Double::description() const
{
    return u"a floating-point value with an optional decimal part";
}


//----------------------------------------------------------------------------
// Convert the number to a string object.
//----------------------------------------------------------------------------

ts::UString ts::Double::toString(size_t min_width,
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
        decimals = 6;
    }

    // Format the floating point number in a slightly oversized UTF-8 buffer.
    std::string str8(std::numeric_limits<float_t>::max_digits10 + decimals + 10, CHAR_NULL);
    std::snprintf(&str8[0], str8.length() - 1, "%.*f", int(decimals), _value);

    // Work on UString from now on.
    UString str;
    str.assignFromUTF8(str8.c_str());
    Format(str, min_width, right_justified, separator, force_sign && _value >= 0,  decimals, force_decimals, decimal_dot, pad);
    return str;
}


//----------------------------------------------------------------------------
// Parse a string and interpret it as a number.
//----------------------------------------------------------------------------

bool ts::Double::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    UString str16(str);
    Deformat(str16, separator, decimal_dot);
    const std::string str8(str16.toUTF8());

    int len = 0;
    const int count = std::sscanf(str8.c_str(), "%lf%n", &_value, &len);
    return count == 1 && len == int(str8.length());
}
