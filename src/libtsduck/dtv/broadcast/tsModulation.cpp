//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsModulation.h"


//----------------------------------------------------------------------------
// This function checks that an enumeration value is supported by
// the native implementation. If it is not, report an error message
// and return false.
//----------------------------------------------------------------------------

bool ts::CheckModEnum(int value, const UString& name, const Enumeration& conv, Report& report)
{
    if (value > -10) {
        return true;
    }
    else {
        report.error(u"%s %s is not supported"
#if defined(TS_LINUX)
                     u" by Linux DVB"
#elif defined(TS_WINDOWS)
                     u" by Windows BDA/DirectShow"
#endif
                     , {name, conv.name(value)});
        return false;
    }
}


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Enumeration ts::ModulationEnum({
    {u"QPSK",     ts::QPSK},
    {u"8-PSK",    ts::PSK_8},
    {u"QAM",      ts::QAM_AUTO},
    {u"16-QAM",   ts::QAM_16},
    {u"32-QAM",   ts::QAM_32},
    {u"64-QAM",   ts::QAM_64},
    {u"128-QAM",  ts::QAM_128},
    {u"256-QAM",  ts::QAM_256},
    {u"8-VSB",    ts::VSB_8},
    {u"16-VSB",   ts::VSB_16},
    {u"16-APSK",  ts::APSK_16},
    {u"32-APSK",  ts::APSK_32},
    {u"DQPSK",    ts::DQPSK},
    {u"4-QAM-NR", ts::QAM_4_NR},
});

const ts::Enumeration ts::InnerFECEnum({
    {u"none", ts::FEC_NONE},
    {u"auto", ts::FEC_AUTO},
    {u"1/2",  ts::FEC_1_2},
    {u"2/3",  ts::FEC_2_3},
    {u"3/4",  ts::FEC_3_4},
    {u"4/5",  ts::FEC_4_5},
    {u"5/6",  ts::FEC_5_6},
    {u"6/7",  ts::FEC_6_7},
    {u"7/8",  ts::FEC_7_8},
    {u"8/9",  ts::FEC_8_9},
    {u"9/10", ts::FEC_9_10},
    {u"3/5",  ts::FEC_3_5},
    {u"1/3",  ts::FEC_1_3},
    {u"1/4",  ts::FEC_1_4},
    {u"2/5",  ts::FEC_2_5},
    {u"5/11", ts::FEC_5_11},
});

const ts::Enumeration ts::PolarizationEnum({
    {u"none",       ts::POL_NONE},
    {u"auto",       ts::POL_AUTO},
    {u"horizontal", ts::POL_HORIZONTAL},
    {u"vertical",   ts::POL_VERTICAL},
    {u"left",       ts::POL_LEFT},
    {u"right",      ts::POL_RIGHT},
});

const ts::Enumeration ts::PilotEnum({
    {u"auto",       ts::PILOT_AUTO},
    {u"on",         ts::PILOT_ON},
    {u"off",        ts::PILOT_OFF},
});

const ts::Enumeration ts::RollOffEnum({
    {u"auto",       ts::ROLLOFF_AUTO},
    {u"0.35",       ts::ROLLOFF_35},
    {u"0.25",       ts::ROLLOFF_25},
    {u"0.20",       ts::ROLLOFF_20},
});

const ts::Enumeration ts::TransmissionModeEnum({
    {u"auto",           ts::TM_AUTO},
    {u"2K",             ts::TM_2K},
    {u"4K",             ts::TM_4K},
    {u"8K",             ts::TM_8K},
    {u"2K-interleaved", ts::TM_2KI},
    {u"4K-interleaved", ts::TM_4KI},
    {u"1K",             ts::TM_1K},
    {u"16K",            ts::TM_16K},
    {u"32K",            ts::TM_32K},
    {u"C=1",            ts::TM_C1},
    {u"C=3780",         ts::TM_C3780},
});

