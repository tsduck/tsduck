//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
                     , name, conv.name(value));
        return false;
    }
}


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::ModulationEnum, ({
    {u"QPSK",      ts::QPSK},
    {u"8-PSK",     ts::PSK_8},
    {u"QAM",       ts::QAM_AUTO},
    {u"16-QAM",    ts::QAM_16},
    {u"32-QAM",    ts::QAM_32},
    {u"64-QAM",    ts::QAM_64},
    {u"128-QAM",   ts::QAM_128},
    {u"256-QAM",   ts::QAM_256},
    {u"8-VSB",     ts::VSB_8},
    {u"16-VSB",    ts::VSB_16},
    {u"16-APSK",   ts::APSK_16},
    {u"32-APSK",   ts::APSK_32},
    {u"DQPSK",     ts::DQPSK},
    {u"4-QAM-NR",  ts::QAM_4_NR},
    {u"1024-QAM",  ts::QAM_1024},
    {u"4096-QAM",  ts::QAM_4096},
    {u"8-APSK-L",  ts::APSK_8_L},
    {u"16-APSK-L", ts::APSK_16_L},
    {u"32-APSK-L", ts::APSK_32_L},
    {u"64-APSK",   ts::APSK_64},
    {u"64-APSK-L", ts::APSK_64_L},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::InnerFECEnum, ({
    {u"none",  ts::FEC_NONE},
    {u"auto",  ts::FEC_AUTO},
    {u"1/2",   ts::FEC_1_2},
    {u"2/3",   ts::FEC_2_3},
    {u"3/4",   ts::FEC_3_4},
    {u"4/5",   ts::FEC_4_5},
    {u"5/6",   ts::FEC_5_6},
    {u"6/7",   ts::FEC_6_7},
    {u"7/8",   ts::FEC_7_8},
    {u"8/9",   ts::FEC_8_9},
    {u"9/10",  ts::FEC_9_10},
    {u"3/5",   ts::FEC_3_5},
    {u"1/3",   ts::FEC_1_3},
    {u"1/4",   ts::FEC_1_4},
    {u"2/5",   ts::FEC_2_5},
    {u"5/11",  ts::FEC_5_11},
    {u"5/9",   ts::FEC_5_9},
    {u"7/9",   ts::FEC_7_9},
    {u"8/15",  ts::FEC_8_15},
    {u"11/15", ts::FEC_11_15},
    {u"13/18", ts::FEC_13_18},
    {u"9/20",  ts::FEC_9_20},
    {u"11/20", ts::FEC_11_20},
    {u"23/36", ts::FEC_23_36},
    {u"25/36", ts::FEC_25_36},
    {u"13/45", ts::FEC_13_45},
    {u"26/45", ts::FEC_26_45},
    {u"28/45", ts::FEC_28_45},
    {u"32/45", ts::FEC_32_45},
    {u"77/90", ts::FEC_77_90},
    {u"11/45", ts::FEC_11_45},
    {u"4/15",  ts::FEC_4_15},
    {u"14/45", ts::FEC_14_45},
    {u"7/15",  ts::FEC_7_15},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::PolarizationEnum, ({
    {u"none",       ts::POL_NONE},
    {u"auto",       ts::POL_AUTO},
    {u"horizontal", ts::POL_HORIZONTAL},
    {u"vertical",   ts::POL_VERTICAL},
    {u"left",       ts::POL_LEFT},
    {u"right",      ts::POL_RIGHT},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::PilotEnum, ({
    {u"auto",       ts::PILOT_AUTO},
    {u"on",         ts::PILOT_ON},
    {u"off",        ts::PILOT_OFF},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::RollOffEnum, ({
    {u"auto",       ts::ROLLOFF_AUTO},
    {u"0.35",       ts::ROLLOFF_35},
    {u"0.25",       ts::ROLLOFF_25},
    {u"0.20",       ts::ROLLOFF_20},
    {u"0.15",       ts::ROLLOFF_15},
    {u"0.10",       ts::ROLLOFF_10},
    {u"0.05",       ts::ROLLOFF_5},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::TransmissionModeEnum, ({
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
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::GuardIntervalEnum, ({
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
    {u"1/64",    ts::GUARD_1_64},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::HierarchyEnum, ({
    {u"auto", ts::HIERARCHY_AUTO},
    {u"none", ts::HIERARCHY_NONE},
    {u"1",    ts::HIERARCHY_1},
    {u"2",    ts::HIERARCHY_2},
    {u"4",    ts::HIERARCHY_4},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::SpectralInversionEnum, ({
    {u"off",  ts::SPINV_OFF},
    {u"on",   ts::SPINV_ON},
    {u"auto", ts::SPINV_AUTO},
}));

TS_DEFINE_GLOBAL(const, ts::Enumeration, ts::PLSModeEnum, ({
    {u"ROOT", ts::PLS_ROOT},
    {u"GOLD", ts::PLS_GOLD},
}));


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
    const std::map<ts::InnerFEC,std::pair<uint32_t,uint32_t>> _FECFraction {
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
}

uint32_t ts::FECMultiplier(InnerFEC fec)
{
    const auto it = _FECFraction.find(fec);
    return it == _FECFraction.end() ? 0 : it->second.first;
}

uint32_t ts::FECDivider(InnerFEC fec)
{
    const auto it = _FECFraction.find(fec);
    return it == _FECFraction.end() ? 1 : it->second.second;
}


//----------------------------------------------------------------------------
// Compute the multiplier and divider of a guard interval value.
//----------------------------------------------------------------------------

namespace {
    const std::map<ts::GuardInterval,std::pair<uint32_t,uint32_t>> _GuardFraction {
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
}

uint32_t ts::GuardIntervalMultiplier(GuardInterval guard)
{
    const auto it = _GuardFraction.find(guard);
    return it == _GuardFraction.end() ? 0 : it->second.first;
}


uint32_t ts::GuardIntervalDivider(GuardInterval guard)
{
    const auto it = _GuardFraction.find(guard);
    return it == _GuardFraction.end() ? 1 : it->second.second;
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
