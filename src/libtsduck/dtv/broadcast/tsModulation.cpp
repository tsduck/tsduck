//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsModulation.h"


//----------------------------------------------------------------------------
// This function checks that an enumeration value is supported by
// the native implementation. If it is not, report an error message
// and return false.
//----------------------------------------------------------------------------

bool ts::CheckModEnum(int value, const UString& name, const Names& conv, Report& report)
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
                     , name, conv.name(value));
        return false;
    }
}


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Names& ts::ModulationEnum()
{
    static const Names data {
        {u"QPSK",      QPSK},
        {u"8-PSK",     PSK_8},
        {u"QAM",       QAM_AUTO},
        {u"16-QAM",    QAM_16},
        {u"32-QAM",    QAM_32},
        {u"64-QAM",    QAM_64},
        {u"128-QAM",   QAM_128},
        {u"256-QAM",   QAM_256},
        {u"8-VSB",     VSB_8},
        {u"16-VSB",    VSB_16},
        {u"16-APSK",   APSK_16},
        {u"32-APSK",   APSK_32},
        {u"DQPSK",     DQPSK},
        {u"4-QAM-NR",  QAM_4_NR},
        {u"1024-QAM",  QAM_1024},
        {u"4096-QAM",  QAM_4096},
        {u"8-APSK-L",  APSK_8_L},
        {u"16-APSK-L", APSK_16_L},
        {u"32-APSK-L", APSK_32_L},
        {u"64-APSK",   APSK_64},
        {u"64-APSK-L", APSK_64_L},
    };
    return data;
}

const ts::Names& ts::InnerFECEnum()
{
    static const Names data {
        {u"none",  FEC_NONE},
        {u"auto",  FEC_AUTO},
        {u"1/2",   FEC_1_2},
        {u"2/3",   FEC_2_3},
        {u"3/4",   FEC_3_4},
        {u"4/5",   FEC_4_5},
        {u"5/6",   FEC_5_6},
        {u"6/7",   FEC_6_7},
        {u"7/8",   FEC_7_8},
        {u"8/9",   FEC_8_9},
        {u"9/10",  FEC_9_10},
        {u"3/5",   FEC_3_5},
        {u"1/3",   FEC_1_3},
        {u"1/4",   FEC_1_4},
        {u"2/5",   FEC_2_5},
        {u"5/11",  FEC_5_11},
        {u"5/9",   FEC_5_9},
        {u"7/9",   FEC_7_9},
        {u"8/15",  FEC_8_15},
        {u"11/15", FEC_11_15},
        {u"13/18", FEC_13_18},
        {u"9/20",  FEC_9_20},
        {u"11/20", FEC_11_20},
        {u"23/36", FEC_23_36},
        {u"25/36", FEC_25_36},
        {u"13/45", FEC_13_45},
        {u"26/45", FEC_26_45},
        {u"28/45", FEC_28_45},
        {u"32/45", FEC_32_45},
        {u"77/90", FEC_77_90},
        {u"11/45", FEC_11_45},
        {u"4/15",  FEC_4_15},
        {u"14/45", FEC_14_45},
        {u"7/15",  FEC_7_15},
    };
    return data;
}

const ts::Names& ts::PolarizationEnum()
{
    static const Names data {
        {u"none",       POL_NONE},
        {u"auto",       POL_AUTO},
        {u"horizontal", POL_HORIZONTAL},
        {u"vertical",   POL_VERTICAL},
        {u"left",       POL_LEFT},
        {u"right",      POL_RIGHT},
    };
    return data;
}

const ts::Names& ts::PilotEnum()
{
    static const Names data {
        {u"auto", PILOT_AUTO},
        {u"on",   PILOT_ON},
        {u"off",  PILOT_OFF},
    };
    return data;
}

const ts::Names& ts::RollOffEnum()
{
    static const Names data {
        {u"auto", ROLLOFF_AUTO},
        {u"0.35", ROLLOFF_35},
        {u"0.25", ROLLOFF_25},
        {u"0.20", ROLLOFF_20},
        {u"0.15", ROLLOFF_15},
        {u"0.10", ROLLOFF_10},
        {u"0.05", ROLLOFF_5},
    };
    return data;
}

