//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
// Perform a bounded addition without overflow.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::BoundedAdd(INT a, INT b)
{
    // Unsigned addition.
    if (a > std::numeric_limits<INT>::max() - b) {
        // Overflow.
        return std::numeric_limits<INT>::max();
    }
    else {
        return a + b;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::BoundedAdd(INT a, INT b)
{
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


//----------------------------------------------------------------------------
// Perform a bounded subtraction without overflow.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::BoundedSub(INT a, INT b)
{
    // Unsigned subtraction.
    if (a < b) {
        // Underflow.
        return 0;
    }
    else {
        return a - b;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::BoundedSub(INT a, INT b)
{
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


//----------------------------------------------------------------------------
// Rounding integers up and down.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::RoundDown(INT x, INT f)
{
    return f == 0 ? x : x - x % f;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::RoundDown(INT x, INT f)
{
    f = INT(std::abs(f));
    return f == 0 ? x : (x >= 0 ? x - x % f : x - (f + x % f) % f);
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::RoundUp(INT x, INT f)
{
    return f == 0 ? x : x + (f - x % f) % f;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::RoundUp(INT x, INT f)
{
    f = INT(std::abs(f));
    return f == 0 ? x : (x >= 0 ? x + (f - x % f) % f : x - x % f);
}


//----------------------------------------------------------------------------
// Perform a sign extension on any subset of a signed integer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::SignExtend(INT x, int bits)
{
    if (bits < 2) {
        // We need at least two bits, one for the sign, one for the value.
        return 0;
    }
    else if (bits >= int(8 * sizeof(x))) {
        // No need to extend, the value is already there.
        return x;
    }
    else {
        // A mask with all one's in MSB unused bits.
        TS_PUSH_WARNING()
        TS_GCC_NOWARNING(shift-negative-value)
        const INT mask = static_cast<INT>(~static_cast<INT>(0) << bits);
        TS_POP_WARNING()

        // Test the sign bit in the LSB signed value.
        return (x & (static_cast<INT>(1) << (bits - 1))) == 0 ? (x & ~mask) : (x | mask);
    }
}
