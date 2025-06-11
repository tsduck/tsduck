//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDektecUtils.h"
#include "tsDektec.h"
#include "tsFeatures.h"
#include "tsModulationArgs.h"
#include "tsCerrReport.h"

TS_CERR_DEBUG(u"libtsdektec loaded");


//-----------------------------------------------------------------------------
// Get the versions of Dektec API and drivers in one single string.
//-----------------------------------------------------------------------------

// Register for options --version and --support.
TS_REGISTER_FEATURE(u"dektec", u"Dektec", ts::Features::SUPPORTED, ts::GetDektecVersions);

ts::UString ts::GetDektecVersions()
{
    std::map<UString,UString> versions;
    GetDektecVersions(versions);

    UString result;
    for (const auto& it : versions) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(it.first);
        result.append(u": ");
        result.append(it.second);
    }
    return result;
}


//-----------------------------------------------------------------------------
// Get the versions of Dektec API and drivers.
//-----------------------------------------------------------------------------

void ts::GetDektecVersions(std::map<UString,UString>& versions)
{
    versions.clear();

    int major = 0;
    int minor = 0;
    int bugfix = 0;
    int build = 0;

    // DTAPI version is always available.
    Dtapi::DtapiGetVersion(major, minor, bugfix, build);
    versions[u"DTAPI"].format(u"%d.%d.%d.%d", major, minor, bugfix, build);

    // DTAPI service is optional.
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDtapiServiceVersion(major, minor, bugfix, build) == DTAPI_OK) {
        versions[u"Service"].format(u"%d.%d.%d.%d", major, minor, bugfix, build);
    }

    // Get all Dektec drivers versions.
    std::vector<Dtapi::DtDriverVersionInfo> drv;
    if (Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_ALL, drv) == DTAPI_OK) {
        for (const auto& it : drv) {
            versions[UString::FromWChar(it.m_Name)].format(u"%d.%d.%d.%d", it.m_Major, it.m_Minor, it.m_BugFix, it.m_Build);
        }
    }
}


//-----------------------------------------------------------------------------
// Enumeration for various Dektec constants, names for values
//-----------------------------------------------------------------------------

const ts::Names& ts::DektecModulationTypes()
{
    static const Names data {
        {u"DVBS-QPSK",    DTAPI_MOD_DVBS_QPSK},
        {u"DVBS-BPSK",    DTAPI_MOD_DVBS_BPSK},
        {u"4-QAM",        DTAPI_MOD_QAM4},
        {u"16-QAM",       DTAPI_MOD_QAM16},
        {u"32-QAM",       DTAPI_MOD_QAM32},
        {u"64-QAM",       DTAPI_MOD_QAM64},
        {u"128-QAM",      DTAPI_MOD_QAM128},
        {u"256-QAM",      DTAPI_MOD_QAM256},
        {u"DVBT",         DTAPI_MOD_DVBT},
        {u"ATSC",         DTAPI_MOD_ATSC},
        {u"DVB-T2",       DTAPI_MOD_DVBT2},
        {u"ISDB-T",       DTAPI_MOD_ISDBT},
        {u"IQDIRECT",     DTAPI_MOD_IQDIRECT},
        {u"DVBS2-QPSK",   DTAPI_MOD_DVBS2_QPSK},
        {u"DVBS2-8PSK",   DTAPI_MOD_DVBS2_8PSK},
        {u"DVBS2-16APSK", DTAPI_MOD_DVBS2_16APSK},
        {u"DVBS2-32APSK", DTAPI_MOD_DVBS2_32APSK},
        {u"DMB-TH",       DTAPI_MOD_DMBTH},
        {u"ADTB-T",       DTAPI_MOD_ADTBT},
        {u"CMMB",         DTAPI_MOD_CMMB},
        {u"T2MI",         DTAPI_MOD_T2MI},
        {u"DVBC2",        DTAPI_MOD_DVBC2}
    };
    return data;
}

const ts::Names& ts::DektecVSB()
{
    static const Names data {
        {u"8-VSB",  DTAPI_MOD_ATSC_VSB8},
        {u"16-VSB", DTAPI_MOD_ATSC_VSB16},
    };
    return data;
}

const ts::Names& ts::DektecFEC()
{
    static const Names data {
        {u"1/2",         DTAPI_MOD_1_2},
        {u"2/3",         DTAPI_MOD_2_3},
        {u"3/4",         DTAPI_MOD_3_4},
        {u"4/5",         DTAPI_MOD_4_5},
        {u"5/6",         DTAPI_MOD_5_6},
        {u"6/7",         DTAPI_MOD_6_7},
        {u"7/8",         DTAPI_MOD_7_8},
        {u"1/4",         DTAPI_MOD_1_4},
        {u"1/3",         DTAPI_MOD_1_3},
        {u"2/5",         DTAPI_MOD_2_5},
        {u"3/5",         DTAPI_MOD_3_5},
        {u"8/9",         DTAPI_MOD_8_9},
        {u"9/10",        DTAPI_MOD_9_10},
        {u"unknown-FEC", DTAPI_MOD_CR_UNK},
    };
    return data;
}

