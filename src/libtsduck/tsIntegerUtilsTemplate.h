//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsFormat.h"

//
// In this module, we work on formal integer types INT. We use std::numeric_limits<INT> to test the
// capability of the type (is_signed, etc.) But, for each instantiation of INT, the corresponding
// expression is constant and the Microsoft compiler complains about that. Disable this specific
// warning in the module. Note the #pragma warning(pop) at the end of file.
//
// warning C4127: conditional expression is constant
//
#if defined(TS_MSC)
#pragma warning(push)
#pragma warning(disable:4127)
#endif


//----------------------------------------------------------------------------
// Perform a bounded addition without overflow.
//----------------------------------------------------------------------------

template <typename INT>
INT ts::BoundedAdd(INT a, INT b)
{
    if (std::numeric_limits<INT>::is_signed) {
        // Signed addition.
        const INT c = a + b;
        if (a > 0 && b > 0 && c <= 0) {
            // Overflow.
            return std::numeric_limits<INT>::max();
        }
        else if (a < 0 && b < 0 && c >= 0) {
            // Underflow.
            return std::numeric_limits<INT>::min();
        }
        else {
            return c;
        }
    }
    else {
        // Unsigned addition.
        if (a > std::numeric_limits<INT>::max() - b) {
            // Overflow.
            return std::numeric_limits<INT>::max();
        }
        else {
            return a + b;
        }
    }
}


//----------------------------------------------------------------------------
// Perform a bounded subtraction without overflow.
//----------------------------------------------------------------------------

template <typename INT>
INT ts::BoundedSub(INT a, INT b)
{
    if (std::numeric_limits<INT>::is_signed) {
        // Signed subtraction.
        const INT c = a - b;
        if (a > 0 && b < 0 && c <= 0) {
            // Overflow.
            return std::numeric_limits<INT>::max();
        }
        else if (a < 0 && b > 0 && c >= 0) {
            // Underflow.
            return std::numeric_limits<INT>::min();
        }
        else {
            return c;
        }
    }
    else {
        // Unsigned subtraction.
        if (a < b) {
            // Underflow.
            return 0;
        }
        else {
            return a - b;
        }
    }
}


//----------------------------------------------------------------------------
// Format a percentage string.
//----------------------------------------------------------------------------

template<typename INT>
std::string ts::PercentageString(INT value, INT total)
{
    if (total < 0) {
        return "?";
    }
    if (total == 0) {
        return "0.00%";
    }
    else {
        // Integral percentage
        const int p1 = int((100 * uint64_t(value)) / uint64_t(total));
        // Percentage first 2 decimals
        const int p2 = int(((10000 * uint64_t(value)) / uint64_t(total)) % 100);
        return ts::Format("%d.%02d%%", p1, p2);
    }
}

#if defined(TS_MSC)
#pragma warning(pop)
#endif
