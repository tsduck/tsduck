//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup hardware
//!  Definition for MPEG transport modulations
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsReport.h"
#include "tsSingleton.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <bdatypes.h>
    #include "tsAfterStandardHeaders.h"
#elif defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <linux/dvb/frontend.h>
    #include <linux/version.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Check if an enumeration value is supported by the native implementation.
    //!
    //! The file @link tsModulation.h @endlink declares several enumeration types
    //! relating to modulation features. Whenever possible, all enumerations have
    //! identical integer values as their counterparts in the Linux DVB or Windows
    //! DirectShow API for faster conversion. When an enum cannot be mapped to a
    //! native value (because the feature is not supported by the operating system),
    //! a "very negative" value is used. Very negative means "-10 or less" since
    //! these values are never used by an implementation.
    //!
    //! This function checks that an enumeration value is supported by
    //! the native implementation. If it is not, report an error message
    //! and return false.
    //!
    //! @param [in] value The @c int value of an enumeration value from on the
    //! enumeration types in file @link tsModulation.h @endlink.
    //! @param [in] name The name of the feature or enumeration type (eg.
    //! "FEC", "guard interval", etc.) Used to report errors.
    //! @param [in] conv The ts::Enumeration instance for the enumeration type.
    //! Used to report errors.
    //! @param [in] report Where to report errors.
    //! @return True if @a value is supported on the operating system. False if
    //! the feature is not supported. In this case, an error message is reported
    //! to @a report.
    //!
    TSDUCKDLL bool CheckModEnum(int value, const UString& name, const Enumeration& conv, Report& report);

    //!
    //! Check if an optional enumeration value is supported by the native implementation.
    //!
    //! @tparam ENUM An integer or enumeration type for the vallue to check.
    //! @param [in] value A variable object containing an enumeration value from on the
    //! enumeration types in file @link tsModulation.h @endlink.
    //! @param [in] name The name of the feature or enumeration type (eg.
    //! "FEC", "guard interval", etc.) Used to report errors.
    //! @param [in] conv The ts::Enumeration instance for the enumeration type.
    //! Used to report errors.
    //! @param [in] report Where to report errors.
    //! @return True if either @a value is not set or its value is supported on the operating system.
    //! False if the value is set and the feature is not supported. In this case, an error message is reported
    //! to @a report.
    //! @see CheckModEnum()
    //!
    template <typename ENUM> requires std::integral<ENUM> || std::is_enum_v<ENUM>
    bool CheckModVar(const std::optional<ENUM>& value, const UString& name, const Enumeration& conv, Report& report)
    {
        return !value.has_value() || CheckModEnum(int(value.value()), name, conv, report);
    }

    //!
    //! Modulation types.
    //! Support depends on tuner types.
    //!
    enum Modulation {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        QPSK      = ::QPSK,
        PSK_8     = ::PSK_8,
        QAM_AUTO  = ::QAM_AUTO,
        QAM_16    = ::QAM_16,
        QAM_32    = ::QAM_32,
        QAM_64    = ::QAM_64,
        QAM_128   = ::QAM_128,
        QAM_256   = ::QAM_256,
        VSB_8     = ::VSB_8,
        VSB_16    = ::VSB_16,
        APSK_16   = ::APSK_16,
        APSK_32   = ::APSK_32,
        DQPSK     = ::DQPSK,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        QAM_4_NR  = -14,
    #else
        QAM_4_NR  = ::QAM_4_NR,
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
        QAM_1024  = -15,
        QAM_4096  = -16,
        APSK_8_L  = -17,
        APSK_16_L = -18,
        APSK_32_L = -19,
        APSK_64   = -20,
        APSK_64_L = -21,
    #else
        QAM_1024  = ::QAM_1024,
        QAM_4096  = ::QAM_4096,
        APSK_8_L  = ::APSK_8_L,
        APSK_16_L = ::APSK_16_L,
        APSK_32_L = ::APSK_32_L,
        APSK_64   = ::APSK_64,
        APSK_64_L = ::APSK_64_L,
    #endif
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        QPSK      = ::BDA_MOD_QPSK,
        PSK_8     = ::BDA_MOD_8PSK,
        QAM_AUTO  = ::BDA_MOD_NOT_DEFINED,
        QAM_16    = ::BDA_MOD_16QAM,
        QAM_32    = ::BDA_MOD_32QAM,
        QAM_64    = ::BDA_MOD_64QAM,
        QAM_128   = ::BDA_MOD_128QAM,
        QAM_256   = ::BDA_MOD_256QAM,
        VSB_8     = ::BDA_MOD_8VSB,
        VSB_16    = ::BDA_MOD_16VSB,
        APSK_16   = ::BDA_MOD_16APSK,
        APSK_32   = ::BDA_MOD_32APSK,
        DQPSK     = -10,
        QAM_4_NR  = -11,
        QAM_1024  = -12,
        QAM_4096  = -13,
        APSK_8_L  = -14,
        APSK_16_L = -15,
        APSK_32_L = -16,
        APSK_64   = -17,
        APSK_64_L = -18,
#else
        QPSK,       //!< QPSK (4-PSK, DVB-S).
        PSK_8,      //!< 8-PSK (DVB-S2).
        QAM_AUTO,   //!< Unspecified QAM.
        QAM_16,     //!< 16-QAM.
        QAM_32,     //!< 32-QAM.
        QAM_64,     //!< 64-QAM.
        QAM_128,    //!< 128-QAM.
        QAM_256,    //!< 256-QAM.
        VSB_8,      //!< 8-VSB.
        VSB_16,     //!< 16-VSB.
        APSK_16,    //!< 16-APSK (DVB-S2).
        APSK_32,    //!< 32-APSK (DVB-S2).
        DQPSK,      //!< DQPSK,
        QAM_4_NR,   //!< 4-QAM-NR,
        QAM_1024,   //!< 1024-QAM
        QAM_4096,   //!< 4096-QAM
        APSK_8_L,   //!< 8-APSK-L
        APSK_16_L,  //!< 16-APSK-L
        APSK_32_L,  //!< 32-APSK-L
        APSK_64,    //!< 64-APSK
        APSK_64_L,  //!< 64-APSK-L
#endif
    };

    //!
    //! Enumeration description of ts::Modulation.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, ModulationEnum);

    //!
    //! Compute the number of bits per symbol for a specified modulation.
    //! @param [in] mod Modulation type.
    //! @return Number of bits per symbol or zero if unknown.
    //!
    TSDUCKDLL uint32_t BitsPerSymbol(Modulation mod);

    //!
    //! Spectral inversion.
    //!
    enum SpectralInversion {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        SPINV_OFF  = ::INVERSION_OFF,
        SPINV_ON   = ::INVERSION_ON,
        SPINV_AUTO = ::INVERSION_AUTO,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        SPINV_OFF  = ::BDA_SPECTRAL_INVERSION_NORMAL,
        SPINV_ON   = ::BDA_SPECTRAL_INVERSION_INVERTED,
        SPINV_AUTO = ::BDA_SPECTRAL_INVERSION_AUTOMATIC,
#else
        SPINV_OFF,   //!< Inversion off.
        SPINV_ON,    //!< Inversion on.
        SPINV_AUTO,  //!< Automatic spectral inversion.
#endif
    };

    //!
    //! Enumeration description of ts::SpectralInversion.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, SpectralInversionEnum);

    //!
    //! Inner Forward Error Correction
    //!
    enum InnerFEC {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        FEC_NONE  = ::FEC_NONE,
        FEC_AUTO  = ::FEC_AUTO,
        FEC_1_2   = ::FEC_1_2,
        FEC_2_3   = ::FEC_2_3,
        FEC_3_4   = ::FEC_3_4,
        FEC_4_5   = ::FEC_4_5,
        FEC_5_6   = ::FEC_5_6,
        FEC_6_7   = ::FEC_6_7,
        FEC_7_8   = ::FEC_7_8,
        FEC_8_9   = ::FEC_8_9,
        FEC_9_10  = ::FEC_9_10,
        FEC_3_5   = ::FEC_3_5,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
        FEC_1_3   = -12,
        FEC_1_4   = -13,
    #else
        FEC_1_3   = ::FEC_1_3,
        FEC_1_4   = ::FEC_1_4,
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        FEC_2_5   = -14,
    #else
        FEC_2_5   = ::FEC_2_5,
    #endif
        FEC_5_11  = -15,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
        FEC_5_9   = -16,
        FEC_7_9   = -17,
        FEC_8_15  = -18,
        FEC_11_15 = -19,
        FEC_13_18 = -20,
        FEC_9_20  = -21,
        FEC_11_20 = -22,
        FEC_23_36 = -23,
        FEC_25_36 = -24,
        FEC_13_45 = -25,
        FEC_26_45 = -26,
        FEC_28_45 = -27,
        FEC_32_45 = -28,
        FEC_77_90 = -29,
    #else
        FEC_5_9   = ::FEC_5_9,
        FEC_7_9   = ::FEC_7_9,
        FEC_8_15  = ::FEC_8_15,
        FEC_11_15 = ::FEC_11_15,
        FEC_13_18 = ::FEC_13_18,
        FEC_9_20  = ::FEC_9_20,
        FEC_11_20 = ::FEC_11_20,
        FEC_23_36 = ::FEC_23_36,
        FEC_25_36 = ::FEC_25_36,
        FEC_13_45 = ::FEC_13_45,
        FEC_26_45 = ::FEC_26_45,
        FEC_28_45 = ::FEC_28_45,
        FEC_32_45 = ::FEC_32_45,
        FEC_77_90 = ::FEC_77_90,
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,5,0)
        FEC_11_45 = -30,
        FEC_4_15  = -31,
        FEC_14_45 = -32,
        FEC_7_15  = -33,
    #else
        FEC_11_45 = ::FEC_11_45,
        FEC_4_15  = ::FEC_4_15,
        FEC_14_45 = ::FEC_14_45,
        FEC_7_15  = ::FEC_7_15,
    #endif
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        FEC_NONE  = ::BDA_BCC_RATE_NOT_SET,
        FEC_AUTO  = ::BDA_BCC_RATE_NOT_DEFINED,
        FEC_1_2   = ::BDA_BCC_RATE_1_2,
        FEC_2_3   = ::BDA_BCC_RATE_2_3,
        FEC_3_4   = ::BDA_BCC_RATE_3_4,
        FEC_4_5   = ::BDA_BCC_RATE_4_5,
        FEC_5_6   = ::BDA_BCC_RATE_5_6,
        FEC_6_7   = ::BDA_BCC_RATE_6_7,
        FEC_7_8   = ::BDA_BCC_RATE_7_8,
        FEC_8_9   = ::BDA_BCC_RATE_8_9,
        FEC_9_10  = ::BDA_BCC_RATE_9_10,
        FEC_3_5   = ::BDA_BCC_RATE_3_5,
        FEC_1_3   = ::BDA_BCC_RATE_1_3,
        FEC_1_4   = ::BDA_BCC_RATE_1_4,
        FEC_2_5   = ::BDA_BCC_RATE_2_5,
        FEC_5_11  = ::BDA_BCC_RATE_5_11,
        FEC_5_9   = -12,
        FEC_7_9   = -13,
        FEC_8_15  = -14,
        FEC_11_15 = -15,
        FEC_13_18 = -16,
        FEC_9_20  = -17,
        FEC_11_20 = -18,
        FEC_23_36 = -19,
        FEC_25_36 = -20,
        FEC_13_45 = -21,
        FEC_26_45 = -22,
        FEC_28_45 = -23,
        FEC_32_45 = -24,
        FEC_77_90 = -25,
        FEC_11_45 = -26,
        FEC_4_15  = -27,
        FEC_14_45 = -28,
        FEC_7_15  = -29,