const ts::Names& ts::DektecInversion()
{
    static const Names data {
        {u"non-inverted", DTAPI_MOD_S_S2_SPECNONINV},
        {u"inverted",     DTAPI_MOD_S_S2_SPECINV},
    };
    return data;
}

const ts::Names& ts::DektecDVBTProperty()
{
    static const Names data {
        {u"5-MHz",                     DTAPI_MOD_DVBT_5MHZ},
        {u"6-MHz",                     DTAPI_MOD_DVBT_6MHZ},
        {u"7-MHz",                     DTAPI_MOD_DVBT_7MHZ},
        {u"8-MHz",                     DTAPI_MOD_DVBT_8MHZ},
        {u"unknown-bandwidth",         DTAPI_MOD_DVBT_BW_UNK},
        {u"QPSK",                      DTAPI_MOD_DVBT_QPSK},
        {u"16-QAM",                    DTAPI_MOD_DVBT_QAM16},
        {u"64-QAM",                    DTAPI_MOD_DVBT_QAM64},
        {u"unknown-constellation",     DTAPI_MOD_DVBT_CO_UNK},
        {u"1/32",                      DTAPI_MOD_DVBT_G_1_32},
        {u"1/16",                      DTAPI_MOD_DVBT_G_1_16},
        {u"1/8",                       DTAPI_MOD_DVBT_G_1_8},
        {u"1/4",                       DTAPI_MOD_DVBT_G_1_4},
        {u"unknown-guard-interval",    DTAPI_MOD_DVBT_GU_UNK},
        {u"indepth-interleave",        DTAPI_MOD_DVBT_INDEPTH},
        {u"native-interleave",         DTAPI_MOD_DVBT_NATIVE},
        {u"2K",                        DTAPI_MOD_DVBT_2K},
        {u"4K",                        DTAPI_MOD_DVBT_4K},
        {u"8K",                        DTAPI_MOD_DVBT_8K},
        {u"unknown-transmission-mode", DTAPI_MOD_DVBT_MD_UNK},
    };
    return data;
}

const ts::Names& ts::DektecPowerMode()
{
    static const Names data {
        {u"high-quality", DTAPI_IOCONFIG_MODHQ},
        {u"low-power",    DTAPI_IOCONFIG_LOWPWR},
    };
    return data;
}


//----------------------------------------------------------------------------
// Attempt to get a "FEC type" for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::GetDektecCodeRate(int& fec, const ModulationArgs& args)
{
    return ToDektecCodeRate(fec, args.inner_fec.value_or(ModulationArgs::DEFAULT_INNER_FEC));
}