const ts::Names& ts::TransmissionModeEnum()
{
    static const Names data {
        {u"auto",           TM_AUTO},
        {u"2K",             TM_2K},
        {u"4K",             TM_4K},
        {u"8K",             TM_8K},
        {u"2K-interleaved", TM_2KI},
        {u"4K-interleaved", TM_4KI},
        {u"1K",             TM_1K},
        {u"16K",            TM_16K},
        {u"32K",            TM_32K},
        {u"C=1",            TM_C1},
        {u"C=3780",         TM_C3780},
    };
    return data;
}

const ts::Names& ts::GuardIntervalEnum()
{
    static const Names data {
        {u"auto",    GUARD_AUTO},
        {u"1/32",    GUARD_1_32},
        {u"1/16",    GUARD_1_16},
        {u"1/8",     GUARD_1_8},
        {u"1/4",     GUARD_1_4},
        {u"1/128",   GUARD_1_128},
        {u"19/128",  GUARD_19_128},
        {u"19/256",  GUARD_19_256},
        {u"PN-420",  GUARD_PN420},
        {u"PN-595",  GUARD_PN595},
        {u"PN-945",  GUARD_PN945},
        {u"1/64",    GUARD_1_64},
    };
    return data;
}

const ts::Names& ts::HierarchyEnum()
{
    static const Names data {
        {u"auto", HIERARCHY_AUTO},
        {u"none", HIERARCHY_NONE},
        {u"1",    HIERARCHY_1},
        {u"2",    HIERARCHY_2},
        {u"4",    HIERARCHY_4},
    };
    return data;
}

const ts::Names& ts::SpectralInversionEnum()
{
    static const Names data {
        {u"off",  SPINV_OFF},
        {u"on",   SPINV_ON},
        {u"auto", SPINV_AUTO},
    };
    return data;
}

const ts::Names& ts::PLSModeEnum()
{
    static const Names data {
        {u"ROOT", PLS_ROOT},
        {u"GOLD", PLS_GOLD},
    };
    return data;
}


//----------------------------------------------------------------------------
// Compute the number of bits per symbol for a specified modulation.
// Return zero if unknown
//----------------------------------------------------------------------------

namespace {
    const std::map<ts::Modulation,uint32_t> _BitsPerSymbol {
        {ts::QPSK,      2},  // Q (in QPSK) = quad = 4 states = 2 bits
        {ts::PSK_8,     3},  // 8 states = 3 bits
        {ts::QAM_16,    4},  // 16 states = 4 bits
        {ts::QAM_32,    5},  // 32 states = 5 bits
        {ts::QAM_64,    6},  // 64 states = 6 bits
        {ts::QAM_128,   7},  // 128 states = 7 bits
        {ts::QAM_256,   8},  // 256 states = 8 bits
        {ts::QAM_AUTO,  0},  // Unknown
        {ts::VSB_8,     3},  // 8 states = 3 bits
        {ts::VSB_16,    4},  // 16 states = 4 bits
        {ts::APSK_16,   4},  // 16 states = 4 bits
        {ts::APSK_32,   5},  // 32 states = 5 bits
        {ts::DQPSK,     2},  // Q = 4 states = 2 bits
        {ts::QAM_4_NR,  2},  // 4 states = 2 bits
        {ts::QAM_1024, 10},  // 1024 states = 10 bits
        {ts::QAM_4096, 12},  // 4096 states = 12 bits
        {ts::APSK_8_L,  3},  // 8 states = 3 bits
        {ts::APSK_16_L, 4},  // 16 states = 4 bits
        {ts::APSK_32_L, 5},  // 32 states = 5 bits
        {ts::APSK_64,   6},  // 64 states = 6 bits
        {ts::APSK_64_L, 6},  // 64 states = 6 bits
    };
}

