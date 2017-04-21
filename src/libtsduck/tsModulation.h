//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Definition for MPEG transport modulations
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsEnumeration.h"
#include "tsReportInterface.h"

namespace ts {

    // Whenever possible, all enumeration have identical integer values as
    // their counterparts in the Linux DVB or Windows DirectShow API for faster
    // conversion. When an enum cannot be mapped to a native value,
    // a "very negative" value is used. Very negative means "more negative
    // than -1" since -1 is sometimes used as enum value in DirectShow.
    
    // This function checks that an enumeration value is supported by
    // the native implementation. If it is not, report an error message
    // and return false.

    TSDUCKDLL bool CheckModEnum (int value, const std::string& name, const Enumeration& conv, ReportInterface&);

    // Known tuner types.
    //
    // "Second generation" tuners are included in their base category:
    // DVB_S includes DVB-S and DVB-S2, DVB_T includes DVB-T and DVB-T2, etc.

    enum TunerType {
#if defined (__linux)
        DVB_S  = ::FE_QPSK,
        DVB_C  = ::FE_QAM,
        DVB_T  = ::FE_OFDM,
        ATSC   = ::FE_ATSC,
#else
        DVB_S,
        DVB_C,
        DVB_T,
        ATSC,
#endif
    };

    TSDUCKDLL extern const Enumeration TunerTypeEnum;

    // Delivery systems.

    enum DeliverySystem {
        DS_UNDEFINED,
        DS_DVB_S,
        DS_DVB_S2,
        DS_DVB_T,
        DS_DVB_T2,
        DS_DVB_C,
        DS_DVB_C_ANNEX_AC,
        DS_DVB_C_ANNEX_B,
        DS_DVB_C2,
        DS_DVB_H,
        DS_ISDB_S,
        DS_ISDB_T,
        DS_ISDB_C,
        DS_ATSC,
        DS_ATSC_MH,
        DS_DMB_TH,
        DS_CMMB,
        DS_DAB,
        DS_DSS,

        DS_COUNT // Fake value, must be last to get the number of values
    };

    TSDUCKDLL extern const Enumeration DeliverySystemEnum;

    // A set of delivery system values. Typically used to indicate the
    // list of standards which are supported by a tuner.

    typedef std::bitset<size_t(DS_COUNT)> DeliverySystemSet;

    // Modulation types. Support depends on tuner types.

    enum Modulation {
#if defined (__linux)
        QPSK     = ::QPSK,
#if defined (__s2api)
        PSK_8    = ::PSK_8,
#else
        PSK_8    = -10,
#endif
        QAM_AUTO = ::QAM_AUTO,
        QAM_16   = ::QAM_16,
        QAM_32   = ::QAM_32,
        QAM_64   = ::QAM_64,
        QAM_128  = ::QAM_128,
        QAM_256  = ::QAM_256,
        VSB_8    = ::VSB_8,
        VSB_16   = ::VSB_16,
#elif defined (__windows)
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
#else
        QPSK,
        PSK_8,      // 8-PSK (DVB-S2)
        QAM_AUTO,   // unspecified
        QAM_16,
        QAM_32,
        QAM_64,
        QAM_128,
        QAM_256,
        VSB_8,
        VSB_16,
#endif
    };

    TSDUCKDLL extern const Enumeration ModulationEnum;

    // Compute the number of bits per symbol for a specified modulation.
    // Return zero if unknown

    TSDUCKDLL uint32_t BitsPerSymbol (Modulation);

    // Spectral inversion

    enum SpectralInversion {
#if defined (__linux)
        SPINV_OFF  = ::INVERSION_OFF,
        SPINV_ON   = ::INVERSION_ON,
        SPINV_AUTO = ::INVERSION_AUTO,
#elif defined (__windows)
        SPINV_OFF  = ::BDA_SPECTRAL_INVERSION_NORMAL,
        SPINV_ON   = ::BDA_SPECTRAL_INVERSION_INVERTED,
        SPINV_AUTO = ::BDA_SPECTRAL_INVERSION_AUTOMATIC,
#else
        SPINV_OFF,
        SPINV_ON,
        SPINV_AUTO,
#endif
};

