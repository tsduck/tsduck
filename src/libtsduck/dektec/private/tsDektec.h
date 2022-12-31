//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Provide a safe way to include the DTAPI definition.
//!
//-----------------------------------------------------------------------------

#pragma once

#if defined(DOXYGEN)

//!
//! Externally defined when the DTAPI is not available.
//!
#define TS_NO_DTAPI

#elif defined(TS_NO_DTAPI)

// An error message to display.
#define TS_NO_DTAPI_MESSAGE u"This version of TSDuck was compiled without Dektec support"

// Replacement for DTAPI versions.
#define DTAPI_VERSION_MAJOR 0
#define DTAPI_VERSION_MINOR 0

#else // Dektec devices are supported.

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
    template <class FIXED,
              typename INT = typename FIXED::int_t,
              const size_t PREC = FIXED::PRECISION,
              typename std::enable_if<std::is_base_of<FixedPoint<INT,PREC>, FIXED>::value, int>::type = 0>
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
    template <class FIXED,
              typename INT = typename FIXED::int_t,
              const size_t PREC = FIXED::PRECISION,
              typename std::enable_if<std::is_base_of<FixedPoint<INT,PREC>, FIXED>::value, int>::type = 0>
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
    template <class FRAC,
              typename INT = typename FRAC::int_t,
              typename std::enable_if<std::is_base_of<Fraction<INT>, FRAC>::value, int>::type = 0>
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
    template <class FRAC,
              typename INT = typename FRAC::int_t,
              typename std::enable_if<std::is_base_of<Fraction<INT>, FRAC>::value, int>::type = 0>
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
    template <class INTEG,
              typename INT_T = typename INTEG::int_t,
              typename std::enable_if<std::is_base_of<Integer<INT_T>, INTEG>::value, int>::type = 0>
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
    template <class INTEG,
              typename INT_T = typename INTEG::int_t,
              typename std::enable_if<std::is_base_of<Integer<INT_T>, INTEG>::value, int>::type = 0>
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
    template <class FPOINT,
              typename FLOAT_T = typename FPOINT::float_t,
              const size_t PREC = FPOINT::DISPLAY_PRECISION,
              typename std::enable_if<std::is_base_of<FloatingPoint<FLOAT_T,PREC>, FPOINT>::value, int>::type = 0>
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
    template <class FPOINT,
              typename FLOAT_T = typename FPOINT::float_t,
              const size_t PREC = FPOINT::DISPLAY_PRECISION,
              typename std::enable_if<std::is_base_of<FloatingPoint<FLOAT_T,PREC>, FPOINT>::value, int>::type = 0>
    inline void FromDektecFractionInt(FPOINT& result, Dtapi::DtFractionInt value)
    {
        result = FPOINT(FLOAT_T(value.m_Num) / FLOAT_T(value.m_Den));
    }
}

#endif // TS_NO_DTAPI

//!
//! Define a synthetic major/minor version number for DTAPI
//!
#define TS_DTAPI_VERSION ((DTAPI_VERSION_MAJOR * 100) + (DTAPI_VERSION_MINOR % 100))
