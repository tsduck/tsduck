//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Some utilities on integers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {
    //!
    //! A C++20 concept which requires a type to be an integer or an enumeration type.
    //!
    template<typename T>
    concept int_enum = std::is_integral<T>::value || std::is_enum<T>::value;

    //
    // Implementation tools for underlying_type.
    //
    //! @cond nodoxygen
    template<bool ISENUM, typename T> struct underlying_type_1 { using type = T; };
    template<typename T> struct underlying_type_1<true, T> { using type = typename std::underlying_type<T>::type; };
    //! @endcond

    //!
    //! The meta-type ts::underlying_type is a generalization of std::underlying_type which works on integral or floating-point types as well.
    //! The underlying type of any type other than enum is the type itself.
    //! @tparam T An integral or enumeration type.
    //!
    template<typename T>
    struct underlying_type {
        //! The underlying integer type.
        using type = typename underlying_type_1<std::is_enum<T>::value, T>::type;
    };

    //!
    //! Helper type for ts::underlying_type.
    //! @tparam T An integral or enumeration type.
    //!
    template<typename T>
    using underlying_type_t = typename underlying_type<T>::type;

    //
    // Implementation tools for make_signed.
    //
    //! @cond nodoxygen
    template<typename T, size_t SIZE, bool ISINT, bool ISSIGNED> struct make_signed_impl { using type = T; };
    template<> struct make_signed_impl<bool, sizeof(bool), std::integral<bool>, std::signed_integral<bool>> { using type = int8_t; };
    template<typename T> struct make_signed_impl<T, 1, true, false> { using type = int16_t; };
    template<typename T> struct make_signed_impl<T, 2, true, false> { using type = int32_t; };
    template<typename T> struct make_signed_impl<T, 4, true, false> { using type = int64_t; };
    template<typename T> struct make_signed_impl<T, 8, true, false> { using type = int64_t; };
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
        using type = typename make_signed_impl<T, sizeof(T), std::integral<T>, std::signed_integral<T>>::type;
    };

    //!
    //! Helper type for ts::make_signed.
    //! @tparam T An integral or floating-point type.
    //!
    template<typename T>
    using make_signed_t = typename make_signed<T>::type;

    //
    // Implementation tools for int_max.
    //
    //! @cond nodoxygen
    template<bool ISSIGNED> struct int_max_impl { using type = void; };
    template<> struct int_max_impl<true> { using type = std::intmax_t; };
    template<> struct int_max_impl<false> { using type = std::uintmax_t; };
    //! @endcond

    //!
    //! The meta-type ts::int_max selects the integer type with largest width and same signedness as another integer type.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //!
    template <typename INT> requires std::integral<INT>
    struct int_max {
        //! The integer type with the same signedness and largest width.
        //! In practice, it is either @c std::uintmax_t or @c std::intmax_t.
        using type = typename int_max_impl<std::signed_integral<INT>>::type;
    };

    //!
    //! Helper type for ts::int_max.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //!
    template <typename INT> requires std::integral<INT>
    using int_max_t = typename int_max<INT>::type;

    //!
    //! Statically check if a integral or enum value is negative.
    //! @tparam T An integral or enumeration type.
    //! @param [in] x A value of type @a T.
    //! @return True if the underlying integral type of @a T is signed and @a x is negative.
    //! False otherwise.
    //!
    template<typename T> requires int_enum<T>
    constexpr bool is_negative(T x)
    {
        if constexpr (std::signed_integral<underlying_type_t<T>>) {
            return static_cast<underlying_type_t<T>>(x) < 0;
        }
        else {
            return false;
        }
    }

    //!
    //! Absolute value of integer types, also working on unsigned types.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @return Ansolute value of @a a.
    //!
    template <typename INT> requires std::integral<INT>
    inline INT abs(INT a)
    {
        if constexpr (std::unsigned_integral<INT>) {
            return a;
        }
        else {
            return a < 0 ? -a : a;
        }
    }

    //!
    //! Integer cross-type bound check.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @return True if the value of @a x is within the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2> requires std::integral<INT1> && std::integral<INT2>
    inline bool bound_check(INT2 x)
    {
        // Actual implementations of bound_check, depending on type profiles.
        if constexpr (std::signed_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) <= sizeof(INT2)) {
            // signed <-- unsigned of same or larger size (test: higher bound).
            return x <= INT2(std::numeric_limits<INT1>::max());
        }
        else if constexpr (std::signed_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) > sizeof(INT2)) {
            // signed <-- unsigned of smaller size (always fit).
            return true;
        }
        else if constexpr (std::unsigned_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // unsigned <-- signed of larger size (test: lower and higher bounds).
            return x >= 0 && x <= INT2(std::numeric_limits<INT1>::max());
        }
        else if constexpr (std::unsigned_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // unsigned <-- signed of same or smaller size (test: lower bound).
            return x >= 0;
        }
        else if constexpr (std::unsigned_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // unsigned <-- unsigned of larger size (test: higher bound).
            return x <= INT2(std::numeric_limits<INT1>::max());
        }
        else if constexpr (std::unsigned_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // unsigned <-- unsigned of smaller size (always fit).
            return true;
        }
        else if constexpr (std::signed_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // signed <-- signed of smaller size (always fit).
            return true;
        }
        else if constexpr (std::signed_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // signed <-- signed of larger size (test: lower and higher bounds).
            return x >= INT2(std::numeric_limits<INT1>::min()) && x <= INT2(std::numeric_limits<INT1>::max());
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Bounded integer cast.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @return The value of @a x, within the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2> requires std::integral<INT1> && std::integral<INT2>
    inline INT1 bounded_cast(INT2 x)
    {
        // Actual implementations of bounded_cast, depending on type profiles.
        if constexpr (std::signed_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) <= sizeof(INT2)) {
            // signed <-- unsigned of same or larger size (test: higher bound).
            return INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max())));
        }
        else if constexpr (std::signed_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) > sizeof(INT2)) {
            // signed <-- unsigned of smaller size (always fit).
            return INT1(x);
        }
        else if constexpr (std::unsigned_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // unsigned <-- signed of larger size (test: lower and higher bounds).
            return x < 0 ? 0 : INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max())));
        }
        else if constexpr (std::unsigned_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // unsigned <-- signed of same or smaller size (test: lower bound).
            return x < 0 ? 0 : INT1(x);
        }
        else if constexpr (std::unsigned_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // unsigned <-- unsigned of larger size (test: higher bound).
            return INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max())));
        }
        else if constexpr (std::unsigned_integral<INT1> && std::unsigned_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // unsigned <-- unsigned of smaller size (always fit).
            return INT1(x);
        }
        else if constexpr (std::signed_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) >= sizeof(INT2)) {
            // signed <-- signed of smaller size (always fit).
            return INT1(x);
        }
        else if constexpr (std::signed_integral<INT1> && std::signed_integral<INT2> && sizeof(INT1) < sizeof(INT2)) {
            // signed <-- signed of larger size (test: lower and higher bounds).
            return INT1(std::max<INT2>(INT2(std::numeric_limits<INT1>::min()), std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max()))));
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Throw an exception if an integer value does not fall into the range of another integer type.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @throw std::out_of_range When the value of @a x is ouside the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2> requires std::integral<INT1> && std::integral<INT2>
    void throw_bound_check(INT2 x)
    {
        if (!bound_check<INT1>(x)) {
            throw std::out_of_range("integer value out of range");
        }
    }

    //!
    //! In debug mode, throw an exception if an integer value does not fall into the range of another integer type.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @throw std::out_of_range When the value of @a x is ouside the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2> requires std::integral<INT1> && std::integral<INT2>
    inline void debug_throw_bound_check(INT2 x)
    {
#if defined(DEBUG)
        throw_bound_check<INT1>(x);
#endif
    }

    //!
    //! Check if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @return True if @a a + @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline bool add_overflow(INT a, INT b, INT res)
    {
        if constexpr (std::unsigned_integral<INT>) {
            return a > res;
        }
        else if constexpr (std::signed_integral<INT>) {
            // A mask with sign bit set for the INT type.
            TS_PUSH_WARNING()
            TS_GCC_NOWARNING(shift-negative-value)
            TS_LLVM_NOWARNING(shift-sign-overflow)
            constexpr INT sign_bit = static_cast<INT>(1) << (8 * sizeof(INT) - 1);
            TS_POP_WARNING()
            // MSB of (x ^ y) is set when x and y have distinct signs.
            // If a and b have distinct signs, never overflow.
            // If a and b have same sign, overflow when the result has a different sign.
            return ((~(a ^ b)) & (a ^ res) & sign_bit) != 0;
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Check if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a + @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    bool add_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Check if an integer substraction generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a - @a b.
    //! @return True if @a a - @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline bool sub_overflow(INT a, INT b, INT res)
    {
        if constexpr (std::unsigned_integral<INT>) {
            return a < b;
        }
        else if constexpr (std::signed_integral<INT>) {
            return add_overflow(a, -b, res);
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Check if an integer substraction generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a - @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    bool sub_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Check if the negation (opposite sign) of an integer generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @return True if @a -a generates an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline bool neg_overflow(INT a)
    {
        if constexpr (std::unsigned_integral<INT>) {
            return a != 0;
        }
        else if constexpr (std::signed_integral<INT>) {
            return a == std::numeric_limits<INT>::min();
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Check if an integer multiplication generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a * @a b.
    //! @return True if @a a * @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline bool mul_overflow(INT a, INT b, INT res)
    {
        return a != 0 && res / a != b;
    }

    //!
    //! Check if an integer multiplication generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a * @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    bool mul_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Throw an exception if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @throw std::overflow_error When @a a + @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void throw_add_overflow(INT a, INT b, INT res)
    {
        if (add_overflow(a, b, res)) {
            throw std::overflow_error("addition overflow");
        }
    }

    //!
    //! Throw an exception if an integer substraction generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a - @a b.
    //! @throw std::overflow_error When @a a - @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void throw_sub_overflow(INT a, INT b, INT res)
    {
        if (sub_overflow(a, b, res)) {
            throw std::overflow_error("substraction overflow");
        }
    }

    //!
    //! Throw an exception if the negation (opposite sign) of an integer generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @throw std::overflow_error When @a -a generates an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void thow_neg_overflow(INT a)
    {
        if (neg_overflow(a)) {
            throw std::overflow_error("sign negation overflow");
        }
    }

    //!
    //! Throw an exception if an integer multiplication generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a * @a b.
    //! @throw std::overflow_error When @a a * @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void throw_mul_overflow(INT a, INT b, INT res)
    {
        if (mul_overflow(a, b, res)) {
            throw std::overflow_error("multiplication overflow");
        }
    }

    //!
    //! Throw an exception if the denominator of an integer division is zero.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] den The denominator of an integer division.
    //! @throw std::underflow_error When @a den is zero.
    //!
    template <typename INT> requires std::integral<INT>
    inline void throw_div_zero(INT den)
    {
        if (den == INT(0)) {
            throw std::underflow_error("divide by zero");
        }
    }

    //!
    //! In debug mode, throw an exception if an integer addition generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @throw std::overflow_error When @a a + @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void debug_throw_add_overflow(INT a, INT b, INT res)
    {
#if defined(DEBUG)
        throw_add_overflow(a, b, res);
#endif
    }

    //!
    //! In debug mode, throw an exception if an integer substraction generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a - @a b.
    //! @throw std::overflow_error When @a a - @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void debug_throw_sub_overflow(INT a, INT b, INT res)
    {
#if defined(DEBUG)
        throw_sub_overflow(a, b, res);
#endif
    }

    //!
    //! In debug mode, throw an exception if the negation (opposite sign) of an integer generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @throw std::overflow_error When @a -a generates an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void debug_thow_neg_overflow(INT a)
    {
#if defined(DEBUG)
        thow_neg_overflow(a);
#endif
    }

    //!
    //! In debug mode, throw an exception if an integer multiplication generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a * @a b.
    //! @throw std::overflow_error When @a a * @a b generated an overflow.
    //!
    template <typename INT> requires std::integral<INT>
    inline void debug_throw_mul_overflow(INT a, INT b, INT res)
    {
#if defined(DEBUG)
        throw_mul_overflow(a, b, res);
#endif
    }

    //!
    //! In debug mode, throw an exception if the denominator of an integer division is zero.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] den The denominator of an integer division.
    //! @throw std::underflow_error When @a den is zero.
    //!
    template <typename INT> requires std::integral<INT>
    inline void debug_throw_div_zero(INT den)
    {
#if defined(DEBUG)
        throw_div_zero(den);
#endif
    }

    //!
    //! Integer division with rounding to closest value (instead of truncating).
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer.
    //! @param [in] b An integer.
    //! @return The value of @a a / @a b, rounded to closest value.
    //!
    template <typename INT> requires std::integral<INT>
    INT rounded_div(INT a, INT b)
    {
        if constexpr (std::unsigned_integral<INT>) {
            return (a + b/2) / b;
        }
        else if constexpr (std::signed_integral<INT>) {
            return ((a < 0) ^ (b < 0)) ? ((a - b/2) / b) : ((a + b/2) / b);
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Perform a bounded addition without overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return The value @a a + @a b. The value is @e bounded, in
    //! case of underflow or overflow, the result is the min or max
    //! value of the type, respectively.
    //!
    template <typename INT> requires std::integral<INT>
    INT bounded_add(INT a, INT b);

    //!
    //! Perform a bounded subtraction without overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return The value @a a - @a b. The value is @e bounded, in
    //! case of underflow or overflow, the result is the min or max
    //! value of the type, respectively.
    //!
    template <typename INT> requires std::integral<INT>
    INT bounded_sub(INT a, INT b);

    //!
    //! Round @a x down to previous multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded down to previous multiple of @a f.
    //!
    template <typename INT> requires std::integral<INT>
    INT round_down(INT x, INT f);

    //!
    //! Round @a x up to next multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded up to next multiple of @a f.
    //!
    template <typename INT> requires std::integral<INT>
    INT round_up(INT x, INT f);

    //!
    //! Reduce the sign of an integer fraction.
    //! Make sure that only the numerator carries the sign. The denominator must remain positive.
    //! @tparam INT An integer type.
    //! @param [in,out] num Fraction numerator.
    //! @param [in,out] den Fraction denominator.
    //!
    template <typename INT> requires std::integral<INT>
    void sign_reduce(INT& num, INT& den)
    {
        if constexpr (std::unsigned_integral<INT>) {
            // no sign
        }
        else if constexpr (std::signed_integral<INT>) {
            if (den < 0) {
                num = -num;
                den = -den;
            }
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Perform a sign extension on any subset of a signed integer.
    //!
    //! @tparam INT A signed integer type.
    //! @param [in] x An integer containing a signed value in some number of LSB.
    //! @param [in] bits Number of least significant bits containing a signed value.
    //! @return A signed integer containing the same signed value with proper sign extension on the full size of INT.
    //!
    template <typename INT> requires std::signed_integral<INT>
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
    template <typename INT> requires std::integral<INT>
    size_t BitSize(INT x);

    //!
    //! Get the mask to select a given number of least significant bits in an integer value.
    //! @tparam INT An integer type.
    //! @param [in] bits Number of least significant bits to select. Zero means all bits.
    //! @return The corresponding mask.
    //!
    template <typename INT> requires std::integral<INT>
    INT LSBMask(size_t bits);

    //!
    //! Get the signed/unsigned qualifier of an integer type as a string.
    //!
    //! @tparam INT An integer type.
    //! @return Either u"signed" or u"unsigned".
    //!
    template <typename INT> requires std::integral<INT>
    inline const UChar* SignedDescription()
    {
        if constexpr (std::unsigned_integral<INT>) {
            return u"unsigned";
        }
        else if constexpr (std::signed_integral<INT>) {
            return u"signed";
        }
        else {
            static_assert(false, "invalid integer type");
        }
    }

    //!
    //! Compute a greatest common denominator (GCD).
    //!
    //! @tparam INT An integer type.
    //! @param [in] x An integer.
    //! @param [in] y An integer.
    //! @return The greatest common denominator (GCD) of @a x and @a y. Always positive.
    //!
    template <typename INT> requires std::integral<INT>
    INT GCD(INT x, INT y);

    //!
    //! Largest representable power of 10 in integer types.
    //! Assuming that no integer type is larger than 64 bits, 10^19 is the largest unsigned power of 10.
    //!
    constexpr size_t MAX_POWER_10 = 19;

    //!
    //! Get a power of 10 using a fast lookup table.
    //!
    //! @param [in] pow The requested power of 10.
    //! @return The requested power of 10. If the value is larger than the largest integer on
    //! this platform, the result is undefined.
    //!
    TSDUCKDLL uint64_t Power10(size_t pow);

    //!
    //! Static values of power of 10.
    //! @tparam POW The exponent of 10.
    //! @tparam INT The integer type for the power of 10.
    //!
    template <typename INT, const size_t POW>
    struct static_power10 {
        #if defined(DOXYGEN)
        static constexpr INT value;   //!< Value of 10 power POW.
        #endif
    };

    //! @cond nodoxygen
    // Template specializations.
    template <typename INT> struct static_power10<INT,  0> { static constexpr INT value = 1; };
    template <typename INT> struct static_power10<INT,  1> { static constexpr INT value = 10; };
    template <typename INT> struct static_power10<INT,  2> { static constexpr INT value = 100; };
    template <typename INT> struct static_power10<INT,  3> { static constexpr INT value = 1'000; };
    template <typename INT> struct static_power10<INT,  4> { static constexpr INT value = 10'000; };
    template <typename INT> struct static_power10<INT,  5> { static constexpr INT value = 100'000; };
    template <typename INT> struct static_power10<INT,  6> { static constexpr INT value = 1'000'000; };
    template <typename INT> struct static_power10<INT,  7> { static constexpr INT value = 10'000'000; };
    template <typename INT> struct static_power10<INT,  8> { static constexpr INT value = 100'000'000; };
    template <typename INT> struct static_power10<INT,  9> { static constexpr INT value = 1'000'000'000; };
    template <typename INT> struct static_power10<INT, 10> { static constexpr INT value = 10'000'000'000; };
    template <typename INT> struct static_power10<INT, 11> { static constexpr INT value = 100'000'000'000; };
    template <typename INT> struct static_power10<INT, 12> { static constexpr INT value = 1'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 13> { static constexpr INT value = 10'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 14> { static constexpr INT value = 100'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 15> { static constexpr INT value = 1'000'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 16> { static constexpr INT value = 10'000'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 17> { static constexpr INT value = 100'000'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 18> { static constexpr INT value = 1'000'000'000'000'000'000; };
    template <typename INT> struct static_power10<INT, 19> { static constexpr INT value = 10'000'000'000'000'000'000ull; };
    //! @endcond

    //!
    //! Define the smaller unsigned integer type with at least a given number of bits.
    //! @tparam BITS Minimum number of bits. Must be in the range 0 to 64.
    //!
    template <const size_t BITS>
    struct smaller_unsigned {
        //!
        //! The smaller unsigned integer type with at list @a BITS bits.
        //!
        using type = typename std::conditional<
            BITS <= 8,
            uint8_t,
            typename std::conditional<
                BITS <= 16,
                uint16_t,
                typename std::conditional<
                    BITS <= 32,
                    uint32_t,
                    typename std::conditional<
                        BITS <= 64,
                        uint64_t,
                        void
                    >::type
                >::type
            >::type
        >::type;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Not inlined to avoid optimization which breaks the code.
template <typename INT> requires std::integral<INT>
bool ts::add_overflow(INT a, INT b)
{
    INT res = a + b;
    return add_overflow<INT>(a, b, res);
}

template <typename INT> requires std::integral<INT>
bool ts::sub_overflow(INT a, INT b)
{
    INT res = a - b;
    return sub_overflow<INT>(a, b, res);
}

template <typename INT> requires std::integral<INT>
bool ts::mul_overflow(INT a, INT b)
{
    INT res = a * b;
    return mul_overflow<INT>(a, b, res);
}


//----------------------------------------------------------------------------
// Perform a bounded addition without overflow.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
INT ts::bounded_add(INT a, INT b)
{
    if constexpr (std::unsigned_integral<INT>) {
        // Unsigned addition.
        if (a > std::numeric_limits<INT>::max() - b) {
            // Overflow.
            return std::numeric_limits<INT>::max();
        }
        else {
            return a + b;
        }
    }
    else if constexpr (std::signed_integral<INT>) {
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
        static_assert(false, "invalid integer type");
    }
}


//----------------------------------------------------------------------------
// Perform a bounded subtraction without overflow.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
INT ts::bounded_sub(INT a, INT b)
{
    if constexpr (std::unsigned_integral<INT>) {
        // Unsigned subtraction.
        if (a < b) {
            // Underflow.
            return 0;
        }
        else {
            return a - b;
        }
    }
    else if constexpr (std::signed_integral<INT>) {
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
        static_assert(false, "invalid integer type");
    }
}


//----------------------------------------------------------------------------
// Rounding integers up and down.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
INT ts::round_down(INT x, INT f)
{
    if constexpr (std::unsigned_integral<INT>) {
        return f == 0 ? x : x - x % f;
    }
    else if constexpr (std::signed_integral<INT>) {
        f = INT(std::abs(f));
        return f == 0 ? x : (x >= 0 ? x - x % f : x - (f + x % f) % f);
    }
    else {
        static_assert(false, "invalid integer type");
    }
}

template <typename INT> requires std::integral<INT>
INT ts::round_up(INT x, INT f)
{
    if constexpr (std::unsigned_integral<INT>) {
        return f == 0 ? x : x + (f - x % f) % f;
    }
    else if constexpr (std::signed_integral<INT>) {
        f = INT(std::abs(f));
        return f == 0 ? x : (x >= 0 ? x + (f - x % f) % f : x - x % f);
    }
    else {
        static_assert(false, "invalid integer type");
    }
}


//----------------------------------------------------------------------------
// Perform a sign extension on any subset of a signed integer.
//----------------------------------------------------------------------------

template <typename INT> requires std::signed_integral<INT>
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

template <typename INT> requires std::integral<INT>
size_t ts::BitSize(INT x)
{
    if constexpr (std::unsigned_integral<INT>) {
        size_t size = 1;
        const size_t maxbit = 8 * sizeof(INT);
        for (size_t bit = 0; bit < maxbit && (x = x >> 1) != 0; ++bit) {
            ++size;
        }
        return size;
    }
    else if constexpr (std::signed_integral<INT>) {
        using UNS_INT = typename std::make_unsigned<INT>::type;
        return BitSize<UNS_INT>(UNS_INT(x));
    }
    else {
        static_assert(false, "invalid integer type");
    }
}

//----------------------------------------------------------------------------
// Get the mask to select a given number of least significant bits.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
INT ts::LSBMask(size_t bits)
{
    if (bits == 0 || bits >= 8 * sizeof(INT)) {
        // Unspecified, keep all bits.
        return ~INT(0);
    }
    else {
        return ~INT(0) >> (8 * sizeof(INT) - bits);
    }
}


//----------------------------------------------------------------------------
// Compute a greatest common denominator (GCD).
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
INT ts::GCD(INT x, INT y)
{
    if constexpr (std::unsigned_integral<INT>) {
        INT z;
        while (y != 0) {
            z = x % y;
            x = y;
            y = z;
        }
    }
    else if constexpr (std::signed_integral<INT>) {
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
    }
    else {
        static_assert(false, "invalid integer type");
    }
    return x;
}
