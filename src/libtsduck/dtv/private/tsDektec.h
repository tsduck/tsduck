//-----------------------------------------------------------------------------
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

    // Replacement for symbols which are used in command definitions.
    // Unused in practice since DTAPI is not there.
    #define DTAPI_ATSC3_6MHZ             0
    #define DTAPI_ATSC3_7MHZ             0
    #define DTAPI_ATSC3_8MHZ             0
    #define DTAPI_CMMB_BW_2MHZ           0
    #define DTAPI_CMMB_BW_8MHZ           0
    #define DTAPI_DVBC2_6MHZ             0
    #define DTAPI_DVBC2_8MHZ             0
    #define DTAPI_DVBT2_10MHZ            0
    #define DTAPI_DVBT2_1_7MHZ           0
    #define DTAPI_DVBT2_5MHZ             0
    #define DTAPI_DVBT2_6MHZ             0
    #define DTAPI_DVBT2_7MHZ             0
    #define DTAPI_DVBT2_8MHZ             0
    #define DTAPI_DVBT2_BPSK             0
    #define DTAPI_DVBT2_COD_1_2          0
    #define DTAPI_DVBT2_COD_2_3          0
    #define DTAPI_DVBT2_COD_3_4          0
    #define DTAPI_DVBT2_COD_3_5          0
    #define DTAPI_DVBT2_COD_4_5          0
    #define DTAPI_DVBT2_COD_5_6          0
    #define DTAPI_DVBT2_FEF_1K_OFDM      0
    #define DTAPI_DVBT2_FEF_1K_OFDM_384  0
    #define DTAPI_DVBT2_FEF_ZERO         0
    #define DTAPI_DVBT2_FFT_16K          0
    #define DTAPI_DVBT2_FFT_1K           0
    #define DTAPI_DVBT2_FFT_2K           0
    #define DTAPI_DVBT2_FFT_32K          0
    #define DTAPI_DVBT2_FFT_4K           0
    #define DTAPI_DVBT2_FFT_8K           0
    #define DTAPI_DVBT2_GI_1_128         0
    #define DTAPI_DVBT2_GI_1_16          0
    #define DTAPI_DVBT2_GI_1_32          0
    #define DTAPI_DVBT2_GI_1_4           0
    #define DTAPI_DVBT2_GI_1_8           0
    #define DTAPI_DVBT2_GI_19_128        0
    #define DTAPI_DVBT2_GI_19_256        0
    #define DTAPI_DVBT2_IL_MULTI         0
    #define DTAPI_DVBT2_IL_ONETOONE      0
    #define DTAPI_DVBT2_ISSY_LONG        0
    #define DTAPI_DVBT2_ISSY_NONE        0
    #define DTAPI_DVBT2_ISSY_SHORT       0
    #define DTAPI_DVBT2_LDPC_16K         0
    #define DTAPI_DVBT2_LDPC_64K         0
    #define DTAPI_DVBT2_MISO_OFF         0
    #define DTAPI_DVBT2_MISO_TX1         0
    #define DTAPI_DVBT2_MISO_TX1TX2      0
    #define DTAPI_DVBT2_MISO_TX2         0
    #define DTAPI_DVBT2_PAPR_ACE         0
    #define DTAPI_DVBT2_PAPR_ACE_TR      0
    #define DTAPI_DVBT2_PAPR_NONE        0
    #define DTAPI_DVBT2_PAPR_TR          0
    #define DTAPI_DVBT2_PLP_TYPE_1       0
    #define DTAPI_DVBT2_PLP_TYPE_2       0
    #define DTAPI_DVBT2_PLP_TYPE_COMM    0
    #define DTAPI_DVBT2_PP_1             0
    #define DTAPI_DVBT2_PP_2             0
    #define DTAPI_DVBT2_PP_3             0
    #define DTAPI_DVBT2_PP_4             0
    #define DTAPI_DVBT2_PP_5             0
    #define DTAPI_DVBT2_PP_6             0
    #define DTAPI_DVBT2_PP_7             0
    #define DTAPI_DVBT2_PP_8             0
    #define DTAPI_DVBT2_PROFILE_BASE     0
    #define DTAPI_DVBT2_PROFILE_LITE     0
    #define DTAPI_DVBT2_QAM16            0
    #define DTAPI_DVBT2_QAM256           0
    #define DTAPI_DVBT2_QAM64            0
    #define DTAPI_DVBT2_QPSK             0
    #define DTAPI_IOCONFIG_MODHQ         0
    #define DTAPI_IOCONFIG_LOWPWR        0
    #define DTAPI_ISDBT_BW_5MHZ          0
    #define DTAPI_ISDBT_BW_6MHZ          0
    #define DTAPI_ISDBT_BW_7MHZ          0
    #define DTAPI_ISDBT_BW_8MHZ          0
    #define DTAPI_ISDBT_SEGM_1           0
    #define DTAPI_ISDBT_SEGM_13          0
    #define DTAPI_ISDBT_SEGM_3           0
    #define DTAPI_LED_BLUE               0
    #define DTAPI_LED_GREEN              0
    #define DTAPI_LED_HARDWARE           0
    #define DTAPI_LED_OFF                0
    #define DTAPI_LED_RED                0
    #define DTAPI_LED_YELLOW             0
    #define DTAPI_MOD_1_2                0
    #define DTAPI_MOD_1_3                0
    #define DTAPI_MOD_1_4                0
    #define DTAPI_MOD_2_3                0
    #define DTAPI_MOD_2_5                0
    #define DTAPI_MOD_3_4                0
    #define DTAPI_MOD_3_5                0
    #define DTAPI_MOD_4_5                0
    #define DTAPI_MOD_5_6                0
    #define DTAPI_MOD_6_7                0
    #define DTAPI_MOD_7_8                0
    #define DTAPI_MOD_8_9                0
    #define DTAPI_MOD_9_10               0
    #define DTAPI_MOD_ADTBT              0
    #define DTAPI_MOD_ATSC               0
    #define DTAPI_MOD_ATSC3              0
    #define DTAPI_MOD_ATSC_VSB16         0
    #define DTAPI_MOD_ATSC_VSB8          0
    #define DTAPI_MOD_CMMB               0
    #define DTAPI_MOD_CR_AUTO            0
    #define DTAPI_MOD_DAB                0
    #define DTAPI_MOD_DMBTH              0
    #define DTAPI_MOD_DTMB_0_4           0
    #define DTAPI_MOD_DTMB_0_6           0
    #define DTAPI_MOD_DTMB_0_8           0
    #define DTAPI_MOD_DTMB_5MHZ          0
    #define DTAPI_MOD_DTMB_6MHZ          0
    #define DTAPI_MOD_DTMB_7MHZ          0
    #define DTAPI_MOD_DTMB_8MHZ          0
    #define DTAPI_MOD_DTMB_IL_1          0
    #define DTAPI_MOD_DTMB_IL_2          0
    #define DTAPI_MOD_DTMB_PN420         0
    #define DTAPI_MOD_DTMB_PN595         0
    #define DTAPI_MOD_DTMB_PN945         0
    #define DTAPI_MOD_DTMB_QAM16         0
    #define DTAPI_MOD_DTMB_QAM32         0
    #define DTAPI_MOD_DTMB_QAM4          0
    #define DTAPI_MOD_DTMB_QAM4NR        0
    #define DTAPI_MOD_DTMB_QAM64         0
    #define DTAPI_MOD_DVBC2              0
    #define DTAPI_MOD_DVBS2_16APSK       0
    #define DTAPI_MOD_DVBS2_32APSK       0
    #define DTAPI_MOD_DVBS2_8PSK         0
    #define DTAPI_MOD_DVBS2_QPSK         0
    #define DTAPI_MOD_DVBS_BPSK          0
    #define DTAPI_MOD_DVBS_QPSK          0
    #define DTAPI_MOD_DVBT               0
    #define DTAPI_MOD_DVBT2              0
    #define DTAPI_MOD_DVBT_2K            0
    #define DTAPI_MOD_DVBT_4K            0
    #define DTAPI_MOD_DVBT_5MHZ          0
    #define DTAPI_MOD_DVBT_6MHZ          0
    #define DTAPI_MOD_DVBT_7MHZ          0
    #define DTAPI_MOD_DVBT_8K            0
    #define DTAPI_MOD_DVBT_8MHZ          0
    #define DTAPI_MOD_DVBT_CO_AUTO       0
    #define DTAPI_MOD_DVBT_G_1_16        0
    #define DTAPI_MOD_DVBT_G_1_32        0
    #define DTAPI_MOD_DVBT_G_1_4         0
    #define DTAPI_MOD_DVBT_G_1_8         0
    #define DTAPI_MOD_DVBT_GU_AUTO       0
    #define DTAPI_MOD_DVBT_MD_AUTO       0
    #define DTAPI_MOD_DVBT_QAM16         0
    #define DTAPI_MOD_DVBT_QAM64         0
    #define DTAPI_MOD_DVBT_QPSK          0
    #define DTAPI_MOD_ISDBT              0
    #define DTAPI_MOD_J83_A              0
    #define DTAPI_MOD_J83_B              0
    #define DTAPI_MOD_J83_C              0
    #define DTAPI_MOD_QAM128             0
    #define DTAPI_MOD_QAM16              0
    #define DTAPI_MOD_QAM256             0
    #define DTAPI_MOD_QAM32              0
    #define DTAPI_MOD_QAM4               0
    #define DTAPI_MOD_QAM64              0
    #define DTAPI_MOD_QAM_AUTO           0
    #define DTAPI_MOD_QAMB_I128_J1       0
    #define DTAPI_MOD_QAMB_I128_J1D      0
    #define DTAPI_MOD_QAMB_I128_J2       0
    #define DTAPI_MOD_QAMB_I128_J3       0
    #define DTAPI_MOD_QAMB_I128_J4       0
    #define DTAPI_MOD_QAMB_I128_J5       0
    #define DTAPI_MOD_QAMB_I128_J6       0
    #define DTAPI_MOD_QAMB_I128_J7       0
    #define DTAPI_MOD_QAMB_I128_J8       0
    #define DTAPI_MOD_QAMB_I16_J8        0
    #define DTAPI_MOD_QAMB_I32_J4        0
    #define DTAPI_MOD_QAMB_I64_J2        0
    #define DTAPI_MOD_QAMB_I8_J16        0
    #define DTAPI_MOD_QAMB_IL_AUTO       0
    #define DTAPI_IOCONFIG_12GSDI        0
    #define DTAPI_IOCONFIG_3GSDI         0
    #define DTAPI_IOCONFIG_6GSDI         0
    #define DTAPI_IOCONFIG_ASI           0
    #define DTAPI_IOCONFIG_AVENC         0
    #define DTAPI_IOCONFIG_DEKTECST      0
    #define DTAPI_IOCONFIG_DEMOD         0
    #define DTAPI_IOCONFIG_GPSTIME       0
    #define DTAPI_IOCONFIG_HDMI          0
    #define DTAPI_IOCONFIG_HDSDI         0
    #define DTAPI_IOCONFIG_IFADC         0
    #define DTAPI_IOCONFIG_IP            0
    #define DTAPI_IOCONFIG_MOD           0
    #define DTAPI_IOCONFIG_PHASENOISE    0
    #define DTAPI_IOCONFIG_RS422         0
    #define DTAPI_IOCONFIG_SDIRX         0
    #define DTAPI_IOCONFIG_SDI           0
    #define DTAPI_IOCONFIG_SPI           0
    #define DTAPI_IOCONFIG_SPISDI        0
    #define DTAPI_IOCONFIG_2160P50       0
    #define DTAPI_IOCONFIG_2160P50B      0
    #define DTAPI_IOCONFIG_2160P59_94    0
    #define DTAPI_IOCONFIG_2160P59_94B   0
    #define DTAPI_IOCONFIG_2160P60       0
    #define DTAPI_IOCONFIG_2160P60B      0
    #define DTAPI_IOCONFIG_1080P50       0
    #define DTAPI_IOCONFIG_1080P50B      0
    #define DTAPI_IOCONFIG_1080P59_94    0
    #define DTAPI_IOCONFIG_1080P59_94B   0
    #define DTAPI_IOCONFIG_1080P60       0
    #define DTAPI_IOCONFIG_1080P60B      0
    #define DTAPI_IOCONFIG_2160P23_98    0
    #define DTAPI_IOCONFIG_2160P24       0
    #define DTAPI_IOCONFIG_2160P25       0
    #define DTAPI_IOCONFIG_2160P29_97    0
    #define DTAPI_IOCONFIG_2160P30       0
    #define DTAPI_IOCONFIG_1080I50       0
    #define DTAPI_IOCONFIG_1080I59_94    0
    #define DTAPI_IOCONFIG_1080I60       0
    #define DTAPI_IOCONFIG_1080P23_98    0
    #define DTAPI_IOCONFIG_1080P24       0
    #define DTAPI_IOCONFIG_1080P25       0
    #define DTAPI_IOCONFIG_1080P29_97    0
    #define DTAPI_IOCONFIG_1080P30       0
    #define DTAPI_IOCONFIG_1080PSF23_98  0
    #define DTAPI_IOCONFIG_1080PSF24     0
    #define DTAPI_IOCONFIG_1080PSF25     0
    #define DTAPI_IOCONFIG_1080PSF29_97  0
    #define DTAPI_IOCONFIG_1080PSF30     0
    #define DTAPI_IOCONFIG_720P23_98     0
    #define DTAPI_IOCONFIG_720P24        0
    #define DTAPI_IOCONFIG_720P25        0
    #define DTAPI_IOCONFIG_720P29_97     0
    #define DTAPI_IOCONFIG_720P30        0
    #define DTAPI_IOCONFIG_720P50        0
    #define DTAPI_IOCONFIG_720P59_94     0
    #define DTAPI_IOCONFIG_720P60        0
    #define DTAPI_IOCONFIG_525I59_94     0
    #define DTAPI_IOCONFIG_625I50        0
    #define DTAPI_IOCONFIG_SPI525I59_94  0
    #define DTAPI_IOCONFIG_SPI625I50     0
    #define DTAPI_FEC_DISABLE            0
    #define DTAPI_FEC_2D_M1              0
    #define DTAPI_FEC_2D_M1_B            0
    #define DTAPI_FEC_2D_M2              0
    #define DTAPI_FEC_2D_M2_B            0

#else

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
            return Dtapi::DtFractionInt(int(value.raw()), int(FIXED::FACTOR));
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
            else {
                // Use 1/100 precision (arbitrary).
                return Dtapi::DtFractionInt(int(100.0 * value.toDouble()), 100);
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
                  typename std::enable_if<std::is_base_of<FloatingPoint<FLOAT_T>, FPOINT>::value, int>::type = 0>
        inline Dtapi::DtFractionInt ToDektecFractionInt(const FPOINT& value)
        {
            // Use 1/100 precision (arbitrary).
            return Dtapi::DtFractionInt(int(100.0 * value.toDouble()), 100);
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
                  typename std::enable_if<std::is_base_of<FloatingPoint<FLOAT_T>, FPOINT>::value, int>::type = 0>
        inline void FromDektecFractionInt(FPOINT& result, Dtapi::DtFractionInt value)
        {
            result = FPOINT(FLOAT_T(value.m_Num) / FLOAT_T(value.m_Den));
        }
    }

#endif

//!
//! Define a synthetic major/minor version number for DTAPI
//!
#define TS_DTAPI_VERSION ((DTAPI_VERSION_MAJOR * 100) + (DTAPI_VERSION_MINOR % 100))
