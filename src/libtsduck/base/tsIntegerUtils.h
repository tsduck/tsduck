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
//!
//!  @file
//!  @ingroup cpp
//!  Some utilities on integers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //
    // Implementation tools for make_signed.
    //
    //! @cond nodoxygen
    template<typename T, size_t SIZE, bool ISSIGNED> struct make_signed_impl { typedef T type; };
    template<> struct make_signed_impl<bool, 1, false> { typedef int8_t type; };
    template<typename T> struct make_signed_impl<T, 1, false> { typedef int16_t type; };
    template<typename T> struct make_signed_impl<T, 2, false> { typedef int32_t type; };
    template<typename T> struct make_signed_impl<T, 4, false> { typedef int64_t type; };
    template<typename T> struct make_signed_impl<T, 8, false> { typedef int64_t type; };
    //! @endcond

    //!
    //! The meta-type ts::make_signed is a generalization of std::make_signed which works on floating point-types as well.
    //! The signed type of a floating-point type or a signed integer type is the type itself.
    //! The signed type of an unsigned integer type is the signed type with the immediately larger size.
    //! @tparam T An integral or floating-point type.
    //!
    template<typename T>
    struct make_signed {
        //! The equivalent signed type.
        typedef typename make_signed_impl<T, sizeof(T), std::is_signed<T>::value>::type type;
    };

    //!
    //! Perform a bounded addition without overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return The value @a a + @a b. The value is @e bounded, in
    //! case of underflow or overflow, the result is the min or max
    //! value of the type, respectively.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT BoundedAdd(INT a, INT b); // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT BoundedAdd(INT a, INT b); // signed version
    //! @endcond

    //!
    //! Perform a bounded subtraction without overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return The value @a a - @a b. The value is @e bounded, in
    //! case of underflow or overflow, the result is the min or max
    //! value of the type, respectively.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT BoundedSub(INT a, INT b); // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT BoundedSub(INT a, INT b); // signed version
    //! @endcond

    //!
    //! Round @a x down to previous multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded down to previous multiple of @a f.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT RoundDown(INT x, INT f); // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT RoundDown(INT x, INT f); // signed version
    //! @endcond

    //!
    //! Round @a x up to next multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded up to next multiple of @a f.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT RoundUp(INT x, INT f); // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT RoundUp(INT x, INT f); // signed version
    //! @endcond

    //!
    //! Perform a sign extension on any subset of a signed integer.
    //!
    //! @tparam INT A signed integer type.
    //! @param [in] x An integer containing a signed value in some number of LSB.
    //! @param [in] bits Number of least significant bits containing a signed value.
    //! @return A signed integer containing the same signed value with proper sign extension on the full size of INT.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT SignExtend(INT x, size_t bits);

    //!
    //! Compute the maximum width of the decimal representation of an integer type.
    //! @param [in] typeSize Size of the integer type in bytes (result of @c sizeof).
    //! @param [in] digitSeparatorSize Size in characters of the digit-grouping separator.
    //! @return The maximum width in characters.
    //!
    size_t MaxDecimalWidth(size_t typeSize, size_t digitSeparatorSize = 0);

    //!
    //! Compute the maximum width of the hexadecimal representation of an integer type.
    //! @param [in] typeSize Size of the integer type in bytes (result of @c sizeof).
    //! @param [in] digitSeparatorSize Size in characters of the digit-grouping separator.
    //! @return The maximum width in characters.
    //!
    size_t MaxHexaWidth(size_t typeSize, size_t digitSeparatorSize = 0);

    //!
    //! Get the size in bits of an integer value.
    //! This is the minimum number of bits to represent the value up to its most-significant '1' bit.
    //!
    //! @tparam INT An integer type.
    //! @param [in] x An integer containing a signed value in some number of LSB.
    //! @return The minimum number of bits to represent the value up to its most-significant '1' bit.
    //! This is never zero, at least one bit is needed to represent the value zero.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    size_t BitSize(INT x); // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    size_t BitSize(INT x); // signed version
    //! @endcond

    //!
    //! Get a power of 10 using a fast lookup table.
    //!
    //! @tparam INT An integer type.
    //! @param [in] pow The requested power of 10.
    //! @return The requested power of 10. If the value is larger than the largest integer on
    //! this platform, the result is undefined.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    inline INT Power10(size_t pow) { return static_cast<INT>(Power10<uint64_t>(pow)); }

    //! @cond nodoxygen
    // Template specialization.
    template<> TSDUCKDLL uint64_t Power10<uint64_t>(size_t pow);
    //! @endcond

    //!
    //! Static values of power of 10.
    //! @tparam POW The exponent of 10.
    //! @tparam INT The integer type for the power of 10.
    //!
    template <typename INT, const size_t POW>
    struct static_power10 {
        #if defined (DOXYGEN)
        static constexpr INT value;   //!< Value of 10 power POW.
        #endif
    };

    //! @cond nodoxygen
    // Template specializations.
    template <typename INT> struct static_power10<INT,  0> { static constexpr INT value = 1; };
    template <typename INT> struct static_power10<INT,  1> { static constexpr INT value = 10; };
    template <typename INT> struct static_power10<INT,  2> { static constexpr INT value = 100; };
    template <typename INT> struct static_power10<INT,  3> { static constexpr INT value = 1000; };
    template <typename INT> struct static_power10<INT,  4> { static constexpr INT value = 10000; };
    template <typename INT> struct static_power10<INT,  5> { static constexpr INT value = 100000; };
    template <typename INT> struct static_power10<INT,  6> { static constexpr INT value = 1000000; };
    template <typename INT> struct static_power10<INT,  7> { static constexpr INT value = 10000000; };
    template <typename INT> struct static_power10<INT,  8> { static constexpr INT value = 100000000; };
    template <typename INT> struct static_power10<INT,  9> { static constexpr INT value = 1000000000; };
    template <typename INT> struct static_power10<INT, 10> { static constexpr INT value = TS_UCONST64(10000000000); };
    template <typename INT> struct static_power10<INT, 11> { static constexpr INT value = TS_UCONST64(100000000000); };
    template <typename INT> struct static_power10<INT, 12> { static constexpr INT value = TS_UCONST64(1000000000000); };
    template <typename INT> struct static_power10<INT, 13> { static constexpr INT value = TS_UCONST64(10000000000000); };
    template <typename INT> struct static_power10<INT, 14> { static constexpr INT value = TS_UCONST64(100000000000000); };
    template <typename INT> struct static_power10<INT, 15> { static constexpr INT value = TS_UCONST64(1000000000000000); };
    template <typename INT> struct static_power10<INT, 16> { static constexpr INT value = TS_UCONST64(10000000000000000); };
    template <typename INT> struct static_power10<INT, 17> { static constexpr INT value = TS_UCONST64(100000000000000000); };
    template <typename INT> struct static_power10<INT, 18> { static constexpr INT value = TS_UCONST64(1000000000000000000); };
    template <typename INT> struct static_power10<INT, 19> { static constexpr INT value = TS_UCONST64(10000000000000000000); };
    //! @endcond
}

#include "tsIntegerUtilsTemplate.h"
