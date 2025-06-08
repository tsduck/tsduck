//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Provide a safe way to include the DTAPI definition.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsInteger.h"
#include "tsFraction.h"
#include "tsFixedPoint.h"
#include "tsFloatingPoint.h"

// The DTAPI header triggers some warnings, ignore them.
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4263)
TS_MSC_NOWARNING(4264)
TS_MSC_NOWARNING(4265)
TS_MSC_NOWARNING(4266)
TS_MSC_NOWARNING(4625)
TS_MSC_NOWARNING(4626)
TS_MSC_NOWARNING(5026)
TS_MSC_NOWARNING(5027)
TS_MSC_NOWARNING(5204)
TS_GCC_NOWARNING(non-virtual-dtor)
TS_GCC_NOWARNING(overloaded-virtual)
TS_GCC_NOWARNING(zero-as-null-pointer-constant)
TS_LLVM_NOWARNING(non-virtual-dtor)
TS_LLVM_NOWARNING(overloaded-virtual)
TS_LLVM_NOWARNING(zero-as-null-pointer-constant)
#define _NO_USING_NAMESPACE_DTAPI
#include "DTAPI.h"
TS_POP_WARNING()

namespace ts {
    //!
    //! Convert a FixedPoint value into a Dektec-defined fractional int.
    //! @tparam FIXED An instantiation of FixedPoint. All other template parameters are here
    //! to enforce a fixed-point type and should be left to their default values.
    //! @param [in] value A FixedPoint value.
    //! @return Corresponding Dektec-defined fractional int.
    //!
    template <class FIXED, typename INT = typename FIXED::int_t, const size_t PREC = FIXED::PRECISION>
        requires std::derived_from<FIXED, FixedPoint<INT,PREC>>
    Dtapi::DtFractionInt ToDektecFractionInt(FIXED value)
    {
        // DtFractionInt uses "int" members. We may use larger types in our fraction type.
        if (bound_check<int>(value.raw())) {
            return Dtapi::DtFractionInt(int(value.raw()), int(FIXED::FACTOR));
        }
        else if (FIXED::PRECISION > 1 && bound_check<int>(value.raw() / 10)) {
            // Too large, divide precision by 10.
            return Dtapi::DtFractionInt(int(value.raw() / 10), int(FIXED::FACTOR / 10));
        }
        else {
            // Too large, drop precision.
            return Dtapi::DtFractionInt(int(value.toInt()), 1);
        }
    }

    //!
    //! Convert a Dektec-defined fractional integer into a FixedPoint value.
    //! @tparam FIXED An instantiation of FixedPoint. All other template parameters are here
    //! to enforce a fixed-point type and should be left to their default values.
    //! @param [out] result The converted FixedPoint value.
    //! @param [in] value A Dektec-defined fractional int.
    //!
    template <class FIXED, typename INT = typename FIXED::int_t, const size_t PREC = FIXED::PRECISION>
        requires std::derived_from<FIXED, FixedPoint<INT,PREC>>
    void FromDektecFractionInt(FIXED& result, Dtapi::DtFractionInt value)
    {
        result = value.m_Num;
        result /= value.m_Den;
    }

    //!
    //! Convert a Fraction value into a Dektec-defined fractional int.
    //! @tparam FRAC An instantiation of Fraction. All other template parameters are here
    //! to enforce a fraction type and should be left to their default values.
    //! @param [in] value A FixedPoint value.
    //! @return Corresponding Dektec-defined fractional int.
    //!
    template <class FRAC, typename INT = typename FRAC::int_t>
        requires std::derived_from<FRAC, Fraction<INT>>
    Dtapi::DtFractionInt ToDektecFractionInt(const FRAC& value)
    {
        // DtFractionInt uses "int" members. We may use larger types in our fraction type.
        if (bound_check<int>(value.numerator()) && bound_check<int>(value.denominator())) {
            return Dtapi::DtFractionInt(int(value.numerator()), int(value.denominator()));
        }
        else if (bound_check<int>(int64_t(100.0 * value.toDouble()))) {
            // 1/100 precision fits.
            return Dtapi::DtFractionInt(int(100.0 * value.toDouble()), 100);
        }
        else {
            // Too large, drop precision.
            return Dtapi::DtFractionInt(int(value.toInt()), 1);
        }
    }