    TSDUCKDLL extern const Enumeration SpectralInversionEnum;

    // Inner Forward Error Correction

    enum InnerFEC {
#if defined (__linux)
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
#if defined (__s2api)
        FEC_9_10 = ::FEC_9_10,
        FEC_3_5  = ::FEC_3_5,
#else
        FEC_9_10 = -10,
        FEC_3_5  = -11,
#endif
        FEC_1_3  = -12,
        FEC_1_4  = -13,
        FEC_2_5  = -14,
        FEC_5_11 = -15,
#elif defined (__windows)
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
        FEC_NONE,
        FEC_AUTO,   // unspecified
        FEC_1_2,
        FEC_2_3,
        FEC_3_4,
        FEC_4_5,
        FEC_5_6,
        FEC_6_7,
        FEC_7_8,
        FEC_8_9,
        FEC_9_10,
        FEC_3_5,
        FEC_1_3,
        FEC_1_4,
        FEC_2_5,
        FEC_5_11,
#endif
    };

    TSDUCKDLL extern const Enumeration InnerFECEnum;

    // Compute the multiplier and divider of a FEC value.
    // Return zero if unknown

    TSDUCKDLL uint32_t FECMultiplier (InnerFEC);
    TSDUCKDLL uint32_t FECDivider (InnerFEC);

    // Polarization

    enum Polarization {
#if defined (__windows)
        POL_NONE       = ::BDA_POLARISATION_NOT_SET,
        POL_AUTO       = ::BDA_POLARISATION_NOT_DEFINED,
        POL_HORIZONTAL = ::BDA_POLARISATION_LINEAR_H,
        POL_VERTICAL   = ::BDA_POLARISATION_LINEAR_V,
        POL_LEFT       = ::BDA_POLARISATION_CIRCULAR_L,
        POL_RIGHT      = ::BDA_POLARISATION_CIRCULAR_R,
#else
        POL_NONE,
        POL_AUTO,
        POL_HORIZONTAL,  // linear
        POL_VERTICAL,    // linear
        POL_LEFT,        // circular
        POL_RIGHT,       // circular
#endif
    };

    TSDUCKDLL extern const Enumeration PolarizationEnum;

    // Pilot (DVB-S2)

    enum Pilot {
#if defined (__linux) && defined (__s2api)
        PILOT_AUTO = ::PILOT_AUTO,
        PILOT_ON   = ::PILOT_ON,
        PILOT_OFF  = ::PILOT_OFF,
#elif defined (__linux) && !defined (__s2api)
        PILOT_AUTO = -10,
        PILOT_ON   = -11,
        PILOT_OFF  = -12,
#elif defined (__windows)
        PILOT_AUTO = ::BDA_PILOT_NOT_DEFINED,
        PILOT_ON   = ::BDA_PILOT_ON,
        PILOT_OFF  = ::BDA_PILOT_OFF,
#else
        PILOT_AUTO,
        PILOT_ON,
        PILOT_OFF,
#endif
    };

    TSDUCKDLL extern const Enumeration PilotEnum;

    // Roll-off (DVB-S2)

    enum RollOff {
#if defined (__linux) && defined (__s2api)
        ROLLOFF_AUTO = ::ROLLOFF_AUTO,
        ROLLOFF_35   = ::ROLLOFF_35,
        ROLLOFF_25   = ::ROLLOFF_25,
        ROLLOFF_20   = ::ROLLOFF_20,
#elif defined (__linux) && !defined (__s2api)
        ROLLOFF_AUTO = -10,
        ROLLOFF_35   = -11,
        ROLLOFF_25   = -12,
        ROLLOFF_20   = -13,
#elif defined (__windows)
        ROLLOFF_AUTO = ::BDA_ROLL_OFF_NOT_DEFINED,
        ROLLOFF_35   = ::BDA_ROLL_OFF_35,
        ROLLOFF_25   = ::BDA_ROLL_OFF_25,
        ROLLOFF_20   = ::BDA_ROLL_OFF_20,
#else
        ROLLOFF_AUTO,
        ROLLOFF_35,   // Implied in DVB-S, default in DVB-S2
        ROLLOFF_25,
        ROLLOFF_20,
#endif
    };

