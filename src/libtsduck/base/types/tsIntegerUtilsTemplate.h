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
#include <type_traits>

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
template <typename INT> constexpr INT ts::static_power10<INT,0>::value;
template <typename INT> constexpr INT ts::static_power10<INT,1>::value;
template <typename INT> constexpr INT ts::static_power10<INT,2>::value;
template <typename INT> constexpr INT ts::static_power10<INT,3>::value;
template <typename INT> constexpr INT ts::static_power10<INT,4>::value;
template <typename INT> constexpr INT ts::static_power10<INT,5>::value;
template <typename INT> constexpr INT ts::static_power10<INT,6>::value;
template <typename INT> constexpr INT ts::static_power10<INT,7>::value;
template <typename INT> constexpr INT ts::static_power10<INT,8>::value;
template <typename INT> constexpr INT ts::static_power10<INT,9>::value;
template <typename INT> constexpr INT ts::static_power10<INT,10>::value;
template <typename INT> constexpr INT ts::static_power10<INT,11>::value;
template <typename INT> constexpr INT ts::static_power10<INT,12>::value;
template <typename INT> constexpr INT ts::static_power10<INT,13>::value;
template <typename INT> constexpr INT ts::static_power10<INT,14>::value;
template <typename INT> constexpr INT ts::static_power10<INT,15>::value;
template <typename INT> constexpr INT ts::static_power10<INT,16>::value;
template <typename INT> constexpr INT ts::static_power10<INT,17>::value;
template <typename INT> constexpr INT ts::static_power10<INT,18>::value;
template <typename INT> constexpr INT ts::static_power10<INT,19>::value;
#endif


//----------------------------------------------------------------------------
// Not inlined to avoid optimization which breaks the code.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::add_overflow(INT a, INT b)
{
    INT res = a + b;
    return add_overflow<INT>(a, b, res);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::sub_overflow(INT a, INT b)
{
    INT res = a - b;
    return sub_overflow<INT>(a, b, res);
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type N>
bool ts::mul_overflow(INT a, INT b)
{
    INT res = a * b;
    return mul_overflow<INT>(a, b, res);
}


//----------------------------------------------------------------------------
// Perform a bounded addition without overflow.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::bounded_add(INT a, INT b)
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
INT ts::bounded_add(INT a, INT b)
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
INT ts::bounded_sub(INT a, INT b)
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
INT ts::bounded_sub(INT a, INT b)
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
INT ts::round_down(INT x, INT f)
{
    return f == 0 ? x : x - x % f;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::round_down(INT x, INT f)
{
    f = INT(std::abs(f));
    return f == 0 ? x : (x >= 0 ? x - x % f : x - (f + x % f) % f);
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::round_up(INT x, INT f)
{
    return f == 0 ? x : x + (f - x % f) % f;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::round_up(INT x, INT f)
{
    f = INT(std::abs(f));
    return f == 0 ? x : (x >= 0 ? x + (f - x % f) % f : x - x % f);
}


//----------------------------------------------------------------------------
// Perform a sign extension on any subset of a signed integer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::SignExtend(INT x, size_t bits)
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
        TS_LLVM_NOWARNING(shift-sign-overflow)
        const INT mask = static_cast<INT>(~static_cast<INT>(0) << bits);
        TS_POP_WARNING()

        // Test the sign bit in the LSB signed value.
        return (x & (static_cast<INT>(1) << (bits - 1))) == 0 ? (x & ~mask) : (x | mask);
    }
}


//----------------------------------------------------------------------------
// Get the size in bits of an unsigned integer value.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
size_t ts::BitSize(INT x)
{
    size_t size = 1;
    const size_t maxbit = 8 * sizeof(INT);
    for (size_t bit = 0; bit < maxbit && (x = x >> 1) != 0; ++bit) {
        ++size;
    }
    return size;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
size_t ts::BitSize(INT x)
{
    typedef typename std::make_unsigned<INT>::type UNS_INT;
    return BitSize<UNS_INT>(UNS_INT(x));
}

//----------------------------------------------------------------------------
// Compute a greatest common denominator (GCD).
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
INT ts::GCD(INT x, INT y)
{
    INT z;
    while (y != 0) {
        z = x % y;
        x = y;
        y = z;
    }
    return x;
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
INT ts::GCD(INT x, INT y)
{
    INT z;
    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    while (y != 0) {
        z = x % y;
        x = y;
        y = z;
    }
    return x;
}
