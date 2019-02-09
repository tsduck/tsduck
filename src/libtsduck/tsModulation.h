//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  @ingroup hardware
//!  Definition for MPEG transport modulations
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsEnumeration.h"
#include "tsReport.h"

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
    //! Known tuner types.
    //!
    //! "Second generation" tuners are included in their base category:
    //! DVB_S includes DVB-S and DVB-S2, DVB_T includes DVB-T and DVB-T2, etc.
    //!
    enum TunerType {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        DVB_S  = ::FE_QPSK,
        DVB_C  = ::FE_QAM,
        DVB_T  = ::FE_OFDM,
        ATSC   = ::FE_ATSC,
#else
        DVB_S,   //!< DVB-S, DVB-S2
        DVB_C,   //!< DVB-C, DVB-C2
        DVB_T,   //!< DVB-T, DVB-T2
        ATSC,    //!< ATSC
#endif
    };

    //!
    //! Enumeration description of ts::TunerType.
    //!
    TSDUCKDLL extern const Enumeration TunerTypeEnum;

    //!
    //! Delivery systems.
    //!
    enum DeliverySystem {
        DS_UNDEFINED,      //!< Undefined.
        DS_DVB_S,          //!< DVB-S.
        DS_DVB_S2,         //!< DVB-S2.
        DS_DVB_T,          //!< DVB-T.
        DS_DVB_T2,         //!< DVB-T2.
        DS_DVB_C,          //!< DVB-C.
        DS_DVB_C_ANNEX_AC, //!< DVB-C Annex A,C.
        DS_DVB_C_ANNEX_B,  //!< DVB-C Annex B.
        DS_DVB_C2,         //!< DVB-C2.
        DS_DVB_H,          //!< DVB-H.
        DS_ISDB_S,         //!< ISDB-S.
        DS_ISDB_T,         //!< ISDB-T.
        DS_ISDB_C,         //!< ISDB-C.
        DS_ATSC,           //!< ATSC.
        DS_ATSC_MH,        //!< ATSC-MH.
        DS_DMB_TH,         //!< DMB-TH.
        DS_CMMB,           //!< CMMB.
        DS_DAB,            //!< DAB.
        DS_DSS,            //!< DSS.
        DS_COUNT           //!< Fake value, must be last to get the number of values.
    };

    //!
    //! Enumeration description of ts::DeliverySystem.
    //!
    TSDUCKDLL extern const Enumeration DeliverySystemEnum;

    //!
    //! A set of delivery system values (ts::DeliverySystem).
    //! Typically used to indicate the list of standards which are supported by a tuner.
    //!
    typedef std::bitset<size_t(DS_COUNT)> DeliverySystemSet;

    //!
    //! Modulation types.
    //! Support depends on tuner types.
    //!
    enum Modulation {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        QPSK     = ::QPSK,
        PSK_8    = ::PSK_8,
        QAM_AUTO = ::QAM_AUTO,
        QAM_16   = ::QAM_16,
        QAM_32   = ::QAM_32,
        QAM_64   = ::QAM_64,
        QAM_128  = ::QAM_128,
        QAM_256  = ::QAM_256,
        VSB_8    = ::VSB_8,
        VSB_16   = ::VSB_16,
        APSK_16  = ::APSK_16,
        APSK_32  = ::APSK_32,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        QPSK     = ::BDA_MOD_QPSK,
        PSK_8    = ::BDA_MOD_8PSK,
        QAM_AUTO = ::BDA_MOD_NOT_DEFINED,
        QAM_16   = ::BDA_MOD_16QAM,
        QAM_32   = ::BDA_MOD_32QAM,
        QAM_64   = ::BDA_MOD_64QAM,
        QAM_128  = ::BDA_MOD_128QAM,
        QAM_256  = ::BDA_MOD_256QAM,
        VSB_8    = ::BDA_MOD_8VSB,
        VSB_16   = ::BDA_MOD_16VSB,
        APSK_16  = ::BDA_MOD_16APSK,
        APSK_32  = ::BDA_MOD_32APSK,
#else
        QPSK,       //!< QPSK (4-PSK, DVB-S).
        PSK_8,      //!< 8-PSK (DVB-S2).
        QAM_AUTO,   //!< Unspecified QAM.
        QAM_16,     //!< QAM-16.
        QAM_32,     //!< QAM-32.
        QAM_64,     //!< QAM-64.
        QAM_128,    //!< QAM-128.
        QAM_256,    //!< QAM-256.
        VSB_8,      //!< VSB-8.
        VSB_16,     //!< VSB-16.
        APSK_16,    //!< 16-APSK (DVB-S2).
        APSK_32,    //!< 32-APSK (DVB-S2).
#endif
    };

    //!
    //! Enumeration description of ts::Modulation.
    //!
    TSDUCKDLL extern const Enumeration ModulationEnum;

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
    TSDUCKDLL extern const Enumeration SpectralInversionEnum;

    //!
    //! Inner Forward Error Correction
    //!
    enum InnerFEC {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        FEC_NONE = ::FEC_NONE,
        FEC_AUTO = ::FEC_AUTO,
        FEC_1_2  = ::FEC_1_2,
        FEC_2_3  = ::FEC_2_3,
        FEC_3_4  = ::FEC_3_4,
        FEC_4_5  = ::FEC_4_5,
        FEC_5_6  = ::FEC_5_6,
        FEC_6_7  = ::FEC_6_7,
        FEC_7_8  = ::FEC_7_8,
        FEC_8_9  = ::FEC_8_9,
        FEC_9_10 = ::FEC_9_10,
        FEC_3_5  = ::FEC_3_5,
        FEC_1_3  = -12,
        FEC_1_4  = -13,
        FEC_2_5  = -14,
        FEC_5_11 = -15,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        FEC_NONE = ::BDA_BCC_RATE_NOT_SET,
        FEC_AUTO = ::BDA_BCC_RATE_NOT_DEFINED,
        FEC_1_2  = ::BDA_BCC_RATE_1_2,
        FEC_2_3  = ::BDA_BCC_RATE_2_3,
        FEC_3_4  = ::BDA_BCC_RATE_3_4,
        FEC_4_5  = ::BDA_BCC_RATE_4_5,
        FEC_5_6  = ::BDA_BCC_RATE_5_6,
        FEC_6_7  = ::BDA_BCC_RATE_6_7,
        FEC_7_8  = ::BDA_BCC_RATE_7_8,
        FEC_8_9  = ::BDA_BCC_RATE_8_9,
        FEC_9_10 = ::BDA_BCC_RATE_9_10,
        FEC_3_5  = ::BDA_BCC_RATE_3_5,
        FEC_1_3  = ::BDA_BCC_RATE_1_3,
        FEC_1_4  = ::BDA_BCC_RATE_1_4,
        FEC_2_5  = ::BDA_BCC_RATE_2_5,
        FEC_5_11 = ::BDA_BCC_RATE_5_11,
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
#endif
    };

    //!
    //! Enumeration description of ts::InnerFEC.
    //!
    TSDUCKDLL extern const Enumeration InnerFECEnum;

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
    TSDUCKDLL extern const Enumeration PolarizationEnum;

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
    TSDUCKDLL extern const Enumeration PilotEnum;

    //!
    //! Roll-off (DVB-S2)
    //!
    enum RollOff {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        ROLLOFF_AUTO = ::ROLLOFF_AUTO,
        ROLLOFF_35   = ::ROLLOFF_35,
        ROLLOFF_25   = ::ROLLOFF_25,
        ROLLOFF_20   = ::ROLLOFF_20,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        ROLLOFF_AUTO = ::BDA_ROLL_OFF_NOT_DEFINED,
        ROLLOFF_35   = ::BDA_ROLL_OFF_35,
        ROLLOFF_25   = ::BDA_ROLL_OFF_25,
        ROLLOFF_20   = ::BDA_ROLL_OFF_20,
#else
        ROLLOFF_AUTO,  //!< Automatic rolloff.
        ROLLOFF_35,    //!< Rolloff 35, implied in DVB-S, default in DVB-S2.
        ROLLOFF_25,    //!< Rolloff 25.
        ROLLOFF_20,    //!< Rolloff 20.
#endif
    };

    //!
    //! Enumeration description of ts::RollOff.
    //!
    TSDUCKDLL extern const Enumeration RollOffEnum;

    //!
    //! Bandwidth (OFDM, DVB-T/T2)
    //!
    enum BandWidth {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        BW_AUTO      =        0,  // values in Hz, not enum
        BW_1_712_MHZ =  1712000,
        BW_5_MHZ     =  5000000,
        BW_6_MHZ     =  6000000,
        BW_7_MHZ     =  7000000,
        BW_8_MHZ     =  8000000,
        BW_10_MHZ    = 10000000,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        BW_AUTO      = -10,
        BW_1_712_MHZ = -11,
        BW_5_MHZ     =   5,  // values in MHz, not enum
        BW_6_MHZ     =   6,
        BW_7_MHZ     =   7,
        BW_8_MHZ     =   8,
        BW_10_MHZ    =  10,
#else
        BW_AUTO,      //!< Bandwidth automatically set.
        BW_1_712_MHZ, //!< 1.712 MHz bandwidth (DVB-T2 only).
        BW_5_MHZ,     //!< 5 MHz bandwidth (DVB-T2 only).
        BW_6_MHZ,     //!< 6 MHz bandwidth.
        BW_7_MHZ,     //!< 7 MHz bandwidth.
        BW_8_MHZ,     //!< 8 MHz bandwidth.
        BW_10_MHZ,    //!< 10 MHz bandwidth (DVB-T2 only).
#endif
    };

    //!
    //! Enumeration description of ts::BandWidth.
    //!
    TSDUCKDLL extern const Enumeration BandWidthEnum;

    //!
    //! Get the bandwidth value in Hz.
    //! @param [in] bw Bandwidth enumeration value.
    //! @return Bandwidth in Hz or zero if unknown.
    //!
    TSDUCKDLL uint32_t BandWidthValueHz(BandWidth bw);

    //!
    //! Get the bandwidth code from a value in Hz.
    //! @param [in] hz Bandwidth in Hz.
    //! @return Bandwidth enumeration value or BW_AUTO if undefined.
    //!
    TSDUCKDLL BandWidth BandWidthCodeFromHz(uint32_t hz);

    //!
    //! Transmission mode (OFDM)
    //!
    enum TransmissionMode {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        TM_AUTO = ::TRANSMISSION_MODE_AUTO,
        TM_2K   = ::TRANSMISSION_MODE_2K,
        TM_4K   = ::TRANSMISSION_MODE_4K,
        TM_8K   = ::TRANSMISSION_MODE_8K,
        TM_2KI  = -10,
        TM_4KI  = -11,
        TM_1K   = ::TRANSMISSION_MODE_1K,
        TM_16K  = ::TRANSMISSION_MODE_16K,
        TM_32K  = ::TRANSMISSION_MODE_32K,
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        TM_AUTO = ::BDA_XMIT_MODE_NOT_DEFINED,
        TM_2K   = ::BDA_XMIT_MODE_2K,
        TM_4K   = ::BDA_XMIT_MODE_4K,
        TM_8K   = ::BDA_XMIT_MODE_8K,
        TM_2KI  = ::BDA_XMIT_MODE_2K_INTERLEAVED,
        TM_4KI  = ::BDA_XMIT_MODE_4K_INTERLEAVED,
        TM_1K   = ::BDA_XMIT_MODE_1K,
        TM_16K  = ::BDA_XMIT_MODE_16K,
        TM_32K  = ::BDA_XMIT_MODE_32K,
#else
        TM_AUTO,  //!< Transmission mode automatically set.
        TM_2K,    //!< 2K transmission mode.
        TM_4K,    //!< 4K transmission mode.
        TM_8K,    //!< 8K transmission mode.
        TM_2KI,   //!< 2K-interleaved transmission mode.
        TM_4KI,   //!< 4K-interleaved transmission mode.
        TM_1K,    //!< 1K transmission mode, DVB-T2 (use 1K FFT).
        TM_16K,   //!< 16K transmission mode, DVB-T2 (use 16K FFT).
        TM_32K,   //!< 32K transmission mode, DVB-T2 (use 32K FFT).
#endif
    };

    //!
    //! Enumeration description of ts::TransmissionMode.
    //!
    TSDUCKDLL extern const Enumeration TransmissionModeEnum;

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
#elif defined(TS_WINDOWS) && !defined(DOXYGEN)
        GUARD_AUTO   = ::BDA_GUARD_NOT_DEFINED,
        GUARD_1_32   = ::BDA_GUARD_1_32,
        GUARD_1_16   = ::BDA_GUARD_1_16,
        GUARD_1_8    = ::BDA_GUARD_1_8,
        GUARD_1_4    = ::BDA_GUARD_1_4,
        GUARD_1_128  = ::BDA_GUARD_1_128,
        GUARD_19_128 = ::BDA_GUARD_19_128,
        GUARD_19_256 = ::BDA_GUARD_19_256,