    TSDUCKDLL extern const Enumeration RollOffEnum;

    // Bandwidth (OFDM)

    enum BandWidth {
#if defined (__linux)
        BW_AUTO  = ::BANDWIDTH_AUTO,
        BW_8_MHZ = ::BANDWIDTH_8_MHZ,
        BW_7_MHZ = ::BANDWIDTH_7_MHZ,
        BW_6_MHZ = ::BANDWIDTH_6_MHZ,
        BW_5_MHZ = -10,
#elif defined (__windows)
        BW_AUTO  = -10,
        BW_8_MHZ = 8,  // values in MHz, not enum
        BW_7_MHZ = 7,
        BW_6_MHZ = 6,
        BW_5_MHZ = 5,
#else
        BW_AUTO,
        BW_8_MHZ,
        BW_7_MHZ,
        BW_6_MHZ,
        BW_5_MHZ,
#endif
    };
 
    TSDUCKDLL extern const Enumeration BandWidthEnum;

    // Get the bandwidth value in Hz.
    // Return zero if unknown.

    TSDUCKDLL uint32_t BandWidthValueHz (BandWidth);

    // Get the bandwidth code from a value in Hz.
    // Return BW_AUTO if undefined.

    TSDUCKDLL BandWidth BandWidthCodeFromHz (uint32_t);

    // Transmission mode (OFDM)

    enum TransmissionMode {
#if defined (__linux)
        TM_AUTO = ::TRANSMISSION_MODE_AUTO,
        TM_2K   = ::TRANSMISSION_MODE_2K,
        TM_4K   = -10,
        TM_8K   = ::TRANSMISSION_MODE_8K,
#elif (__windows)
        TM_AUTO = ::BDA_XMIT_MODE_NOT_DEFINED,
        TM_2K   = ::BDA_XMIT_MODE_2K,
        TM_4K   = ::BDA_XMIT_MODE_4K,
        TM_8K   = ::BDA_XMIT_MODE_8K,
#else
        TM_AUTO,
        TM_2K,
        TM_4K,
        TM_8K,
#endif
    };

    TSDUCKDLL extern const Enumeration TransmissionModeEnum;

    // Guard interval (OFDM)

    enum GuardInterval {
#if defined (__linux)
        GUARD_AUTO = ::GUARD_INTERVAL_AUTO,
        GUARD_1_32 = ::GUARD_INTERVAL_1_32,
        GUARD_1_16 = ::GUARD_INTERVAL_1_16,
        GUARD_1_8  = ::GUARD_INTERVAL_1_8,
        GUARD_1_4  = ::GUARD_INTERVAL_1_4,
#elif defined (__windows)
        GUARD_AUTO = ::BDA_GUARD_NOT_DEFINED,
        GUARD_1_32 = ::BDA_GUARD_1_32,
        GUARD_1_16 = ::BDA_GUARD_1_16,
        GUARD_1_8  = ::BDA_GUARD_1_8,
        GUARD_1_4  = ::BDA_GUARD_1_4,
#else
        GUARD_AUTO,
        GUARD_1_32,
        GUARD_1_16,
        GUARD_1_8,
        GUARD_1_4,
#endif
    };

    TSDUCKDLL extern const Enumeration GuardIntervalEnum;

    // Compute the multiplier and divider of a guard interval value.
    // Return zero if unknown

    TSDUCKDLL uint32_t GuardIntervalMultiplier (GuardInterval);
    TSDUCKDLL uint32_t GuardIntervalDivider (GuardInterval);

    // Hierarchy (OFDM)

