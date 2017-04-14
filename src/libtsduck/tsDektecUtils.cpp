//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#include "tsDektecUtils.h"
#include "tsDektec.h"
#include "tsFormat.h"


//-----------------------------------------------------------------------------
// Enumeration for various Dektec constants, names for values
//-----------------------------------------------------------------------------

const ts::Enumeration ts::DektecModulationTypes(
#if !defined(TS_NO_DTAPI)
    "DVBS-QPSK", DTAPI_MOD_DVBS_QPSK,
    "DVBS-BPSK", DTAPI_MOD_DVBS_BPSK,
    "4-QAM", DTAPI_MOD_QAM4,
    "16-QAM", DTAPI_MOD_QAM16,
    "32-QAM", DTAPI_MOD_QAM32,
    "64-QAM", DTAPI_MOD_QAM64,
    "128-QAM", DTAPI_MOD_QAM128,
    "256-QAM", DTAPI_MOD_QAM256,
    "DVBT", DTAPI_MOD_DVBT,
    "ATSC", DTAPI_MOD_ATSC,
    "DVB-T2", DTAPI_MOD_DVBT2,
    "ISDB-T", DTAPI_MOD_ISDBT,
    "IQDIRECT", DTAPI_MOD_IQDIRECT,
    "DVBS2-QPSK", DTAPI_MOD_DVBS2_QPSK,
    "DVBS2-8PSK", DTAPI_MOD_DVBS2_8PSK,
    "DVBS2-16APSK", DTAPI_MOD_DVBS2_16APSK,
    "DVBS2-32APSK", DTAPI_MOD_DVBS2_32APSK,
    "DMB-TH", DTAPI_MOD_DMBTH,
    "ADTB-T", DTAPI_MOD_ADTBT,
    "CMMB", DTAPI_MOD_CMMB,
    "T2MI", DTAPI_MOD_T2MI,
    "DVBC2", DTAPI_MOD_DVBC2,
#endif
    TS_NULL_CHAR_PTR, 0);

const ts::Enumeration ts::DektecVSB(
#if !defined(TS_NO_DTAPI)
    "8-VSB", DTAPI_MOD_ATSC_VSB8,
    "16-VSB", DTAPI_MOD_ATSC_VSB16,
#endif
    TS_NULL_CHAR_PTR, 0);

const ts::Enumeration ts::DektecFEC(
#if !defined(TS_NO_DTAPI)
    "1/2", DTAPI_MOD_1_2,
    "2/3", DTAPI_MOD_2_3,
    "3/4", DTAPI_MOD_3_4,
    "4/5", DTAPI_MOD_4_5,
    "5/6", DTAPI_MOD_5_6,
    "6/7", DTAPI_MOD_6_7,
    "7/8", DTAPI_MOD_7_8,
    "1/4", DTAPI_MOD_1_4,
    "1/3", DTAPI_MOD_1_3,
    "2/5", DTAPI_MOD_2_5,
    "3/5", DTAPI_MOD_3_5,
    "8/9", DTAPI_MOD_8_9,
    "9/10", DTAPI_MOD_9_10,
    "unknown-FEC", DTAPI_MOD_CR_UNK,
#endif
    TS_NULL_CHAR_PTR, 0);

const ts::Enumeration ts::DektecInversion(
#if !defined(TS_NO_DTAPI)
    "non-inverted", DTAPI_MOD_S_S2_SPECNONINV,
    "inverted", DTAPI_MOD_S_S2_SPECINV,
#endif
    TS_NULL_CHAR_PTR, 0);

const ts::Enumeration ts::DektecDVBTProperty(
#if !defined(TS_NO_DTAPI)
    "5-MHz", DTAPI_MOD_DVBT_5MHZ,
    "6-MHz", DTAPI_MOD_DVBT_6MHZ,
    "7-MHz", DTAPI_MOD_DVBT_7MHZ,
    "8-MHz", DTAPI_MOD_DVBT_8MHZ,
    "unknown-bandwidth", DTAPI_MOD_DVBT_BW_UNK,
    "QPSK", DTAPI_MOD_DVBT_QPSK,
    "16-QAM", DTAPI_MOD_DVBT_QAM16,
    "64-QAM", DTAPI_MOD_DVBT_QAM64,
    "unknown-constellation", DTAPI_MOD_DVBT_CO_UNK,
    "1/32", DTAPI_MOD_DVBT_G_1_32,
    "1/16", DTAPI_MOD_DVBT_G_1_16,
    "1/8", DTAPI_MOD_DVBT_G_1_8,
    "1/4", DTAPI_MOD_DVBT_G_1_4,
    "unknown-guard-interval", DTAPI_MOD_DVBT_GU_UNK,
    "indepth-interleave", DTAPI_MOD_DVBT_INDEPTH,
    "native-interleave", DTAPI_MOD_DVBT_NATIVE,
    "2K", DTAPI_MOD_DVBT_2K,
    "4K", DTAPI_MOD_DVBT_4K,
    "8K", DTAPI_MOD_DVBT_8K,
    "unknown-transmission-mode", DTAPI_MOD_DVBT_MD_UNK,
#endif
    TS_NULL_CHAR_PTR, 0);


//-----------------------------------------------------------------------------
// Get the versions of Dektec API and drivers.
//-----------------------------------------------------------------------------

std::string ts::GetDektecVersions()
{
#if defined(TS_NO_DTAPI)
    return TS_NO_DTAPI_MESSAGE;
#else
    // DTAPI version is always available.
    int major = 0;
    int minor = 0;
    int bugfix = 0;
    int build = 0;
    Dtapi::DtapiGetVersion(major, minor, bugfix, build);
    std::string result(Format("DTAPI: %d.%d.%d.%d", major, minor, bugfix, build));

    // Services and drivers are optional.
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDtapiServiceVersion(major, minor, bugfix, build) == DTAPI_OK) {
        result.append(Format(", Service: %d.%d.%d.%d", major, minor, bugfix, build));
    }
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_PCI, major, minor, bugfix, build) == DTAPI_OK) {
        result.append(Format(", Dta: %d.%d.%d.%d", major, minor, bugfix, build));
    }
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_USB, major, minor, bugfix, build) == DTAPI_OK) {
        result.append(Format(", Dtu: %d.%d.%d.%d", major, minor, bugfix, build));
    }
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_NW, major, minor, bugfix, build) == DTAPI_OK) {
        result.append(Format(", DtaNw: %d.%d.%d.%d", major, minor, bugfix, build));
    }
    major = minor = bugfix = build = 0;
    if (Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_NWAP, major, minor, bugfix, build) == DTAPI_OK) {
        result.append(Format(", DtaNwAp: %d.%d.%d.%d", major, minor, bugfix, build));
    }

    return result;
#endif
}
