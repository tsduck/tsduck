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
//!  Some utilities on integers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

//!
//! A convenience macro to declare a template with complex @c std::enable_if conditions on two integer types.
//! This macro declares two template integer types @a INT1 and @a INT2. It expands to:
//! @code
//! template <typename INT1, typename INT2, typename std::enable_if<...>::type = 0>
//! @endcode
//!
//! @param sign1 Either @a signed or @a unsigned, condition on first integer type INT1.
//! @param op A comparison operator ('==', '<', '>=', etc) to apply on the sizes of @a INT1 and @a INT2.
//! @param sign2 Either @a signed or @a unsigned, condition on second integer type INT2.
//!
#define TS_TEMPLATE_IF_INTS(sign1, op, sign2)                          \
    template <typename INT1,                                           \
              typename INT2,                                           \
              typename std::enable_if<std::is_integral<INT1>::value && \
                                      std::is_integral<INT2>::value && \
                                      std::is_##sign1<INT1>::value &&  \
                                      std::is_##sign2<INT2>::value &&  \
                                      (sizeof(INT1) op sizeof(INT2)),  \
                                      int>::type = 0>

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
    //! Absolute value of integer types, also working on unsigned types.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @return Ansolute value of @a a.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    inline INT abs(INT a) { return a; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    inline INT abs(INT a) { return a < 0 ? -a : a; } // signed version
    //! @endcond

#if defined(DOXYGEN)
    //!
    //! Integer cross-type bound check.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @return True if the value of @a x is within the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
    bool bound_check(INT2 x);
#else
    // Actual implementations of bound_check, depending on type profiles.

    // signed <-- unsigned of same or larger size (test: higher bound).
    TS_TEMPLATE_IF_INTS(signed, <=, unsigned)
    inline bool bound_check(INT2 x) { return x <= INT2(std::numeric_limits<INT1>::max()); }

    // signed <-- unsigned of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(signed, >, unsigned)
    inline bool bound_check(INT2 x) { return true; }

    // unsigned <-- signed of larger size (test: lower and higher bounds).
    TS_TEMPLATE_IF_INTS(unsigned, <, signed)
    inline bool bound_check(INT2 x) { return x >= 0 && x <= INT2(std::numeric_limits<INT1>::max()); }

    // unsigned <-- signed of same or smaller size (test: lower bound).
    TS_TEMPLATE_IF_INTS(unsigned, >=, signed)
    inline bool bound_check(INT2 x) { return x >= 0; }

    // unsigned <-- unsigned of larger size (test: higher bound).
    TS_TEMPLATE_IF_INTS(unsigned, <, unsigned)
    inline bool bound_check(INT2 x) { return x <= INT2(std::numeric_limits<INT1>::max()); }

    // unsigned <-- unsigned of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(unsigned, >=, unsigned)
    inline bool bound_check(INT2 x) { return true; }

    // signed <-- signed of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(signed, >=, signed)
    inline bool bound_check(INT2 x) { return true; }

    // signed <-- signed of larger size (test: lower and higher bounds).
    TS_TEMPLATE_IF_INTS(signed, <, signed)
    inline bool bound_check(INT2 x) { return x >= INT2(std::numeric_limits<INT1>::min()) && x <= INT2(std::numeric_limits<INT1>::max()); }
#endif

#if defined(DOXYGEN)
    //!
    //! Bounded integer cast.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @return The value of @a x, within the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
    INT1 bounded_cast(INT2 x);
