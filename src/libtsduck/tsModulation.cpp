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
//
//  Definition for MPEG transport modulations
//
//----------------------------------------------------------------------------

#include "tsModulation.h"
#include "tsDecimal.h"
#include "tsFormat.h"


//----------------------------------------------------------------------------
// This function checks that an enumeration value is supported by
// the native implementation. If it is not, report an error message
// and return false.
//----------------------------------------------------------------------------

bool ts::CheckModEnum (int value, const std::string& name, const Enumeration& conv, ReportInterface& report)
{
    if (value > -10) {
        return true;
    }
    else {
        report.error (name + " " + conv.name (value) + " is not supported"
#if defined (__linux)
                      " by Linux DVB"
#elif defined (__windows)
                      " by Windows BDA/DirectShow"
#endif
                      );
        return false;
    }
}


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Enumeration ts::TunerTypeEnum
    ("DVB-S",  ts::DVB_S,
     "DVB-C",  ts::DVB_C,
     "DVB-T",  ts::DVB_T,
     "ATSC",   ts::ATSC,
     TS_NULL);

const ts::Enumeration ts::DeliverySystemEnum
    ("undefined", ts::DS_UNDEFINED,
     "DVB-S",     ts::DS_DVB_S,
     "DVB-S2",    ts::DS_DVB_S2,
     "DVB-T",     ts::DS_DVB_T,
     "DVB-T2",    ts::DS_DVB_T2,
     "DVB-C",     ts::DS_DVB_C,
     "DVB-C/AC",  ts::DS_DVB_C_ANNEX_AC,
     "DVB-C/B",   ts::DS_DVB_C_ANNEX_B,
     "DVB-C2",    ts::DS_DVB_C2,
     "DVB-H",     ts::DS_DVB_H,
     "ISDB-S",    ts::DS_ISDB_S,
     "ISDB-T",    ts::DS_ISDB_T,
     "ISDB-C",    ts::DS_ISDB_C,
     "ATSC",      ts::DS_ATSC,
     "ATSC-MH",   ts::DS_ATSC_MH,
     "DMB-TH",    ts::DS_DMB_TH,
     "CMMB",      ts::DS_CMMB,
     "DAB",       ts::DS_DAB,
     "DSS",       ts::DS_DSS,
     TS_NULL);

const ts::Enumeration ts::ModulationEnum
    ("QPSK",    ts::QPSK,
     "8-PSK",   ts::PSK_8,
     "QAM",     ts::QAM_AUTO,
     "16-QAM",  ts::QAM_16,
     "32-QAM",  ts::QAM_32,
     "64-QAM",  ts::QAM_64,
     "128-QAM", ts::QAM_128,
     "256-QAM", ts::QAM_256,
     "8-VSB",   ts::VSB_8,
     "16-VSB",  ts::VSB_16,
     TS_NULL);

const ts::Enumeration ts::InnerFECEnum
    ("none", ts::FEC_NONE,
     "auto", ts::FEC_AUTO,
     "1/2",  ts::FEC_1_2,
     "2/3",  ts::FEC_2_3,
     "3/4",  ts::FEC_3_4,
     "4/5",  ts::FEC_4_5,
     "5/6",  ts::FEC_5_6,
     "6/7",  ts::FEC_6_7,
     "7/8",  ts::FEC_7_8,
     "8/9",  ts::FEC_8_9,
     "9/10", ts::FEC_9_10,
     "3/5",  ts::FEC_3_5,
     "1/3",  ts::FEC_1_3,
     "1/4",  ts::FEC_1_4,
     "2/5",  ts::FEC_2_5,
     "5/11", ts::FEC_5_11,
     TS_NULL);

const ts::Enumeration ts::PolarizationEnum
    ("none",       ts::POL_NONE,
     "auto",       ts::POL_AUTO,
     "horizontal", ts::POL_HORIZONTAL,
     "vertical",   ts::POL_VERTICAL,
     "left",       ts::POL_LEFT,
     "right",      ts::POL_RIGHT,
     TS_NULL);