bool ts::ToDektecCodeRate(int& fec, InnerFEC in_enum)
{
    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(switch-default)
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    bool supported = true;
    switch (in_enum) {
        case FEC_1_2:  fec = DTAPI_MOD_1_2; break;
        case FEC_1_3:  fec = DTAPI_MOD_1_3; break;
        case FEC_1_4:  fec = DTAPI_MOD_1_4; break;
        case FEC_2_3:  fec = DTAPI_MOD_2_3; break;
        case FEC_2_5:  fec = DTAPI_MOD_2_5; break;
        case FEC_3_4:  fec = DTAPI_MOD_3_4; break;
        case FEC_3_5:  fec = DTAPI_MOD_3_5; break;
        case FEC_4_5:  fec = DTAPI_MOD_4_5; break;
        case FEC_5_6:  fec = DTAPI_MOD_5_6; break;
        case FEC_6_7:  fec = DTAPI_MOD_6_7; break;
        case FEC_7_8:  fec = DTAPI_MOD_7_8; break;
        case FEC_8_9:  fec = DTAPI_MOD_8_9; break;
        case FEC_9_10: fec = DTAPI_MOD_9_10; break;
        default: supported = false; break;
    }
    return supported;

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Attempt to get a "modulation type" for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::GetDektecModulationType(int& type, const ModulationArgs& args)
{
    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(switch-default)
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    // Determine modulation type.
    bool supported = true;
    switch (args.delivery_system.value_or(DS_UNDEFINED)) {
        case DS_DVB_S:   type = DTAPI_MOD_DVBS_QPSK; break;
        case DS_DVB_T:   type = DTAPI_MOD_DVBT; break;
        case DS_DVB_T2:  type = DTAPI_MOD_DVBT2; break;
        case DS_ATSC:    type = DTAPI_MOD_ATSC; break;
        case DS_ATSC_MH: type = DTAPI_MOD_ATSC_MH; break;
        case DS_ISDB_S:  type = DTAPI_MOD_ISDBS; break;
        case DS_ISDB_T:  type = DTAPI_MOD_ISDBT; break;
        case DS_DVB_C2:  type = DTAPI_MOD_DVBC2; break;
        case DS_DAB:     type = DTAPI_MOD_DAB; break;
        case DS_CMMB:    type = DTAPI_MOD_CMMB; break;
        case DS_DVB_S2:
            switch (args.modulation.value_or(ModulationArgs::DEFAULT_MODULATION_DVBS)) {
                case QPSK:    type = DTAPI_MOD_DVBS2_QPSK; break;
                case PSK_8:   type = DTAPI_MOD_DVBS2_8PSK; break;
                case APSK_16: type = DTAPI_MOD_DVBS2_16APSK; break;
                case APSK_32: type = DTAPI_MOD_DVBS2_32APSK; break;
                default:      type = DTAPI_MOD_DVBS2; break;
            }
            break;
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C_ANNEX_C:
            switch (args.modulation.value_or(QAM_AUTO)) {
                case QAM_16:  type = DTAPI_MOD_QAM16;  break;
                case QAM_32:  type = DTAPI_MOD_QAM32;  break;
                case QAM_64:  type = DTAPI_MOD_QAM64;  break;
                case QAM_128: type = DTAPI_MOD_QAM128; break;
                case QAM_256: type = DTAPI_MOD_QAM256; break;
                default: supported = false; break;
            }
            break;
        default:
            supported = false;
            break;
    }
    return supported;

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Attempt to convert the tuning parameters for Dektec modulator cards.
//----------------------------------------------------------------------------

bool ts::GetDektecModulation(int& modulation_type, int& param0, int& param1, int& param2, const ModulationArgs& args)
{
    // Get known parameters.
    if (!GetDektecModulationType(modulation_type, args) || !GetDektecCodeRate(param0, args)) {
        return false;
    }

    // Additional parameters param1 and param2
    param1 = param2 = 0;
    if (args.delivery_system == DS_DVB_S2) {
        param1 = args.pilots.value_or(ModulationArgs::DEFAULT_PILOTS) == PILOT_ON ? DTAPI_MOD_S2_PILOTS : DTAPI_MOD_S2_NOPILOTS;
        // Assume long FEC frame for broadcast service (should be updated by caller if necessary).
        param1 |= DTAPI_MOD_S2_LONGFRM;
        // Roll-off
        switch (args.roll_off.value_or(ModulationArgs::DEFAULT_ROLL_OFF)) {
            case ROLLOFF_AUTO: param1 |= DTAPI_MOD_ROLLOFF_AUTO; break;
            case ROLLOFF_20:   param1 |= DTAPI_MOD_ROLLOFF_20; break;
            case ROLLOFF_25:   param1 |= DTAPI_MOD_ROLLOFF_25; break;
            case ROLLOFF_35:   param1 |= DTAPI_MOD_ROLLOFF_35; break;
            case ROLLOFF_15:   param1 |= DTAPI_MOD_ROLLOFF_15; break;
            case ROLLOFF_10:   param1 |= DTAPI_MOD_ROLLOFF_10; break;
            case ROLLOFF_5:    param1 |= DTAPI_MOD_ROLLOFF_5; break;
            default: break;
        }
        // Physical layer scrambling initialization sequence
        param2 = int(args.pls_code.value_or(ModulationArgs::DEFAULT_PLS_CODE));
    }

    return true;
}


//----------------------------------------------------------------------------
// Attempt to compute a bitrate from a ModulationArgs using the DTAPI.
//----------------------------------------------------------------------------

// This function can be used to compute any type of bitrate, if supported by the DTAPI library.
TS_REGISTER_BITRATE_CALCULATOR(ts::GetDektecBitRate, {});

bool ts::GetDektecBitRate(BitRate& bitrate, const ModulationArgs& args)
{
    int mod = 0, param0 = 0, param1 = 0, param2 = 0, irate = 0;
    const uint32_t symrate = args.symbol_rate.value_or(ModulationArgs::DEFAULT_SYMBOL_RATE_DVBS);
    Dtapi::DtFractionInt frate;
    if (GetDektecModulation(mod, param0, param1, param2, args)) {
        // Successfully found Dektec modulation parameters. Compute the bitrate in fractional form first.
        // It has been observed that the values from the DtFractionInt are sometimes negative.
        // This is a DTAPI bug, probably due to some internal integer overflow.
        if (Dtapi::DtapiModPars2TsRate(frate, mod, param0, param1, param2, int(symrate)) == DTAPI_OK && frate.m_Num > 0 && frate.m_Den > 0) {
            FromDektecFractionInt(bitrate, frate);
        }
        else if (Dtapi::DtapiModPars2TsRate(irate, mod, param0, param1, param2, int(symrate)) == DTAPI_OK && irate > 0) {
            // The fractional version failed or returned a negative value. Used the int version.
            bitrate = irate;
        }
        return bitrate > 0;
    }
    return false;
}