const ts::Enumeration ts::GuardIntervalEnum({
    {u"auto",    ts::GUARD_AUTO},
    {u"1/32",    ts::GUARD_1_32},
    {u"1/16",    ts::GUARD_1_16},
    {u"1/8",     ts::GUARD_1_8},
    {u"1/4",     ts::GUARD_1_4},
    {u"1/128",   ts::GUARD_1_128},
    {u"19/128",  ts::GUARD_19_128},
    {u"19/256",  ts::GUARD_19_256},
    {u"PN-420",  ts::GUARD_PN420},
    {u"PN-595",  ts::GUARD_PN595},
    {u"PN-945",  ts::GUARD_PN945},
});

const ts::Enumeration ts::HierarchyEnum({
    {u"auto", ts::HIERARCHY_AUTO},
    {u"none", ts::HIERARCHY_NONE},
    {u"1",    ts::HIERARCHY_1},
    {u"2",    ts::HIERARCHY_2},
    {u"4",    ts::HIERARCHY_4},
});

const ts::Enumeration ts::SpectralInversionEnum({
    {u"off",  ts::SPINV_OFF},
    {u"on",   ts::SPINV_ON},
    {u"auto", ts::SPINV_AUTO},
});

const ts::Enumeration ts::PLSModeEnum({
    {u"ROOT",  ts::PLS_ROOT},
    {u"GOLD",  ts::PLS_GOLD},
    {u"COMBO", ts::PLS_COMBO},
});


//----------------------------------------------------------------------------
// Compute the number of bits per symbol for a specified modulation.
// Return zero if unknown
//----------------------------------------------------------------------------

uint32_t ts::BitsPerSymbol(Modulation modulation)
{
    switch (modulation) {
        case QPSK:     return 2;  // Q (in QPSK) = quad = 4 states = 2 bits
        case PSK_8:    return 3;  // 8 states = 3 bits
        case QAM_16:   return 4;  // 16 states = 4 bits
        case QAM_32:   return 5;  // 32 states = 5 bits
        case QAM_64:   return 6;  // 64 states = 6 bits
        case QAM_128:  return 7;  // 128 states = 7 bits
        case QAM_256:  return 8;  // 256 states = 8 bits
        case QAM_AUTO: return 0;  // Unknown
        case VSB_8:    return 3;  // 8 states = 3 bits
        case VSB_16:   return 4;  // 16 states = 4 bits
        case APSK_16:  return 4;  // 16 states = 4 bits
        case APSK_32:  return 5;  // 32 states = 5 bits
        case DQPSK:    return 2;  // Q = 4 states = 2 bits
        case QAM_4_NR: return 2;  // 4 states = 2 bits
        default:       return 0;  // Unknown
    }
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a FEC value.
// Return zero if unknown
//----------------------------------------------------------------------------

uint32_t ts::FECMultiplier(InnerFEC fec)
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
        case FEC_AUTO: return 0; // Unknown
        default:       return 0; // Unknown
    }
}

uint32_t ts::FECDivider(InnerFEC fec)
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
        case FEC_AUTO: return 0; // Unknown
        default:       return 0; // Unknown
    }
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a guard interval value.
//----------------------------------------------------------------------------

uint32_t ts::GuardIntervalMultiplier(GuardInterval guard)
{
    switch (guard) {
        case GUARD_1_4:    return 1;
        case GUARD_1_8:    return 1;
        case GUARD_1_16:   return 1;
        case GUARD_1_32:   return 1;
        case GUARD_1_128:  return 1;
        case GUARD_19_128: return 19;
        case GUARD_19_256: return 19;
        case GUARD_PN420:  return 0; // unknown
        case GUARD_PN595:  return 0; // unknown
        case GUARD_PN945:  return 0; // unknown
        case GUARD_AUTO:   return 0; // unknown
        default:           return 0; // unknown
    }
}


uint32_t ts::GuardIntervalDivider(GuardInterval guard)
{
    switch (guard) {
        case GUARD_1_4:    return 4;
        case GUARD_1_8:    return 8;
        case GUARD_1_16:   return 16;
        case GUARD_1_32:   return 32;
        case GUARD_1_128:  return 128;
        case GUARD_19_128: return 128;
        case GUARD_19_256: return 256;
        case GUARD_PN420:  return 0; // unknown
        case GUARD_PN595:  return 0; // unknown
        case GUARD_PN945:  return 0; // unknown
        case GUARD_AUTO:   return 0; // unknown
        default:           return 0; // unknown
    }
}