const ts::Enumeration ts::PilotEnum
    ("auto",       ts::PILOT_AUTO,
     "on",         ts::PILOT_ON,
     "off",        ts::PILOT_OFF,
     TS_NULL);

const ts::Enumeration ts::RollOffEnum
    ("auto",       ts::ROLLOFF_AUTO,
     "0.35",       ts::ROLLOFF_35,
     "0.25",       ts::ROLLOFF_25,
     "0.20",       ts::ROLLOFF_20,
     TS_NULL);

const ts::Enumeration ts::BandWidthEnum
    ("auto",  ts::BW_AUTO,
     "8-MHz", ts::BW_8_MHZ,
     "7-MHz", ts::BW_7_MHZ,
     "6-MHz", ts::BW_6_MHZ,
     "5-MHz", ts::BW_5_MHZ,
     TS_NULL);

const ts::Enumeration ts::TransmissionModeEnum
    ("auto", ts::TM_AUTO,
     "2K",   ts::TM_2K,
     "4K",   ts::TM_4K,
     "8K",   ts::TM_8K,
     TS_NULL);

const ts::Enumeration ts::GuardIntervalEnum
    ("auto", ts::GUARD_AUTO,
     "1/32", ts::GUARD_1_32,
     "1/16", ts::GUARD_1_16,
     "1/8",  ts::GUARD_1_8,
     "1/4",  ts::GUARD_1_4,
     TS_NULL);

const ts::Enumeration ts::HierarchyEnum
    ("auto", ts::HIERARCHY_AUTO,
     "none", ts::HIERARCHY_NONE,
     "1",    ts::HIERARCHY_1,
     "2",    ts::HIERARCHY_2,
     "4",    ts::HIERARCHY_4,
     TS_NULL);

const ts::Enumeration ts::SpectralInversionEnum
    ("off",  ts::SPINV_OFF,
     "on",   ts::SPINV_ON,
     "auto", ts::SPINV_AUTO,
     TS_NULL);


//----------------------------------------------------------------------------
// Compute the number of bits per symbol for a specified modulation.
// Return zero if unknown
//----------------------------------------------------------------------------