#else
    // Actual implementations of bounded_cast, depending on type profiles.

    // signed <-- unsigned of same or larger size (test: higher bound).
    TS_TEMPLATE_IF_INTS(signed, <=, unsigned)
    inline INT1 bounded_cast(INT2 x) { return INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max()))); }

    // signed <-- unsigned of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(signed, >, unsigned)
    inline INT1 bounded_cast(INT2 x) { return INT1(x); }

    // unsigned <-- signed of larger size (test: lower and higher bounds).
    TS_TEMPLATE_IF_INTS(unsigned, <, signed)
    inline INT1 bounded_cast(INT2 x) { return x < 0 ? 0 : INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max()))); }

    // unsigned <-- signed of same or smaller size (test: lower bound).
    TS_TEMPLATE_IF_INTS(unsigned, >=, signed)
    inline INT1 bounded_cast(INT2 x) { return x < 0 ? 0 : INT1(x); }

    // unsigned <-- unsigned of larger size (test: higher bound).
    TS_TEMPLATE_IF_INTS(unsigned, <, unsigned)
    inline INT1 bounded_cast(INT2 x) { return INT1(std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max()))); }

    // unsigned <-- unsigned of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(unsigned, >=, unsigned)
    inline INT1 bounded_cast(INT2 x) { return INT1(x); }

    // signed <-- signed of smaller size (always fit).
    TS_TEMPLATE_IF_INTS(signed, >=, signed)
    inline INT1 bounded_cast(INT2 x) { return INT1(x); }

    // signed <-- signed of larger size (test: lower and higher bounds).
    TS_TEMPLATE_IF_INTS(signed, <, signed)
    inline INT1 bounded_cast(INT2 x)
    {
        return INT1(std::max<INT2>(INT2(std::numeric_limits<INT1>::min()), std::min<INT2>(x, INT2(std::numeric_limits<INT1>::max()))));
    }
#endif

    //!
    //! Throw an exception if an integer value does not fall into the range of another integer type.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @throw std::out_of_range When the value of @a x is ouside the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
    void throw_bound_check(INT2 x) {
        if (!bound_check<INT1>(x)) {
            throw std::out_of_range("integer value out of range");
        }
    }

#if defined(DEBUG) || defined(DOXYGEN)
    //!
    //! In debug mode, throw an exception if an integer value does not fall into the range of another integer type.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT1 An integer type.
    //! @tparam INT2 An integer type.
    //! @param [in] x An integer value of type @a INT2.
    //! @throw std::out_of_range When the value of @a x is ouside the limits of type @a INT1.
    //!
    template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
    inline void debug_throw_bound_check(INT2 x) { throw_bound_check<INT1>(x); }
#else
    template <typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
    inline void debug_throw_bound_check(INT2 x) {}