#else
        GUARD_AUTO,    //!< Guard interval automatically set.
        GUARD_1_32,    //!< Guard interval 1/32.
        GUARD_1_16,    //!< Guard interval 1/16.
        GUARD_1_8,     //!< Guard interval 1/8.
        GUARD_1_4,     //!< Guard interval 1/4.
        GUARD_1_128,   //!< Guard interval 1/128 (DVB-T2).
        GUARD_19_128,  //!< Guard interval 19/128 (DVB-T2).
        GUARD_19_256,  //!< Guard interval 19/256 (DVB-T2).
#endif
    };

    //!
    //! Enumeration description of ts::GuardInterval.
    //!
    TSDUCKDLL extern const Enumeration GuardIntervalEnum;

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
    TSDUCKDLL extern const Enumeration HierarchyEnum;

    //!
    //! Representation of a Physical Layer Pipe (PLP) id (DVB-T2).
    //!
    //! For DVB-T2, the valid range of PLP ids is 0 to 255.
    //! The special value PLP_DISABLE is used to disable PLP selection.
    //!
    enum PLP : uint32_t {
        PLP_DISABLE = 0xFFFFFFFF //!< Special value meaning "disable PLP selection".
    };

    //!
    //! UHF (Ultra High Frequecy) band.
    //!
    //! UHF channels in MHz: frequency = 306 + 8 * channel.
    //!
    //! 167 kHz offsets may be applied (-1, 1, 2 or 3).
    //!
    namespace UHF {

        const uint64_t CHANNEL_BASE = 306000000;   //!< UHF band base (306 MHz).
        const uint64_t CHANNEL_WIDTH =  8000000;   //!< UHF channel width (8 MHz).
        const uint64_t CHANNEL_OFFSET =  166666;   //!< Optional channel offset (~167 kHz).

        const int FIRST_CHANNEL = 21;  //!< First channel in UHF band.
        const int LAST_CHANNEL  = 69;  //!< Last channel in UHF band.

        //!
        //! Compute a UHF frequency from a channel number and optional offset count.
        //! @param [in] channel UHF channel number.
        //! @param [in] offset_count Optional offset count (usually from -1 to +3).
        //! @return Frequency in Hz.
        //!
        TSDUCKDLL inline uint64_t Frequency(int channel, int offset_count = 0)
        {
            return CHANNEL_BASE + uint64_t(channel) * CHANNEL_WIDTH + uint64_t(offset_count) * CHANNEL_OFFSET;
        }

        //!
        //! Compute a UHF channel number from a frequency.
        //! @param [in] frequency Frequency in Hz.
        //! @return UHF channel number.
        //!
        TSDUCKDLL inline int Channel(uint64_t frequency)
        {
            return int((frequency - CHANNEL_BASE + CHANNEL_WIDTH / 2) / CHANNEL_WIDTH);
        }

        //!
        //! Compute a UHF offset count from frequency (approximate if necessary)
        //! @param [in] frequency Frequency in Hz.
        //! @return Offset count (positive or negative).
        //!
        TSDUCKDLL int OffsetCount(uint64_t frequency);

        //!
        //! Check if a frequency is in the UHF band.
        //! @param [in] frequency Frequency in Hz.
        //! @param [in] min_offset Minimum allowed offset. Default: -3.
        //! @param [in] max_offset Maximum allowed offset. Default: +3.
        //! @return True if the frequency is in the UHF band.
        //!
        TSDUCKDLL inline bool InBand(uint64_t frequency, int min_offset = -3, int max_offset = 3)
        {
            return frequency >= Frequency(FIRST_CHANNEL, min_offset) && frequency <= Frequency(LAST_CHANNEL, max_offset);
        }

        //!
        //! Return a human-readable description of a UHF channel.
        //! @param [in] channel UHF channel number.
        //! @param [in] offset UHF channel offset count. Displayed only if non-zero.
        //! @param [in] strength Signal strength in percent. Ignored if negative.
        //! @param [in] quality Signal quality in percent. Ignored if negative.
        //! @return Channel description.
        //!
        TSDUCKDLL UString Description(int channel, int offset, int strength = -1, int quality = -1);
    }

    //!
    //! VHF (Very High Frequency) band III.
    //!
    //! VHF band III channels in MHz: frequency = 142.5 + 7 * channel.
    //!
    //! 167 kHz offsets may be applied (-1, 1, 2 or 3).
    //!
    namespace VHF {

        const uint64_t CHANNEL_BASE = 142500000;  //!< VHF band base (142.5 MHz).
        const uint64_t CHANNEL_WIDTH =  7000000;  //!< VHF channel width (7 MHz).
        const uint64_t CHANNEL_OFFSET =  166666;  //!< Optional channel offset (~167 kHz).

        const int FIRST_CHANNEL =  5;    //!< First channel in VHF band III.
        const int LAST_CHANNEL  = 12;    //!< Last channel in VHF band III.

        //!
        //! Compute a VHF frequency from a channel number and optional offset count.
        //! @param [in] channel VHF channel number.
        //! @param [in] offset_count Optional offset count (usually from -1 to +3).
        //! @return Frequency in Hz.
        //!
        TSDUCKDLL inline uint64_t Frequency(int channel, int offset_count = 0)
        {
            return CHANNEL_BASE + uint64_t(channel) * CHANNEL_WIDTH + uint64_t(offset_count) * CHANNEL_OFFSET;
        }

        //!
        //! Compute a VHF channel number from a frequency.
        //! @param [in] frequency Frequency in Hz.
        //! @return VHF channel number.
        //!
        TSDUCKDLL inline int Channel(uint64_t frequency)
        {
            return int((frequency - CHANNEL_BASE + CHANNEL_WIDTH / 2) / CHANNEL_WIDTH);
        }

        //!
        //! Compute a VHF offset count from frequency (approximate if necessary)
        //! @param [in] frequency Frequency in Hz.
        //! @return Offset count (positive or negative).
        //!
        TSDUCKDLL int OffsetCount(uint64_t frequency);

        //!
        //! Check if a frequency is in the UHF band III.
        //! @param [in] frequency Frequency in Hz.
        //! @param [in] min_offset Minimum allowed offset. Default: -3.
        //! @param [in] max_offset Maximum allowed offset. Default: +3.
        //! @return True if the frequency is in the VHF band III.
        //!
        TSDUCKDLL inline bool InBand (uint64_t frequency, int min_offset = -3, int max_offset = 3)
        {
            return frequency >= Frequency (FIRST_CHANNEL, min_offset) && frequency <= Frequency (LAST_CHANNEL, max_offset);
        }
    }
}