#else
        FEC_NONE,   //!< No FEC.
        FEC_AUTO,   //!< Automatic FEC, unspecified.
        FEC_1_2,    //!< FEC 1/2.
        FEC_2_3,    //!< FEC 2/3.
        FEC_3_4,    //!< FEC 3/4.
        FEC_4_5,    //!< FEC 4/5.
        FEC_5_6,    //!< FEC 5/6.
        FEC_6_7,    //!< FEC 6/7.
        FEC_7_8,    //!< FEC 7/8.
        FEC_8_9,    //!< FEC 8/9.
        FEC_9_10,   //!< FEC 9/10.
        FEC_3_5,    //!< FEC 3/5.
        FEC_1_3,    //!< FEC 1/3.
        FEC_1_4,    //!< FEC 1/4.
        FEC_2_5,    //!< FEC 2/5.
        FEC_5_11,   //!< FEC 5/11.
        FEC_5_9,    //!< FEC 5/9.
        FEC_7_9,    //!< FEC 7/9.
        FEC_8_15,   //!< FEC 8/15.
        FEC_11_15,  //!< FEC 11/15.
        FEC_13_18,  //!< FEC 13/18.
        FEC_9_20,   //!< FEC 9/20.
        FEC_11_20,  //!< FEC 11/20.
        FEC_23_36,  //!< FEC 23/36.
        FEC_25_36,  //!< FEC 25/36.
        FEC_13_45,  //!< FEC 13/45.
        FEC_26_45,  //!< FEC 26/45.
        FEC_28_45,  //!< FEC 28/45.
        FEC_32_45,  //!< FEC 32/45.
        FEC_77_90,  //!< FEC 77/90.
        FEC_11_45,  //!< FEC 11/45.
        FEC_4_15,   //!< FEC 4/15.
        FEC_14_45,  //!< FEC 14/45.
        FEC_7_15,   //!< FEC 7/15.
