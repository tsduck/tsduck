//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

namespace ts {
    //
    // Implementation tools for underlying_type.
    //
    //! @cond nodoxygen
    template<bool ISENUM, typename T> struct underlying_type_1 { typedef T type; };
    template<typename T> struct underlying_type_1<true, T> { typedef typename std::underlying_type<T>::type type; };
    //! @endcond

    //!
    //! The meta-type ts::underlying_type is a generalization of std::underlying_type which works on integer types as well.
    //! The underlying type of an integer type is the type itself.
    //! @tparam T An integral or enumeration type.
    //!
    template<typename T>
    struct underlying_type {
        //! The underlying integer type.
        typedef typename underlying_type_1<std::is_enum<T>::value, T>::type type;
    };

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
//! TS_ENABLE_BITMASK_OPERATORS(E);
//!
//! E e = A | B | C;
//! e ^= B | C;
//! @endcode
//!
#define TS_ENABLE_BITMASK_OPERATORS(T)           \
    namespace ts {                               \
        /** Template specialization on type T */ \
        template<>                               \
        struct EnableBitMaskOperators<T>         \
        {                                        \
            /** Enable bitmask operators on T */ \
            static constexpr bool value = true;  \
        };                                       \
    }                                            \
    typedef int TS_UNIQUE_NAME(for_trailing_semicolon)

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
//! Boolean "not" operator on enumeration types.
//! @tparam ENUM An enumeration type or enumeration class.
//! @param [in] a Enumeration value. Typically an expression uing bitmask "and" or "or" operators on @a ENUM.
//! @return True if @a a is zero (no flag set).
//!
template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value && ts::EnableBitMaskOperators<ENUM>::value>::type* = nullptr>
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