#endif

    //!
    //! Check if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @return True if @a a + @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    inline bool add_overflow(INT a, INT b, INT res) { return a > res; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    inline bool add_overflow(INT a, INT b, INT res) { // signed version
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
    //! @endcond

    //!
    //! Check if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a + @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    bool add_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Check if an integer substraction generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a - @a b.
    //! @return True if @a a - @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    inline bool sub_overflow(INT a, INT b, INT res) { return a < b; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    inline bool sub_overflow(INT a, INT b, INT res) { return add_overflow(a, -b, res); } // signed version
    //! @endcond

    //!
    //! Check if an integer substraction generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a - @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    bool sub_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Check if the negation (opposite sign) of an integer generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @return True if @a -a generates an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    inline bool neg_overflow(INT a) { return a != 0; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    inline bool neg_overflow(INT a) { return a == std::numeric_limits<INT>::min(); } // signed version
    //! @endcond

    //!
    //! Check if an integer multiplication generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a * @a b.
    //! @return True if @a a * @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline bool mul_overflow(INT a, INT b, INT res) { return a != 0 && res / a != b; }

    //!
    //! Check if an integer multiplication generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @return True if @a a * @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    bool mul_overflow(INT a, INT b) TS_NO_OPTIMIZE;

    //!
    //! Throw an exception if an integer addition generates an overflow.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @throw std::overflow_error When @a a + @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void throw_add_overflow(INT a, INT b, INT res) {
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
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void throw_sub_overflow(INT a, INT b, INT res) {
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
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void thow_neg_overflow(INT a) {
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
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void throw_mul_overflow(INT a, INT b, INT res) {
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
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void throw_div_zero(INT den) {
        if (den == INT(0)) {
            throw std::underflow_error("divide by zero");
        }
    }

#if defined(DEBUG) || defined(DOXYGEN)
    //!
    //! In debug mode, throw an exception if an integer addition generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a + @a b.
    //! @throw std::overflow_error When @a a + @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_add_overflow(INT a, INT b, INT res) { throw_add_overflow(a, b, res); }

    //!
    //! In debug mode, throw an exception if an integer substraction generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a - @a b.
    //! @throw std::overflow_error When @a a - @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_sub_overflow(INT a, INT b, INT res) { throw_sub_overflow(a, b, res); }

    //!
    //! In debug mode, throw an exception if the negation (opposite sign) of an integer generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer value.
    //! @throw std::overflow_error When @a -a generates an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_thow_neg_overflow(INT a) { thow_neg_overflow(a); }

    //!
    //! In debug mode, throw an exception if an integer multiplication generates an overflow.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a First integer.
    //! @param [in] b Second integer.
    //! @param [in] res The result of @a a * @a b.
    //! @throw std::overflow_error When @a a * @a b generated an overflow.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_mul_overflow(INT a, INT b, INT res) { throw_mul_overflow(a, b, res); }

    //!
    //! In debug mode, throw an exception if the denominator of an integer division is zero.
    //! If the macro @c DEBUG is not defined, this function does nothing.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] den The denominator of an integer division.
    //! @throw std::underflow_error When @a den is zero.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_div_zero(INT den) { throw_div_zero(den); }
#else
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_add_overflow(INT a, INT b, INT res) {}

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_sub_overflow(INT a, INT b, INT res) {}

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_thow_neg_overflow(INT a) {}

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_mul_overflow(INT a, INT b, INT res) {}

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value, int>::type = 0>
    inline void debug_throw_div_zero(INT den) {}
#endif

    //!
    //! Integer division with rounding to closest value (instead of truncating).
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer.
    //! @param [in] b An integer.
    //! @return The value of @a a / @a b, rounded to closest value.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT rounded_div(INT a, INT b) { return (a + b/2) / b; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT rounded_div(INT a, INT b) { return ((a < 0) ^ (b < 0)) ? ((a - b/2) / b) : ((a + b/2) / b); } // signed version
    //! @endcond

    //!
    //! Check if an integer value is negative, optimized for signed or unsigned type.
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! @param [in] a An integer.
    //! @return True if @a a is negative.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    bool is_negative(INT a) { return false; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    bool is_negative(INT a) { return a < 0; } // signed version
    //! @endcond

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
    INT bounded_add(INT a, INT b); // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT bounded_add(INT a, INT b); // signed version
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
    INT bounded_sub(INT a, INT b); // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT bounded_sub(INT a, INT b); // signed version
    //! @endcond

    //!
    //! Round @a x down to previous multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded down to previous multiple of @a f.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT round_down(INT x, INT f); // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT round_down(INT x, INT f); // signed version
    //! @endcond

    //!
    //! Round @a x up to next multiple of a factor @a f.
    //! @tparam INT An integer type.
    //! @param [in] x An integer value.
    //! @param [in] f A factor (its absolute value is used if negative).
    //! @return The value @a x rounded up to next multiple of @a f.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT round_up(INT x, INT f); // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT round_up(INT x, INT f); // signed version
    //! @endcond

    //!
    //! Reduce the sign of an integer fraction.
    //! Make sure that only the numerator carries the sign. The denominator must remain positive.
    //! @tparam INT An integer type.
    //! @param [in,out] num Fraction numerator.
    //! @param [in,out] den Fraction denominator.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    void sign_reduce(INT& num, INT& den) {} // unsigned version

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    void sign_reduce(INT& num, INT& den) { if (den < 0) { num = -num; den = -den; } } // signed version
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
    //! Get the signed/unsigned qualifier of an integer type as a string.
    //!
    //! @tparam INT An integer type.
    //! @return Either u"signed" or u"unsigned".
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value, int>::type = 0>
    inline const UChar* SignedDescription() { return u"unsigned"; } // unsigned version

    //! @cond nodoxygen
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value, int>::type = 0>
    inline const UChar* SignedDescription() { return u"signed"; } // signed version
    //! @endcond

    //!
    //! Compute a greatest common denominator (GCD).
    //!
    //! @tparam INT An integer type.
    //! @param [in] x An integer.
    //! @param [in] y An integer.
    //! @return The greatest common denominator (GCD) of @a x and @a y. Always positive.
    //!
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type* = nullptr>
    INT GCD(INT x, INT y);

    //! @cond nodoxygen
    template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type* = nullptr>
    INT GCD(INT x, INT y); // signed version
    //! @endcond

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
        typedef typename std::conditional<
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
        >::type type;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Not inlined to avoid optimization which breaks the code.
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