#endif
    };

    //!
    //! Enumeration description of ts::InnerFEC.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, InnerFECEnum);

    //!
    //! Compute the multiplier of a FEC value.
    //! @param [in] fec Inner FEC value.
    //! @return The multiplier (eg. 9 for FEC_9_10) or zero if unknown.
    //!
    TSDUCKDLL uint32_t FECMultiplier(InnerFEC fec);

    //!
    //! Compute the divider of a FEC value.
    //! @param [in] fec Inner FEC value.
    //! @return The divider (eg. 10 for FEC_9_10) or zero if unknown.
    //!
    TSDUCKDLL uint32_t FECDivider(InnerFEC fec);

    //!
    //! Polarization
    //!
    enum Polarization {
#if defined(TS_WINDOWS) && !defined(DOXYGEN)
        POL_NONE       = ::BDA_POLARISATION_NOT_SET,
        POL_AUTO       = ::BDA_POLARISATION_NOT_DEFINED,
        POL_HORIZONTAL = ::BDA_POLARISATION_LINEAR_H,
        POL_VERTICAL   = ::BDA_POLARISATION_LINEAR_V,
        POL_LEFT       = ::BDA_POLARISATION_CIRCULAR_L,
        POL_RIGHT      = ::BDA_POLARISATION_CIRCULAR_R,
#else
        POL_NONE,        //!< Polarization not set.
        POL_AUTO,        //!< Polarization automatically set.
        POL_HORIZONTAL,  //!< Horizontal linear polarization.
        POL_VERTICAL,    //!< Vertical linear polarization.
        POL_LEFT,        //!< Left circular polarization.
        POL_RIGHT,       //!< Right circular polarization.
#endif
    };

    //!
    //! Enumeration description of ts::Polarization.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, PolarizationEnum);

    //!
    //! Pilot (DVB-S2)
    //!
    enum Pilot {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        PILOT_AUTO = ::PILOT_AUTO,
        PILOT_ON   = ::PILOT_ON,
        PILOT_OFF  = ::PILOT_OFF,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        PILOT_AUTO = ::BDA_PILOT_NOT_DEFINED,
        PILOT_ON   = ::BDA_PILOT_ON,
        PILOT_OFF  = ::BDA_PILOT_OFF,
#else
        PILOT_AUTO,  //!< Pilot automatically set.
        PILOT_ON,    //!< Pilot on.
        PILOT_OFF,   //!< Pilot off.
#endif
    };

    //!
    //! Enumeration description of ts::Pilot.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, PilotEnum);

    //!
    //! Roll-off (DVB-S2)
    //!
    enum RollOff {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        ROLLOFF_AUTO = ::ROLLOFF_AUTO,
        ROLLOFF_35   = ::ROLLOFF_35,
        ROLLOFF_25   = ::ROLLOFF_25,
        ROLLOFF_20   = ::ROLLOFF_20,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
        ROLLOFF_15   = -10,
        ROLLOFF_10   = -11,
        ROLLOFF_5    = -12,
    #else
        ROLLOFF_15   = ::ROLLOFF_15,
        ROLLOFF_10   = ::ROLLOFF_10,
        ROLLOFF_5    = ::ROLLOFF_5,
    #endif
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        ROLLOFF_AUTO = ::BDA_ROLL_OFF_NOT_DEFINED,
        ROLLOFF_35   = ::BDA_ROLL_OFF_35,
        ROLLOFF_25   = ::BDA_ROLL_OFF_25,
        ROLLOFF_20   = ::BDA_ROLL_OFF_20,
        ROLLOFF_15   = -10,
        ROLLOFF_10   = -11,
        ROLLOFF_5    = -12,
#else
        ROLLOFF_AUTO,  //!< Automatic rolloff.
        ROLLOFF_35,    //!< Rolloff 0.35, implied in DVB-S, default in DVB-S2.
        ROLLOFF_25,    //!< Rolloff 0.25.
        ROLLOFF_20,    //!< Rolloff 0.20.
        ROLLOFF_15,    //!< Rolloff 0.15.
        ROLLOFF_10,    //!< Rolloff 0.10.
        ROLLOFF_5,     //!< Rolloff 0.05.
#endif
    };

    //!
    //! Enumeration description of ts::RollOff.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, RollOffEnum);

    //!
    //! Bandwidth (OFDM, DVB-T/T2)
    //!
    //! Legacy issue: The bandwith type for DVB-T/T2 and ISDB-T used to be an enum type
    //! with a few values (BW_AUTO, BW_8_MHZ, etc.). This was a legacy from the Linux
    //! DVB API version 3. The bandwidth is now a 32-bit unsigned integer containing
    //! a value in Hz. The former enum values are redefined as constants in header
    //! file tsLegacyBandWidth.h.
    //!
    using BandWidth = uint32_t;

    //!
    //! Transmission mode (OFDM)
    //!
    enum TransmissionMode {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        TM_AUTO  = ::TRANSMISSION_MODE_AUTO,
        TM_2K    = ::TRANSMISSION_MODE_2K,
        TM_4K    = ::TRANSMISSION_MODE_4K,
        TM_8K    = ::TRANSMISSION_MODE_8K,
        TM_2KI   = -10,
        TM_4KI   = -11,
        TM_1K    = ::TRANSMISSION_MODE_1K,
        TM_16K   = ::TRANSMISSION_MODE_16K,
        TM_32K   = ::TRANSMISSION_MODE_32K,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        TM_C1    = -12,
        TM_C3780 = -13,
    #else
        TM_C1    = ::TRANSMISSION_MODE_C1,
        TM_C3780 = ::TRANSMISSION_MODE_C3780,
    #endif
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        TM_AUTO  = ::BDA_XMIT_MODE_NOT_DEFINED,
        TM_2K    = ::BDA_XMIT_MODE_2K,
        TM_4K    = ::BDA_XMIT_MODE_4K,
        TM_8K    = ::BDA_XMIT_MODE_8K,
        TM_2KI   = ::BDA_XMIT_MODE_2K_INTERLEAVED,
        TM_4KI   = ::BDA_XMIT_MODE_4K_INTERLEAVED,
        TM_1K    = ::BDA_XMIT_MODE_1K,
        TM_16K   = ::BDA_XMIT_MODE_16K,
        TM_32K   = ::BDA_XMIT_MODE_32K,
        TM_C1    = -10,
        TM_C3780 = -11,
#else
        TM_AUTO,  //!< Transmission mode automatically set.
        TM_2K,    //!< 2K transmission mode, aka ISDB-T "mode 1".
        TM_4K,    //!< 4K transmission mode, aka ISDB-T "mode 2".
        TM_8K,    //!< 8K transmission mode, aka ISDB-T "mode 3".
        TM_2KI,   //!< 2K-interleaved transmission mode.
        TM_4KI,   //!< 4K-interleaved transmission mode.
        TM_1K,    //!< 1K transmission mode, DVB-T2 (use 1K FFT).
        TM_16K,   //!< 16K transmission mode, DVB-T2 (use 16K FFT).
        TM_32K,   //!< 32K transmission mode, DVB-T2 (use 32K FFT).
        TM_C1,    //!< Single Carrier (C=1) transmission mode (DTMB only).
        TM_C3780, //!< Multi Carrier (C=3780) transmission mode (DTMB only).
#endif
    };

    //!
    //! Enumeration description of ts::TransmissionMode.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, TransmissionModeEnum);

    //!
    //! Guard interval (OFDM)
    //!
    enum GuardInterval {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        GUARD_AUTO   = ::GUARD_INTERVAL_AUTO,
        GUARD_1_32   = ::GUARD_INTERVAL_1_32,
        GUARD_1_16   = ::GUARD_INTERVAL_1_16,
        GUARD_1_8    = ::GUARD_INTERVAL_1_8,
        GUARD_1_4    = ::GUARD_INTERVAL_1_4,
        GUARD_1_128  = ::GUARD_INTERVAL_1_128,
        GUARD_19_128 = ::GUARD_INTERVAL_19_128,
        GUARD_19_256 = ::GUARD_INTERVAL_19_256,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        GUARD_PN420  = -9,
        GUARD_PN595  = -10,
        GUARD_PN945  = -11,
    #else
        GUARD_PN420  = ::GUARD_INTERVAL_PN420,
        GUARD_PN595  = ::GUARD_INTERVAL_PN595,
        GUARD_PN945  = ::GUARD_INTERVAL_PN945,
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
        GUARD_1_64   = -12,
    #else
        GUARD_1_64   = ::GUARD_INTERVAL_1_64,
    #endif
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        GUARD_AUTO   = ::BDA_GUARD_NOT_DEFINED,
        GUARD_1_32   = ::BDA_GUARD_1_32,
        GUARD_1_16   = ::BDA_GUARD_1_16,
        GUARD_1_8    = ::BDA_GUARD_1_8,
        GUARD_1_4    = ::BDA_GUARD_1_4,
        GUARD_1_128  = ::BDA_GUARD_1_128,
        GUARD_19_128 = ::BDA_GUARD_19_128,
        GUARD_19_256 = ::BDA_GUARD_19_256,
        GUARD_PN420  = -10,
        GUARD_PN595  = -11,
        GUARD_PN945  = -12,
        GUARD_1_64   = -13,
#else
        GUARD_AUTO,    //!< Guard interval automatically set.
        GUARD_1_32,    //!< Guard interval 1/32.
        GUARD_1_16,    //!< Guard interval 1/16.
        GUARD_1_8,     //!< Guard interval 1/8.
        GUARD_1_4,     //!< Guard interval 1/4.
        GUARD_1_128,   //!< Guard interval 1/128 (DVB-T2).
        GUARD_19_128,  //!< Guard interval 19/128 (DVB-T2).
        GUARD_19_256,  //!< Guard interval 19/256 (DVB-T2).
        GUARD_PN420,   //!< PN length 420 (1/4).
        GUARD_PN595,   //!< PN length 595 (1/6).
        GUARD_PN945,   //!< PN length 945 (1/9).
        GUARD_1_64,    //!< Guard interval 1/64.
#endif
    };

    //!
    //! Enumeration description of ts::GuardInterval.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, GuardIntervalEnum);

    //!
    //! Compute the multiplier of a guard interval value.
    //! @param [in] g Guard interval value.
    //! @return The multiplier (eg. 1 for GUARD_1_16) or zero if unknown.
    //!
    TSDUCKDLL uint32_t GuardIntervalMultiplier(GuardInterval g);

    //!
    //! Compute the divider of a guard interval value.
    //! @param [in] g Guard interval value.
    //! @return The divider (eg. 16 for GUARD_1_16) or zero if unknown.
    //!
    TSDUCKDLL uint32_t GuardIntervalDivider(GuardInterval g);

    //!
    //! Hierarchy (OFDM)
    //!
    enum Hierarchy {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        HIERARCHY_AUTO = ::HIERARCHY_AUTO,
        HIERARCHY_NONE = ::HIERARCHY_NONE,
        HIERARCHY_1    = ::HIERARCHY_1,
        HIERARCHY_2    = ::HIERARCHY_2,
        HIERARCHY_4    = ::HIERARCHY_4,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        HIERARCHY_AUTO = ::BDA_HALPHA_NOT_DEFINED,
        HIERARCHY_NONE = ::BDA_HALPHA_NOT_SET,
        HIERARCHY_1    = ::BDA_HALPHA_1,
        HIERARCHY_2    = ::BDA_HALPHA_2,
        HIERARCHY_4    = ::BDA_HALPHA_4,
#else
        HIERARCHY_AUTO,  //!< Hierarchy automatically set.
        HIERARCHY_NONE,  //!< No hierarchy.
        HIERARCHY_1,     //!< Hierarchy 1.
        HIERARCHY_2,     //!< Hierarchy 2.
        HIERARCHY_4,     //!< Hierarchy 4.
#endif
    };

    //!
    //! Enumeration description of ts::Hierarchy.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, HierarchyEnum);

    //
    // Representation of multistream in DVB-T2, DVB-S2, ISDB-S.
    //
    constexpr uint32_t PLP_MAX      = 0xFF;        //!< Maximum Physical Layer Pipe (PLP) value for DVB-T2.
    constexpr uint32_t PLP_DISABLE  = 0xFFFFFFFF;  //!< Special PLP value meaning "disable PLP selection".
    constexpr uint32_t ISI_MAX      = 0xFF;        //!< Maximum Input Stream Id (ISI) value for DVB-S2.
    constexpr uint32_t ISI_DISABLE  = 0xFFFFFFFF;  //!< Special ISI value meaning "disable multistream selection".
    constexpr uint32_t PLS_CODE_MAX = 0x3FFFF;     //!< Maximum Physical Layer Scrambling (PLS) code value for DVB-S2.
    constexpr uint32_t STREAM_ID_MAX      = 0xFFFF;      //!< Maximum Stream Id value (same as inner TS id) for ISDB-S.
    constexpr uint32_t STREAM_ID_DISABLE  = 0xFFFFFFFF;  //!< Special Stream Id value meaning "disable multistream selection".

    //!
    //! Physical Layer Scrambling (PLS) modes for DVB-S2.
    //!
    enum PLSMode {
        PLS_ROOT = 0,  //!< DVB-S2 PLS "ROOT" mode.
        PLS_GOLD = 1,  //!< DVB-S2 PLS "GOLD" mode.
    };

    //!
    //! Enumeration description of ts::PLSMode.
    //!
    TS_DECLARE_GLOBAL(const, Enumeration, PLSModeEnum);

    //!
    //! Convert a PLS code from GOLD to ROOT mode.
    //! @param [in] mode GOLD PLS mode value.
    //! @return Corresponding ROOT PLS mode value.
    //! @see ETSI EN 302 307-1, section 5.5.4.
    //!
    TSDUCKDLL uint32_t PLSCodeGoldToRoot(uint32_t mode);

    //!
    //! Convert a PLS code from ROOT to GOLD mode.
    //! @param [in] mode ROOT PLS mode value.
    //! @return Corresponding GOLD PLS mode value.
    //! @see ETSI EN 302 307-1, section 5.5.4.
    //!
    TSDUCKDLL uint32_t PLSCodeRootToGold(uint32_t mode);
}