uint32_t ts::BitsPerSymbol (Modulation modulation)
{
    switch (modulation) {
        case QPSK:    return 2;  // Q (in QPSK) = quad = 4 states = 2 bits
        case PSK_8:   return 3;  // 8 states = 3 bits
        case QAM_16:  return 4;  // 16 states = 4 bits
        case QAM_32:  return 5;  // 32 states = 5 bits
        case QAM_64:  return 6;  // 64 states = 6 bits
        case QAM_128: return 7;  // 128 states = 7 bits
        case QAM_256: return 8;  // 256 states = 8 bits
        default:      return 0;  // Unknown
    }
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a FEC value.
// Return zero if unknown
//----------------------------------------------------------------------------

uint32_t ts::FECMultiplier (InnerFEC fec)
{
    switch (fec) {
        case FEC_NONE: return 1; // none means 1/1
        case FEC_1_2:  return 1;
        case FEC_2_3:  return 2;
        case FEC_3_4:  return 3;
        case FEC_4_5:  return 4;
        case FEC_5_6:  return 5;
        case FEC_6_7:  return 6;
        case FEC_7_8:  return 7;
        case FEC_8_9:  return 8;
        case FEC_9_10: return 9;
        case FEC_3_5:  return 3;
        case FEC_1_3:  return 1;
        case FEC_1_4:  return 1;
        case FEC_2_5:  return 2;
        case FEC_5_11: return 5;
        default:       return 0; // Unknown
    }
}

uint32_t ts::FECDivider (InnerFEC fec)
{
    switch (fec) {
        case FEC_NONE: return 1; // none means 1/1
        case FEC_1_2:  return 2;
        case FEC_2_3:  return 3;
        case FEC_3_4:  return 4;
        case FEC_4_5:  return 5;
        case FEC_5_6:  return 6;
        case FEC_6_7:  return 7;
        case FEC_7_8:  return 8;
        case FEC_8_9:  return 9;
        case FEC_9_10: return 10;
        case FEC_3_5:  return 5;
        case FEC_1_3:  return 3;
        case FEC_1_4:  return 4;
        case FEC_2_5:  return 5;
        case FEC_5_11: return 11;
        default:       return 0; // Unknown
    }
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a guard interval value.
// Return zero if unknown
//----------------------------------------------------------------------------

uint32_t ts::GuardIntervalMultiplier (GuardInterval guard)
{
    switch (guard) {
        case GUARD_1_4:  return 1;
        case GUARD_1_8:  return 1;
        case GUARD_1_16: return 1;
        case GUARD_1_32: return 1;
        default:         return 0; // unknown
    }
}


uint32_t ts::GuardIntervalDivider (GuardInterval guard)
{
    switch (guard) {
        case GUARD_1_4:  return 4;
        case GUARD_1_8:  return 8;
        case GUARD_1_16: return 16;
        case GUARD_1_32: return 32;
        default:         return 0; // unknown
    }
}


//----------------------------------------------------------------------------
// Get the bandwidth value in Hz.
// Return zero if unknown.
//----------------------------------------------------------------------------

uint32_t ts::BandWidthValueHz (BandWidth bandwidth)
{
    switch (bandwidth) {
        case BW_8_MHZ: return 8000000;
        case BW_7_MHZ: return 7000000;
        case BW_6_MHZ: return 6000000;
        case BW_5_MHZ: return 5000000;
        default:       return 0; // unknown
    }
}


//----------------------------------------------------------------------------
// Get the bandwidth code from a value in Hz.
// Return BW_AUTO if undefined.
//----------------------------------------------------------------------------

ts::BandWidth ts::BandWidthCodeFromHz (uint32_t hz)
{
    switch (hz) {
        case 8000000: return BW_8_MHZ;
        case 7000000: return BW_7_MHZ;
        case 6000000: return BW_6_MHZ;
        case 5000000: return BW_5_MHZ;
        default:      return BW_AUTO;
    }
}


//----------------------------------------------------------------------------
// Compute UHF frequencies, channels and offsets
//----------------------------------------------------------------------------

int ts::UHF::OffsetCount (uint64_t frequency)
{
    int off = int (int64_t (frequency) - int64_t (CHANNEL_BASE) - int64_t (Channel (frequency) * CHANNEL_WIDTH));
    int count = (std::abs (off) + int (CHANNEL_OFFSET / 2)) / int (CHANNEL_OFFSET);
    return off < 0 ? -count : count;
}


//----------------------------------------------------------------------------
// Compute VHF frequencies, channels and offsets
//----------------------------------------------------------------------------

int ts::VHF::OffsetCount (uint64_t frequency)
{
    int off = int (int64_t (frequency) - int64_t (CHANNEL_BASE) - int64_t (Channel (frequency) * CHANNEL_WIDTH));
    int count = (std::abs (off) + int (CHANNEL_OFFSET / 2)) / int (CHANNEL_OFFSET);
    return off < 0 ? -count : count;
}


//----------------------------------------------------------------------------
// Return a human-readable description of a UHF channel.
//----------------------------------------------------------------------------

std::string ts::UHF::Description(int channel, int offset, int strength, int quality)
{
    const uint64_t freq = UHF::Frequency(channel, offset);
    const int mhz = int(freq / 1000000);
    const int khz = int((freq % 1000000) / 1000);
    std::string desc("channel ");
    desc += Decimal(channel);
    if (offset != 0) {
        desc += ", offset ";
        desc += Decimal(offset, 0, true, ",", true);
    }
    desc += " (";
    desc += Decimal(mhz);
    if (khz > 0) {
        desc += Format(".%03d", khz);
    }
    desc += " MHz)";
    if (strength >= 0) {
        desc += Format(", strength: %d%%", strength);
    }
    if (quality >= 0) {
        desc += Format(", quality: %d%%", quality);
    }
    return desc;
}
