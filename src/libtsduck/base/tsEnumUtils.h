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
//!
//!  @file
//!  @ingroup cpp
//!  Some utilities on enumeration types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts
{
    //!
    //! This traits is used to enable bitmask operators on an enumeration type.
    //! The default value disables these operators. Define a template specialization
    //! or use macro TS_ENABLE_BITMASK_OPERATORS to enable the bitmask operators.
    //! @see TS_ENABLE_BITMASK_OPERATORS
    //! @tparam T Any type.
    //!
    template<typename T>
    struct EnableBitMaskOperators
    {
        //! The constant @a value enables or disables the bitmask operators on type @a T.
        static constexpr bool value = false;
    };
}

//!
//! @hideinitializer
//! This macro enables bitmask operators on an enumeration type.
//! @param T An enumeration type or enumeration class.
//!
//! Example:
//! @code
//! enum E {A = 0x01, B = 0x02, C = 0x04};
//! TS_ENABLE_BITMASK_OPERATORS(E)
//!
//! E e = A | B | C;
//! e ^= B | C;
//! @endcode
//!
#define TS_ENABLE_BITMASK_OPERATORS(T)       \
    /** Template specialization on type T */ \
    template<>                               \
    struct ts::EnableBitMaskOperators<T>     \
    {                                        \
        /** Enable bitmask operators on T */ \
        static constexpr bool value = true;  \
    }

//!
//! Bitmask "not" unary operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Operand.
//! @return Bitwise "not" @a a
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator~(ENUM a)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(~static_cast<IENUM>(a));
}

//!
//! Bitmask "or" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a First operand.
//! @param [in] b Second operand.
//! @return Bitwise @a a "or" @a b
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator|(ENUM a, ENUM b)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(static_cast<IENUM>(a) | static_cast<IENUM>(b));
}

//!
//! Bitmask "and" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a First operand.
//! @param [in] b Second operand.
//! @return Bitwise @a a "and" @a b
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator&(ENUM a, ENUM b)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(static_cast<IENUM>(a) & static_cast<IENUM>(b));
}

//!
//! Bitmask "xor" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a First operand.
//! @param [in] b Second operand.
//! @return Bitwise @a a "xor" @a b
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator^(ENUM a, ENUM b)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(static_cast<IENUM>(a) ^ static_cast<IENUM>(b));
}

//!
//! Bitmask "assign or" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in,out] a Variable to update.
//! @param [in] b Second operand.
//! @return A reference to @a a.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator|=(ENUM& a, ENUM b)
{
    return a = a | b;
}

//!
//! Bitmask "assign and" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in,out] a Variable to update.
//! @param [in] b Second operand.
//! @return A reference to @a a.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator&=(ENUM& a, ENUM b)
{
    return a = a & b;
}

//!
//! Bitmask "assign xor" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in,out] a Variable to update.
//! @param [in] b Second operand.
//! @return A reference to @a a.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator^=(ENUM& a, ENUM b)
{
    return a = a ^ b;
}

//!
//! Bitmask "left shift" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Enum value to shift.
//! @param [in] b Number of bits to shift.
//! @return Bitwise @a a, shifted left @a b bits.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator<<(ENUM a, size_t b)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(static_cast<IENUM>(a) << b);
}

//!
//! Bitmask "right shift" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Enum value to shift.
//! @param [in] b Number of bits to shift.
//! @return Bitwise @a a, shifted right @a b bits.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline constexpr ENUM operator>>(ENUM a, size_t b)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(static_cast<IENUM>(a) >> b);
}

//!
//! Bitmask "assign left shift" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in,out] a Enum variable to shift.
//! @param [in] b Number of bits to shift.
//! @return A reference to @a a.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator<<=(ENUM& a, size_t b)
{
    return a = a << b;
}

//!
//! Bitmask "assign right shift" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in,out] a Enum variable to shift.
//! @param [in] b Number of bits to shift.
//! @return A reference to @a a.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator>>=(ENUM& a, size_t b)
{
    return a = a >> b;
}