    //!
    //! Convert a Dektec-defined fractional integer into a Fraction value.
    //! @tparam FRAC An instantiation of Fraction. All other template parameters are here
    //! to enforce a fraction type and should be left to their default values.
    //! @param [out] result The converted FixedPoint value.
    //! @param [in] value A Dektec-defined fractional int.
    //!
    template <class FRAC, typename INT = typename FRAC::int_t>
        requires std::derived_from<FRAC, Fraction<INT>>
    void FromDektecFractionInt(FRAC& result, Dtapi::DtFractionInt value)
    {
        result = FRAC(value.m_Num, value.m_Den);
    }

    //!
    //! Convert an Integer value into a Dektec-defined fractional int.
    //! @tparam INTEG An instantiation of Integer. All other template parameters are here
    //! to enforce an Integer type and should be left to their default values.
    //! @param [in] value An Integer value.
    //! @return Corresponding Dektec-defined fractional int.
    //!
    template <class INTEG, typename INT_T = typename INTEG::int_t>
        requires std::derived_from<INTEG, Integer<INT_T>>
    inline Dtapi::DtFractionInt ToDektecFractionInt(const INTEG& value)
    {
        return Dtapi::DtFractionInt(int(value.toInt()), 1);
    }

    //!
    //! Convert a Dektec-defined fractional integer into an Integer value.
    //! @tparam INTEG An instantiation of Integer. All other template parameters are here
    //! to enforce an Integer type and should be left to their default values.
    //! @param [out] result The converted Integer value.
    //! @param [in] value A Dektec-defined fractional int.
    //!
    template <class INTEG, typename INT_T = typename INTEG::int_t>
        requires std::derived_from<INTEG, Integer<INT_T>>
    inline void FromDektecFractionInt(INTEG& result, Dtapi::DtFractionInt value)
    {
        result = INTEG(rounded_div(value.m_Num, value.m_Den));
    }

    //!
    //! Convert a FloatingPoint value into a Dektec-defined fractional int.
    //! @tparam FPOINT An instantiation of FloatingPoint. All other template parameters are here
    //! to enforce a FloatingPoint type and should be left to their default values.
    //! @param [in] value A FloatingPoint value.
    //! @return Corresponding Dektec-defined fractional int.
    //!
    template <class FPOINT, typename FLOAT_T = typename FPOINT::float_t, const size_t PREC = FPOINT::DISPLAY_PRECISION>
        requires std::derived_from<FPOINT, FloatingPoint<FLOAT_T,PREC>>
    inline Dtapi::DtFractionInt ToDektecFractionInt(const FPOINT& value)
    {
        constexpr int factor = static_power10<int,PREC>::value;
        const uint64_t val = uint64_t(double(factor) * value.toDouble());
        if (bound_check<int>(val)) {
            // Display precision fits.
            return Dtapi::DtFractionInt(int(val), factor);
        }
        else {
            // Too large, drop precision.
            return Dtapi::DtFractionInt(int(value.toInt()), 1);
        }
    }

    //!
    //! Convert a Dektec-defined fractional integer into a FloatingPoint value.
    //! @tparam FPOINT An instantiation of FloatingPoint. All other template parameters are here
    //! to enforce a FloatingPoint type and should be left to their default values.
    //! @param [out] result The converted FloatingPoint value.
    //! @param [in] value A Dektec-defined fractional int.
    //!
    template <class FPOINT, typename FLOAT_T = typename FPOINT::float_t, const size_t PREC = FPOINT::DISPLAY_PRECISION>
        requires std::derived_from<FPOINT, FloatingPoint<FLOAT_T,PREC>>
    inline void FromDektecFractionInt(FPOINT& result, Dtapi::DtFractionInt value)
    {
        result = FPOINT(FLOAT_T(value.m_Num) / FLOAT_T(value.m_Den));
    }
}

//!
//! Define a synthetic major/minor version number for DTAPI
//!
#define TS_DTAPI_VERSION ((DTAPI_VERSION_MAJOR * 100) + (DTAPI_VERSION_MINOR % 100))