uint32_t ts::BitsPerSymbol(Modulation modulation)
{
    const auto it = _BitsPerSymbol.find(modulation);
    return it == _BitsPerSymbol.end() ? 0 : it->second;
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a FEC value.
// Return zero if unknown
//----------------------------------------------------------------------------

namespace {
    const std::pair<uint32_t,uint32_t>& FECFraction(ts::InnerFEC fec)
    {
        static const std::pair<uint32_t,uint32_t> unknown {0, 1};

        static const std::map<ts::InnerFEC, std::pair<uint32_t,uint32_t>> fraction {
            {ts::FEC_NONE,  { 1,  1}}, // none means 1/1
            {ts::FEC_1_2,   { 1,  2}},
            {ts::FEC_2_3,   { 2,  3}},
            {ts::FEC_3_4,   { 3,  4}},
            {ts::FEC_4_5,   { 4,  5}},
            {ts::FEC_5_6,   { 5,  6}},
            {ts::FEC_6_7,   { 6,  7}},
            {ts::FEC_7_8,   { 7,  8}},
            {ts::FEC_8_9,   { 8,  9}},
            {ts::FEC_9_10,  { 9, 10}},
            {ts::FEC_3_5,   { 3,  5}},
            {ts::FEC_1_3,   { 1,  3}},
            {ts::FEC_1_4,   { 1,  4}},
            {ts::FEC_2_5,   { 2,  5}},
            {ts::FEC_5_11,  { 5, 11}},
            {ts::FEC_5_9,   { 5,  9}},
            {ts::FEC_7_9,   { 7,  9}},
            {ts::FEC_8_15,  { 8, 15}},
            {ts::FEC_11_15, {11, 15}},
            {ts::FEC_13_18, {13, 18}},
            {ts::FEC_9_20,  { 9, 20}},
            {ts::FEC_11_20, {11, 20}},
            {ts::FEC_23_36, {23, 36}},
            {ts::FEC_25_36, {25, 36}},
            {ts::FEC_13_45, {13, 45}},
            {ts::FEC_26_45, {26, 45}},
            {ts::FEC_28_45, {28, 45}},
            {ts::FEC_32_45, {32, 45}},
            {ts::FEC_77_90, {77, 90}},
            {ts::FEC_11_45, {11, 45}},
            {ts::FEC_4_15,  { 4, 15}},
            {ts::FEC_14_45, {14, 45}},
            {ts::FEC_7_15,  { 7, 15}},
            {ts::FEC_AUTO,  { 0,  1}}  // Unknown
        };

        const auto it = fraction.find(fec);
        return it == fraction.end() ? unknown : it->second;
    }
}

uint32_t ts::FECMultiplier(InnerFEC fec)
{
    return FECFraction(fec).first;
}

uint32_t ts::FECDivider(InnerFEC fec)
{
    return FECFraction(fec).second;
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a guard interval value.
//----------------------------------------------------------------------------

namespace {
    const std::pair<uint32_t,uint32_t>& GuardFraction(ts::GuardInterval guard)
    {
        static const std::pair<uint32_t,uint32_t> unknown {0, 1};

        static const std::map<ts::GuardInterval, std::pair<uint32_t,uint32_t>> fraction {
            {ts::GUARD_1_4,    { 1,   4}},
            {ts::GUARD_1_8,    { 1,   8}},
            {ts::GUARD_1_16,   { 1,  16}},
            {ts::GUARD_1_32,   { 1,  32}},
            {ts::GUARD_1_128,  { 1, 128}},
            {ts::GUARD_19_128, {19, 128}},
            {ts::GUARD_19_256, {19, 256}},
            {ts::GUARD_PN420,  { 0,   1}}, // unknown
            {ts::GUARD_PN595,  { 0,   1}}, // unknown
            {ts::GUARD_PN945,  { 0,   1}}, // unknown
            {ts::GUARD_1_4,    { 1,   4}},
            {ts::GUARD_AUTO,   { 0,   1}}  // unknown
        };

        const auto it = fraction.find(guard);
        return it == fraction.end() ? unknown : it->second;
    }
}

uint32_t ts::GuardIntervalMultiplier(GuardInterval guard)
{
    return GuardFraction(guard).first;
}


uint32_t ts::GuardIntervalDivider(GuardInterval guard)
{
    return GuardFraction(guard).second;
}


//----------------------------------------------------------------------------
// Convert PLS code values between GOLD and ROOT modes.
//----------------------------------------------------------------------------

uint32_t ts::PLSCodeGoldToRoot(uint32_t gold)
{
    uint32_t x = 1;
    uint32_t g = 0;
    while (g < gold) {
        x = (((x ^ (x >> 7)) & 1) << 17) | (x >> 1);
        g++;
    }
    return x;
}

uint32_t ts::PLSCodeRootToGold(uint32_t root)
{
    uint32_t x = 1;
    uint32_t g = 0;
    while (g < 0x3FFFF)  {
        if (root == x) {
            return g;
        }
        x = (((x ^ (x >> 7)) & 1) << 17) | (x >> 1);
        g++;
    }
    return 0xFFFFFFFF;
}
