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
#include "tsIntegerUtils.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
template <typename INT, const size_t PREC, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* N>
constexpr size_t ts::FixedPoint<INT,PREC,N>::PRECISION;
#endif

template <typename INT, const size_t PREC, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* N>
const INT ts::FixedPoint<INT,PREC,N>::FACTOR = ts::Power10<INT>(PREC);

template <typename INT, const size_t PREC, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* N>
ts::UString ts::FixedPoint<INT,PREC,N>::toString(size_t min_width, bool right_justified, const UString& separator, bool force_sign, bool force_decimals, UChar pad) const
{
    // Format the integral part.
    UString s(UString::Decimal(_value / FACTOR, 0, true, separator, force_sign));

    TS_PUSH_WARNING()
    TS_MSC_NOWARNING(4127) // conditional expression is constant (here PRECISION)
 
    // Format the decimal part.
    const int_t dec = _value % FACTOR;
    if (PRECISION > 0 && (dec != 0 || force_decimals)) {
        s.append(u'.');
        s.append(UString::Decimal(dec, PRECISION, true, UString(), false, u'0'));
        if (!force_decimals) {
            while (s.back() == u'0') {
                s.pop_back();
            }
        }
    }

    TS_POP_WARNING()
        
    // Adjust string width.
    if (s.size() < min_width) {
        if (right_justified) {
            s.insert(0, min_width - s.size(), pad);
        }
        else {
            s.append(min_width - s.size(), pad);
        }
    }
    return s;
}
