//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Encapsulation of a floating-point type as an AbstractNumber.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNumber.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Encapsulation of a double floating-point as an AbstractNumber.
    //! @ingroup cpp
    //!
    //! @tparam FLOAT_T The underlying floating-point type.
    //! @tparam PREC The number of decimal digits to display by default. This is a display
    //! attribute only, it does not alter the binary representation of floating-point values.
    //! The default is to display 6 digits.
    //!
    template <typename FLOAT_T, const size_t PREC = 6, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type = 0>
    class FloatingPoint: public AbstractNumber
    {
    private:
        FLOAT_T _value = 0.0;

    public:
        //!
        //! The underlying floating-point type.
        //!
        typedef FLOAT_T float_t;

        //!
        //! An integer type for conversion to integral values.
        //! Overflow may occur.
        //!
        typedef int64_t int_t;

        //!
        //! The displayed precision of the floating-point type (number of decimal digits).
        //! This is a display attribute only, it does not alter the binary representation of floating-point values.
        //!
        static constexpr size_t DISPLAY_PRECISION = PREC;

        //!
        //! The minimum representable value of this fraction type.
        //!
        static const FloatingPoint MIN;

        //!
        //! The maximum representable value of this fraction type.
        //!
        static const FloatingPoint MAX;

        //!
        //! Precision of "equal" comparisons.
        //! We cannot test equality between floating point values. Because of rounding issues,
        //! it does not make sense. Instead we check if the absolute value of their difference
        //! is lower than some very small value, this precision.
        //!
        static constexpr float_t EQUAL_PRECISION = 100.0 * std::numeric_limits<float_t>::min();

        //!
        //! Default constructor, implicitly initialized to zero.
        //!
        FloatingPoint() = default;

        //!
        //! Constructor from an integer or floating-point value.
        //! @tparam NUM_T Some integer or floating-point type.
        //! @param [in] x Initial value.
        //!
        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint(NUM_T x) : _value(float_t(x)) {}

        // Implementation of interfaces.
        virtual ts::UString description() const override;
        virtual bool inRange(int64_t min, int64_t max) const override;
        virtual int64_t toInt64() const override;
        virtual double toDouble() const override;
        virtual bool fromString(const UString& str, UChar separator = COMMA, UChar decimal_dot = FULL_STOP) override;
        virtual UString toString(size_t min_width = 0,
                                 bool right_justified = true,
                                 UChar separator = COMMA,
                                 bool force_sign = false,
                                 size_t decimals = NPOS,
                                 bool force_decimals = false,
                                 UChar decimal_dot = FULL_STOP,
                                 UChar pad = SPACE) const override;

        //!
        //! Conversion to integral value.
        //! @return The value in integral units. Underflow or overflow rounding is applied when necessary.
        //!
        int_t toInt() const { return int_t(std::round(_value)); }

        //!
        //! Get the absolute value.
        //! @return The absolute value of this fraction number.
        //!
        FloatingPoint abs() const { return FloatingPoint(std::abs(_value)); }

        //!
        //! Get the maximum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The maximum value of this fraction and @a x.
        //!
        FloatingPoint max(const FloatingPoint& x) const { return _value >= x._value ? *this : x; }

        //!
        //! Get the minimum value of two fractions.
        //! @param [in] x Another fraction.
        //! @return The minimum value of this fraction and @a x.
        //!
        FloatingPoint min(const FloatingPoint& x) const { return _value <= x._value ? *this : x; }

        //!
        //! Check if this FloatingPoint number generates an overflow when multiplied by an integer.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @tparam INT2 Another integer type.
        //! @param [in] x An integer of type @a INT2.
        //! @return Always false. A better idea?
        //!
        template<typename INT2, typename std::enable_if<std::is_integral<INT2>::value, int>::type = 0>
        bool mulOverflow(INT2 x) const { return false; }

        //!
        //! Check if this FloatingPoint number generates an overflow when multiplied by another FloatingPoint.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @param [in] x Another FloatingPoint number.
        //! @return Always false. A better idea?
        //!
        bool mulOverflow(const FloatingPoint& x) const { return false; }

        //!
        //! Check if this FloatingPoint number generates an overflow when divided by another FloatingPoint.
        //! The method is present for compliance with other AbstractNumber subclasses.
        //! @param [in] x Another FloatingPoint.
        //! @return Always false. There is no possible division overflow with FloatingPoint.
        //!
        bool divOverflow(const FloatingPoint& x) const { return false; }

        //! @cond nodoxygen
        // The operators are not extensively documented with doxygen (obvious, verbose and redundant).

        FloatingPoint operator-() const { return FloatingPoint(-_value); }
        FloatingPoint operator+(const FloatingPoint& x) const { return FloatingPoint(_value + x._value); }
        FloatingPoint operator-(const FloatingPoint& x) const { return FloatingPoint(_value - x._value); }
        FloatingPoint operator*(const FloatingPoint& x) const { return FloatingPoint(_value * x._value); }
        FloatingPoint operator/(const FloatingPoint& x) const { return FloatingPoint(_value / x._value); }

        FloatingPoint& operator+=(const FloatingPoint& x) { _value += x._value; return *this; }
        FloatingPoint& operator-=(const FloatingPoint& x) { _value -= x._value; return *this; }
        FloatingPoint& operator*=(const FloatingPoint& x) { _value *= x._value; return *this; }
        FloatingPoint& operator/=(const FloatingPoint& x) { _value /= x._value; return *this; }

        bool operator==(const FloatingPoint& x) const { return std::abs(_value - x._value) < EQUAL_PRECISION; }
        TS_UNEQUAL_OPERATOR(FloatingPoint)

        bool operator<=(const FloatingPoint& x) const { return _value <= x._value; }
        bool operator>=(const FloatingPoint& x) const { return _value >= x._value; }
        bool operator<(const FloatingPoint& x) const { return _value < x._value; }
        bool operator>(const FloatingPoint& x) const { return _value > x._value; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator+(NUM_T x) const { return FloatingPoint(_value + float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator-(NUM_T x) const { return FloatingPoint(_value - float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator*(NUM_T x) const { return FloatingPoint(_value * float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint operator/(NUM_T x) const { return FloatingPoint(_value / float_t(x)); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator+=(NUM_T x) { _value += float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator-=(NUM_T x) { _value -= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator*=(NUM_T x) { _value *= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        FloatingPoint& operator/=(NUM_T x) { _value /= float_t(x); return *this; }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator==(NUM_T x) const { return std::abs(_value - float_t(x)) < EQUAL_PRECISION; }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator!=(NUM_T x) const { return std::abs(_value - float_t(x)) >= EQUAL_PRECISION; }
#endif

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator<=(NUM_T x) const { return _value <= float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator>=(NUM_T x) const { return _value >= float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator<(NUM_T x) const { return _value < float_t(x); }

        template<typename NUM_T, typename std::enable_if<std::is_arithmetic<NUM_T>::value, int>::type = 0>
        bool operator>(NUM_T x) const { return _value > float_t(x); }

        //! @endcond
    };
}

//! @cond nodoxygen
// The operators are not extensively documented with doxygen (obvious, verbose and redundant).

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator+(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 + x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator-(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return -(x2 - x1); }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator*(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 * x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline ts::FloatingPoint<FLOAT_T,PREC> operator/(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return ts::FloatingPoint<FLOAT_T,PREC>(x1) / x2; }

// This one explicitly calls the operator== method because without it, you get an error when built with C++20
// error: in C++20 this comparison calls the current function recursively with reversed arguments
template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator==(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2.operator==(x1); }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator!=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return !x2.operator==(x1); }
#endif

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator<=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 >= x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator>=(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 <= x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator<(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 > x1; }

template<typename NUM_T, typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_arithmetic<NUM_T>::value && std::is_floating_point<FLOAT_T>::value, int>::type = 0>
inline bool operator>(NUM_T x1, const ts::FloatingPoint<FLOAT_T,PREC>& x2) { return x2 < x1; }

//! @endcond


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
const ts::FloatingPoint<FLOAT_T,PREC,N> ts::FloatingPoint<FLOAT_T,PREC,N>::MIN(std::numeric_limits<FLOAT_T>::lowest());

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
const ts::FloatingPoint<FLOAT_T,PREC,N> ts::FloatingPoint<FLOAT_T,PREC,N>::MAX(std::numeric_limits<FLOAT_T>::max());

// Virtual numeric conversions.
template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
int64_t ts::FloatingPoint<FLOAT_T,PREC,N>::toInt64() const
{
    return int64_t(std::round(_value));
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
double ts::FloatingPoint<FLOAT_T,PREC,N>::toDouble() const
{
    return double(_value);
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
bool ts::FloatingPoint<FLOAT_T,PREC,N>::inRange(int64_t min, int64_t max) const
{
    return _value >= float_t(min) && _value <= float_t(max);
}

template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
ts::UString ts::FloatingPoint<FLOAT_T,PREC,N>::description() const
{
    return UString::Format(u"%d-bit floating-point value", {8 * sizeof(float_t)});
}

// Convert the number to a string object.
template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
ts::UString ts::FloatingPoint<FLOAT_T,PREC,N>::toString(size_t min_width,
                                                        bool right_justified,
                                                        UChar separator,
                                                        bool force_sign,
                                                        size_t decimals,
                                                        bool force_decimals,
                                                        UChar decimal_dot,
                                                        UChar pad) const
{
    // 6 decimal digits by default.
    if (decimals == NPOS) {
        decimals = DISPLAY_PRECISION;
    }

    // Format the floating point number in a slightly oversized UTF-8 buffer.
    std::string str8(std::numeric_limits<float_t>::max_digits10 + decimals + 10, '\0');
    std::snprintf(&str8[0], str8.length() - 1, "%.*lf", int(decimals), double(_value));

    // Work on UString from now on.
    UString str;
    str.assignFromUTF8(str8.c_str());
    Format(str, min_width, right_justified, separator, force_sign && _value >= 0,  decimals, force_decimals, decimal_dot, pad);
    return str;
}

// Parse a string and interpret it as a number.
template <typename FLOAT_T, const size_t PREC, typename std::enable_if<std::is_floating_point<FLOAT_T>::value, int>::type N>
bool ts::FloatingPoint<FLOAT_T,PREC,N>::fromString(const UString& str, UChar separator, UChar decimal_dot)
{
    UString str16(str);
    Deformat(str16, separator, decimal_dot);
    const std::string str8(str16.toUTF8());

    int len = 0;
    double d = 0.0;
    TS_PUSH_WARNING()
    TS_MSC_NOWARNING(4996) // 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead.
    const int count = std::sscanf(str8.c_str(), "%lf%n", &d, &len);
    TS_POP_WARNING()
    _value = float_t(d);
    return count == 1 && len == int(str8.length());
}
