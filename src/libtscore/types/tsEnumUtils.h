//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtscore cpp
//!  Some utilities on enumeration types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! This traits is used to enable bitmask operators on an enumeration type.
    //! The default value disables these operators. Define a template specialization
    //! or use macro TS_ENABLE_BITMASK_OPERATORS to enable the bitmask operators.
    //! @ingroup libtscore cpp
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
//! @ingroup cpp
//! @param T An enumeration type or enumeration class.
//!
//! Example:
//! @code
//! enum E {A = 0x01, B = 0x02, C = 0x04};
//! TS_ENABLE_BITMASK_OPERATORS(E);
//!
//! E e = A | B | C;
//! e ^= B | C;
//! @endcode
//!
#define TS_ENABLE_BITMASK_OPERATORS(T)                 \
    namespace ts {                                     \
        /** Template specialization on type T */       \
        template<>                                     \
        struct EnableBitMaskOperators<T>               \
        {                                              \
            /** Enable bitmask operators on T */       \
            static constexpr bool value = true;        \
        };                                             \
    }                                                  \
    /** @cond nodoxygen */                             \
    using TS_UNIQUE_NAME(for_trailing_semicolon) = int \
    /** @endcond */

//!
//! Bitmask "not" unary operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Operand.
//! @return Bitwise "not" @a a
//!
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
inline constexpr ENUM operator~(ENUM a)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<ENUM>(~static_cast<IENUM>(a));
}

//!
//! Boolean "not" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Enumeration value. Typically an expression uing bitmask "and" or "or" operators on @a ENUM.
//! @return True if @a a is zero (no flag set).
//!
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
inline constexpr bool operator!(ENUM a)
{
    using IENUM = typename std::underlying_type<ENUM>::type;
    return static_cast<IENUM>(a) == 0;
}

//!
//! Bitmask "or" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a First operand.
//! @param [in] b Second operand.
//! @return Bitwise @a a "or" @a b
//!
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
template <typename ENUM> requires std::is_enum_v<ENUM> && ts::EnableBitMaskOperators<ENUM>::value
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
//! Implementation note: The use of std::enable_if instead of the "requires" directive
//! is a workaround for a bug in clang 19.
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
//! Implementation note: The use of std::enable_if instead of the "requires" directive
//! is a workaround for a bug in clang 19.
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
//! Implementation note: The use of std::enable_if instead of the "requires" directive
//! is a workaround for a bug in clang 19.
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
//! Implementation note: The use of std::enable_if instead of the "requires" directive
//! is a workaround for a bug in clang 19.
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
inline ENUM& operator>>=(ENUM& a, size_t b)
{
    return a = a >> b;
}