    enum Hierarchy {
#if defined (__linux)
        HIERARCHY_AUTO = ::HIERARCHY_AUTO,
        HIERARCHY_NONE = ::HIERARCHY_NONE,
        HIERARCHY_1    = ::HIERARCHY_1,
        HIERARCHY_2    = ::HIERARCHY_2,
        HIERARCHY_4    = ::HIERARCHY_4,
#elif defined (__windows)
        HIERARCHY_AUTO = ::BDA_HALPHA_NOT_DEFINED,
        HIERARCHY_NONE = ::BDA_HALPHA_NOT_SET,
        HIERARCHY_1    = ::BDA_HALPHA_1,
        HIERARCHY_2    = ::BDA_HALPHA_2,
        HIERARCHY_4    = ::BDA_HALPHA_4,
#else
        HIERARCHY_AUTO,
        HIERARCHY_NONE,
        HIERARCHY_1,
        HIERARCHY_2,
        HIERARCHY_4,
#endif
    };

    TSDUCKDLL extern const Enumeration HierarchyEnum;

    //!
    //! UHF (Ultra High Frequecy) band
    //!
    namespace UHF {

        // UHF channels in MHz: freq = 306 + 8 * chan
        // 167 kHz offsets may be applied (-1, 1, 2 or 3).
        // All values below are in Hz:

        const uint64_t CHANNEL_BASE = 306000000;   // 306 MHz
        const uint64_t CHANNEL_WIDTH =  8000000;   // 8 MHz
        const uint64_t CHANNEL_OFFSET =  166666;   // ~167 kHz

        const int FIRST_CHANNEL = 21;  // First channel in UHF band
        const int LAST_CHANNEL  = 69;  // Last channel in UHF band

        // Compute a frequency from channel number and optional offset count
        TSDUCKDLL inline uint64_t Frequency (int channel, int offset_count = 0)
        {
            return CHANNEL_BASE + uint64_t (channel) * CHANNEL_WIDTH + uint64_t (offset_count) * CHANNEL_OFFSET;
        }

        // Compute channel number from frequency.
        TSDUCKDLL inline int Channel (uint64_t frequency)
        {
            return int ((frequency - CHANNEL_BASE + CHANNEL_WIDTH / 2) / CHANNEL_WIDTH);
        }

        // Compute offset count from frequency (approximate if necessary)
        TSDUCKDLL int OffsetCount (uint64_t frequency);

        // Check if a frequency is in the UHF band
        TSDUCKDLL inline bool InBand (uint64_t frequency, int min_offset = -3, int max_offset = 3)
        {
            return frequency >= Frequency (FIRST_CHANNEL, min_offset) && frequency <= Frequency (LAST_CHANNEL, max_offset);
        }
    }

    //!
    //! VHF (Very High Frequency) band III
    //!
    namespace VHF {
        
        // VHF band III channels in MHz: freq = 142.5 + 7 * chan
        // 167 kHz offsets may be applied (-1, 1, 2 or 3).
        // All values below are in Hz:

        const uint64_t CHANNEL_BASE = 142500000;   // 142.5 MHz
        const uint64_t CHANNEL_WIDTH =  7000000;   // 7 MHz
        const uint64_t CHANNEL_OFFSET =  166666;   // ~167 kHz

        const int FIRST_CHANNEL =  5;  // First channel in UHF band III
        const int LAST_CHANNEL  = 12;  // Last channel in UHF band III

        // Compute a frequency from channel number and optional offset count
        TSDUCKDLL inline uint64_t Frequency (int channel, int offset_count = 0)
        {
            return CHANNEL_BASE + uint64_t (channel) * CHANNEL_WIDTH + uint64_t (offset_count) * CHANNEL_OFFSET;
        }

        // Compute channel number from frequency.
        TSDUCKDLL inline int Channel (uint64_t frequency)
        {
            return int ((frequency - CHANNEL_BASE + CHANNEL_WIDTH / 2) / CHANNEL_WIDTH);
        }

        // Compute offset count from frequency (approximate if necessary)
        TSDUCKDLL int OffsetCount (uint64_t frequency);

        // Check if a frequency is in the VHF band
        TSDUCKDLL inline bool InBand (uint64_t frequency, int min_offset = -3, int max_offset = 3)
        {
            return frequency >= Frequency (FIRST_CHANNEL, min_offset) && frequency <= Frequency (LAST_CHANNEL, max_offset);
        }
    }
}
