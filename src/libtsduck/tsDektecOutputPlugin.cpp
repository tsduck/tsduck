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
//! @file tsDektecOutputPlugin.h
//!
//! Declare the ts::DektecOutputPlugin class.
//!
//----------------------------------------------------------------------------

#include "tsDektecOutputPlugin.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsTunerParameters.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersBitrateDiffDVBT.h"
#include "tsTunerParametersATSC.h"
#include "tsModulation.h"
#include "tsDecimal.h"
#include "tsIntegerUtils.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Stubs when compiled without Dektec support.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

ts::DektecOutputPlugin::DektecOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, "Send packets to a Dektec DVB-ASI or modulator device.", "[options]")
{
    setHelp(TS_NO_DTAPI_MESSAGE "\n");
}

ts::DektecOutputPlugin::~DektecOutputPlugin()
{
}

bool ts::DektecOutputPlugin::start()
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return false;
}

bool ts::DektecOutputPlugin::stop()
{
    return true;
}

ts::BitRate ts::DektecOutputPlugin::getBitrate()
{
    return 0;
}

bool ts::DektecOutputPlugin::send(const TSPacket* buffer, size_t packet_count)
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return false;
}

#else

//----------------------------------------------------------------------------
// Class internals.
//----------------------------------------------------------------------------

class ts::DektecOutputPlugin::Guts
{
public:
    bool                 starting;     // Starting phase (loading FIFO, no transmit)
    bool                 is_started;   // Device started
    bool                 mute_on_stop; // Device supports output muting
    int                  dev_index;    // Dektec device index
    int                  chan_index;   // Device output channel index
    DektecDevice         device;       // Device characteristics
    Dtapi::DtDevice      dtdev;        // Device descriptor
    Dtapi::DtOutpChannel chan;         // Output channel
    int                  detach_mode;  // Detach mode
    BitRate              opt_bitrate;  // Bitrate option (0 means unspecified)
    BitRate              cur_bitrate;  // Current output bitrate

    Guts() :                           // Constructor.
        starting(false),
        is_started(false),
        mute_on_stop(false),
        dev_index(-1),
        chan_index(-1),
        device(),
        dtdev(),
        chan(),
        detach_mode(DTAPI_WAIT_UNTIL_SENT),
        opt_bitrate(0),
        cur_bitrate(0)
    {
    }
};


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::DektecOutputPlugin::DektecOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, "Send packets to a Dektec DVB-ASI or modulator device.", "[options]"),
    _guts(new Guts)
{
    CheckNonNull(_guts);

    // Share same option --bandwidth for DVB-T2 and DVB-T/H
    assert(DTAPI_DVBT2_5MHZ == DTAPI_MOD_DVBT_5MHZ);
    assert(DTAPI_DVBT2_6MHZ == DTAPI_MOD_DVBT_6MHZ);
    assert(DTAPI_DVBT2_7MHZ == DTAPI_MOD_DVBT_7MHZ);
    assert(DTAPI_DVBT2_8MHZ == DTAPI_MOD_DVBT_8MHZ);

    // Share same option --bandwidth for DVB-T2 and DMB-T/H
    assert(DTAPI_DVBT2_5MHZ == DTAPI_MOD_DTMB_5MHZ);
    assert(DTAPI_DVBT2_6MHZ == DTAPI_MOD_DTMB_6MHZ);
    assert(DTAPI_DVBT2_7MHZ == DTAPI_MOD_DTMB_7MHZ);
    assert(DTAPI_DVBT2_8MHZ == DTAPI_MOD_DTMB_8MHZ);

    // Declaration of command-line options
    option("204");
    option("bandwidth", 0, Enumeration (
               "1.7", DTAPI_DVBT2_1_7MHZ,
               "5",   DTAPI_DVBT2_5MHZ,
               "6",   DTAPI_DVBT2_6MHZ,
               "7",   DTAPI_DVBT2_7MHZ,
               "8",   DTAPI_DVBT2_8MHZ,
               "10",  DTAPI_DVBT2_10MHZ,
               TS_NULL));
    option("bandwidth-extension");
    option("bitrate", 'b', POSITIVE);
    option("cell-id", 0,  UINT16);
    option("channel", 'c', UNSIGNED);
    option("cmmb-bandwidth", 0, Enumeration (
               "2", DTAPI_CMMB_BW_2MHZ,
               "8", DTAPI_CMMB_BW_8MHZ,
               TS_NULL));
    option("cmmb-pid", 0, PIDVAL);
    option("cmmb-area-id", 0, INTEGER, 0, 1, 0, 127);
    option("cmmb-tx-id", 0, INTEGER, 0, 1, 0, 127);
    option("constellation", 0, Enumeration (
               "QPSK",   DTAPI_MOD_DVBT_QPSK,
               "16-QAM", DTAPI_MOD_DVBT_QAM16,
               "64-QAM", DTAPI_MOD_DVBT_QAM64,
               TS_NULL));
    option("convolutional-rate", 'r', Enumeration (
               "1/2",  DTAPI_MOD_1_2,
               "1/3",  DTAPI_MOD_1_3,  // DVB-S.2 only
               "1/4",  DTAPI_MOD_1_4,  // DVB-S.2 only
               "2/3",  DTAPI_MOD_2_3,
               "2/5",  DTAPI_MOD_2_5,  // DVB-S.2 only
               "3/4",  DTAPI_MOD_3_4,
               "3/5",  DTAPI_MOD_3_5,  // DVB-S.2 only
               "4/5",  DTAPI_MOD_4_5,
               "5/6",  DTAPI_MOD_5_6,
               "6/7",  DTAPI_MOD_6_7,
               "7/8",  DTAPI_MOD_7_8,
               "8/9",  DTAPI_MOD_8_9,  // DVB-S.2 only
               "9/10", DTAPI_MOD_9_10, // DVB-S.2 only
               TS_NULL));
    option("device", 'd', UNSIGNED);
    option("dmb-constellation", 0, Enumeration (
               "4-QAM-NR", DTAPI_MOD_DTMB_QAM4NR,
               "4-QAM",    DTAPI_MOD_DTMB_QAM4,
               "16-QAM",   DTAPI_MOD_DTMB_QAM16,
               "32-QAM",   DTAPI_MOD_DTMB_QAM32,
               "64-QAM",   DTAPI_MOD_DTMB_QAM64,
               TS_NULL));
    option("dmb-fec", 0, Enumeration (
               "0.4", DTAPI_MOD_DTMB_0_4,
               "0.6", DTAPI_MOD_DTMB_0_6,
               "0.8", DTAPI_MOD_DTMB_0_8,
               TS_NULL));
    option("dmb-frame-numbering");
    option("dmb-header", 0, Enumeration (
               "PN420", DTAPI_MOD_DTMB_PN420,
               "PN595", DTAPI_MOD_DTMB_PN595,
               "PN945", DTAPI_MOD_DTMB_PN945,
               TS_NULL));
    option("dmb-interleaver", 0, Enumeration (
               "1", DTAPI_MOD_DTMB_IL_1,
               "2", DTAPI_MOD_DTMB_IL_2,
               TS_NULL));
    option("fef");
    option("fef-interval", 0, INTEGER, 0, 1, 1, 255);
    option("fef-length", 0, INTEGER, 0, 1, 0, 0x003FFFFF);
    option("fef-s1", 0, INTEGER, 0, 1, 2, 7);
    option("fef-s2", 0, INTEGER, 0, 1, 1, 15);
    option("fef-signal", 0, Enumeration (
               "0",      DTAPI_DVBT2_FEF_ZERO,
               "1K",     DTAPI_DVBT2_FEF_1K_OFDM,
               "1K-384", DTAPI_DVBT2_FEF_1K_OFDM_384,
               TS_NULL));
    option("fef-type", 0, INTEGER, 0, 1, 0, 15);
    option("fft-mode", 0, Enumeration (
               "1K",  DTAPI_DVBT2_FFT_1K,
               "2K",  DTAPI_DVBT2_FFT_2K,
               "4K",  DTAPI_DVBT2_FFT_4K,
               "8K",  DTAPI_DVBT2_FFT_8K,
               "16K", DTAPI_DVBT2_FFT_16K,
               "32K", DTAPI_DVBT2_FFT_32K,
               TS_NULL));
    option("frequency", 'f', POSITIVE);
    option("guard-interval", 'g', Enumeration (
               "1/32", DTAPI_MOD_DVBT_G_1_32,
               "1/16", DTAPI_MOD_DVBT_G_1_16,
               "1/8",  DTAPI_MOD_DVBT_G_1_8,
               "1/4",  DTAPI_MOD_DVBT_G_1_4,
               TS_NULL));
    option("indepth-interleave");
    option("input-modulation", 'i');
    option("instant-detach");
    option("inversion");
    option("j83", 0, Enumeration (
               "A", DTAPI_MOD_J83_A,
               "B", DTAPI_MOD_J83_B,
               "C", DTAPI_MOD_J83_C,
               TS_NULL));
    option("level", 'l', INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    option("lnb", 0, Args::STRING);
    option("miso", 0, Enumeration (
               "OFF",  DTAPI_DVBT2_MISO_OFF,
               "1",    DTAPI_DVBT2_MISO_TX1,
               "2",    DTAPI_DVBT2_MISO_TX2,
               "BOTH", DTAPI_DVBT2_MISO_TX1TX2,
               TS_NULL));
    option("modulation", 'm', Enumeration (
               "DVB-S",         DTAPI_MOD_DVBS_QPSK,
               "DVB-S-QPSK",    DTAPI_MOD_DVBS_QPSK,
               "DVB-S-BPSK",    DTAPI_MOD_DVBS_BPSK,
               "DVB-S2",        DTAPI_MOD_DVBS2_QPSK,
               "DVB-S2-QPSK",   DTAPI_MOD_DVBS2_QPSK,
               "DVB-S2-8PSK",   DTAPI_MOD_DVBS2_8PSK,
               "DVB-S2-16APSK", DTAPI_MOD_DVBS2_16APSK,
               "DVB-S2-32APSK", DTAPI_MOD_DVBS2_32APSK,
               "DVB-T",         DTAPI_MOD_DVBT,
               "DVB-T2",        DTAPI_MOD_DVBT2,
               "ATSC-VSB",      DTAPI_MOD_ATSC,
               "4-QAM",         DTAPI_MOD_QAM4,
               "16-QAM",        DTAPI_MOD_QAM16,
               "32-QAM",        DTAPI_MOD_QAM32,
               "64-QAM",        DTAPI_MOD_QAM64,
               "128-QAM",       DTAPI_MOD_QAM128,
               "256-QAM",       DTAPI_MOD_QAM256,
               "ISDB-T",        DTAPI_MOD_ISDBT,
               "DMB-T",         DTAPI_MOD_DMBTH,
               "ADTB-T",        DTAPI_MOD_ADTBT,
               "CMMB",          DTAPI_MOD_CMMB,
               TS_NULL));
    option("mpe-fec");
    option("offset-count", 'o', INTEGER, 0, 1, -3, 3);
    option("papr", 0, Enumeration (
               "NONE", DTAPI_DVBT2_PAPR_NONE,
               "ACE",  DTAPI_DVBT2_PAPR_ACE,
               "TR",   DTAPI_DVBT2_PAPR_TR,
               "BOTH", DTAPI_DVBT2_PAPR_ACE_TR,
               TS_NULL));
    option("pilots", 0);
    option("pilot-pattern", 'p', Enumeration (
               "1", DTAPI_DVBT2_PP_1,
               "2", DTAPI_DVBT2_PP_2,
               "3", DTAPI_DVBT2_PP_3,
               "4", DTAPI_DVBT2_PP_4,
               "5", DTAPI_DVBT2_PP_5,
               "6", DTAPI_DVBT2_PP_6,
               "7", DTAPI_DVBT2_PP_7,
               "8", DTAPI_DVBT2_PP_8,
               TS_NULL));
    option("plp0-code-rate", 0, Enumeration (
               "1/2", DTAPI_DVBT2_COD_1_2,
               "3/5", DTAPI_DVBT2_COD_3_5,
               "2/3", DTAPI_DVBT2_COD_2_3,
               "3/4", DTAPI_DVBT2_COD_3_4,
                "4/5", DTAPI_DVBT2_COD_4_5,
               "5/6", DTAPI_DVBT2_COD_5_6,
               TS_NULL));
    option("plp0-fec-type", 0, Enumeration (
               "16K", DTAPI_DVBT2_LDPC_16K,
               "64K", DTAPI_DVBT2_LDPC_64K,
               TS_NULL));
    option("plp0-group-id", 0, UINT8);
    option("plp0-high-efficiency");
    option("plp0-id", 0, UINT8);
    option("plp0-il-length", 0, UINT8);
    option("plp0-il-type", 0, Enumeration (
               "ONE-TO-ONE", DTAPI_DVBT2_IL_ONETOONE,
               "MULTI",      DTAPI_DVBT2_IL_MULTI,
               TS_NULL));
    option("plp0-in-band");
    option("plp0-issy", 0, Enumeration (
               "NONE",  DTAPI_DVBT2_ISSY_NONE,
               "SHORT", DTAPI_DVBT2_ISSY_SHORT,
               "LONG",  DTAPI_DVBT2_ISSY_LONG,
               TS_NULL));
    option("plp0-modulation", 0, Enumeration (
               "BPSK",    DTAPI_DVBT2_BPSK,
               "QPSK",    DTAPI_DVBT2_QPSK,
               "16-QAM",  DTAPI_DVBT2_QAM16,
               "64-QAM",  DTAPI_DVBT2_QAM64,
               "256-QAM", DTAPI_DVBT2_QAM256,
               TS_NULL));
    option("plp0-null-packet-deletion");
    option("plp0-rotation");
    option("plp0-type", 0, Enumeration (
               "COMMON", DTAPI_DVBT2_PLP_TYPE_COMM,
               "1",      DTAPI_DVBT2_PLP_TYPE_1,
               "2",      DTAPI_DVBT2_PLP_TYPE_2,
               TS_NULL));
    option("qam-b", 'q', Enumeration (
               "I128-J1D", DTAPI_MOD_QAMB_I128_J1D,
               "I64-J2",   DTAPI_MOD_QAMB_I64_J2,
               "I32-J4",   DTAPI_MOD_QAMB_I32_J4,
               "I16-J8",   DTAPI_MOD_QAMB_I16_J8,
               "I8-J16",   DTAPI_MOD_QAMB_I8_J16,
               "I128-J1",  DTAPI_MOD_QAMB_I128_J1,
               "I128-J2",  DTAPI_MOD_QAMB_I128_J2,
               "I128-J3",  DTAPI_MOD_QAMB_I128_J3,
               "I128-J4",  DTAPI_MOD_QAMB_I128_J4,
               "I128-J5",  DTAPI_MOD_QAMB_I128_J5,
               "I128-J6",  DTAPI_MOD_QAMB_I128_J6,
               "I128-J7",  DTAPI_MOD_QAMB_I128_J7,
               "I128-J8",  DTAPI_MOD_QAMB_I128_J8,
               TS_NULL));
    option("s2-gold-code", 0, INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    option("s2-short-fec-frame");
    option("satellite-frequency", 0, POSITIVE);
    option("stuffing", 's');
    option("symbol-rate", 0, POSITIVE);
    option("t2-fpsf", 0, INTEGER, 0, 1, 1, 255);
    option("t2-guard-interval", 0, Enumeration (
               "1/128", DTAPI_DVBT2_GI_1_128,
               "1/32", DTAPI_DVBT2_GI_1_32,
               "1/16", DTAPI_DVBT2_GI_1_16,
               "19/256", DTAPI_DVBT2_GI_19_256,
               "1/8", DTAPI_DVBT2_GI_1_8,
               "19/128", DTAPI_DVBT2_GI_19_128,
               "1/4", DTAPI_DVBT2_GI_1_4,
               TS_NULL));
    option("t2-l1-modulation", 0, Enumeration (
               "BPSK",   DTAPI_DVBT2_BPSK,
               "QPSK",   DTAPI_DVBT2_QPSK,
               "16-QAM", DTAPI_DVBT2_QAM16,
               "64-QAM", DTAPI_DVBT2_QAM64,
               TS_NULL));
    option("t2-network-id", 0, UINT32);
    option("t2-system-id", 0, UINT32);
    option("time-slice");
    option("transmission-mode", 't', Enumeration (
               "2K", DTAPI_MOD_DVBT_2K,
               "4K", DTAPI_MOD_DVBT_4K,
               "8K", DTAPI_MOD_DVBT_8K,
               TS_NULL));
    option("uhf-channel", 'u', INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
    option("vhf-channel", 'v', INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);
    option("vsb", 0, Enumeration (
               "8",  DTAPI_MOD_ATSC_VSB8,
               "16", DTAPI_MOD_ATSC_VSB16,
               TS_NULL));
    option("vsb-taps", 0, INTEGER, 0, 1, 2, 256);

    setHelp("Options:\n"
            "\n"
            "  --204\n"
            "      ASI devices: Send 204-byte packets (188 meaningful bytes plus 16\n"
            "      stuffing bytes for RS coding). By default, send 188-byte packets.\n"
            "\n"
            "  --bandwidth value\n"
            "      DVB-T/H, DVB-T2, ADTB-T and DMB-T/H modulators: indicate bandwidth\n"
            "      in MHz. Must be one of 1.7, 5, 6, 7, 8, 10. The default is 8 MHz.\n"
            "      The bandwidth values 1.7 and 10 MHz are valid for DVB-T2 only.\n"
            "\n"
            "  --bandwidth-extension\n"
            "      DVB-T2 modulators: indicate that the extended carrier mode is used.\n"
            "      By default, use normal carrier mode.\n"
            "\n"
            "  -b value\n"
            "  --bitrate value\n"
            "      Specify output bitrate in bits/second. By default, use the input\n"
            "      device bitrate or, if the input device cannot report bitrate, analyze\n"
            "      some PCR's at the beginning of the input stream to evaluate the\n"
            "      original bitrate of the transport stream.\n"
            "\n"
            "  --cell-id value\n"
            "      DVB-T and DVB-T2 modulators: indicate the cell identifier to set in the\n"
            "      transmition parameters signaling (TPS). Disabled by default with DVB-T.\n"
            "      Default value is 0 with DVB-T2.\n"
            "\n"
            "  -c value\n"
            "  --channel value\n"
            "      Channel index on the output Dektec device. By default, use the\n"
            "      first output channel on the device.\n"
            "\n"
            "  --cmmb-area-id value\n"
            "      CMMB modulators: indicate the area id. The valid range is 0 to 127.\n"
            "      The default is zero.\n"
            "\n"
            "  --cmmb-bandwidth value\n"
            "      CMMB modulators: indicate bandwidth in MHz. Must be one of 2 or 8.\n"
            "      The default is 8 MHz.\n"
            "\n"
            "  --cmmb-pid value\n"
            "      CMMB modulators: indicate the PID of the CMMB stream in the transport\n"
            "      stream. This is a required parameter for CMMB modulation.\n"
            "\n"
            "  --cmmb-transmitter-id value\n"
            "      CMMB modulators: indicate the transmitter id. The valid range is 0 to\n"
            "      127. The default is zero.\n"
            "\n"
            "  --constellation value\n"
            "      DVB-T modulators: indicate the constellation type. Must be one of\n"
            "      QPSK, 16-QAM, 64-QAM. The default is 64-QAM.\n"
            "\n"
            "  -r rate\n"
            "  --convolutional-rate rate\n"
            "      For modulators devices only: specify the convolutional rate.\n"
            "      The specified value depends on the modulation type.\n"
            "      DVB-S: 1/2, 2/3, 3/4, 4/5, 5/6, 6/7, 7/8.\n"
            "      DVB-S2: 1/2, 1/3, 1/4, 2/3, 2/5, 3/4, 3/5, 4/5, 5/6, 6/7, 7/8, 8/9, 9/10.\n"
            "      DVB-T: 1/2, 2/3, 3/4, 5/6, 7/8.\n"
            "      The default is 3/4.\n"
            "\n"
            "  -d value\n"
            "  --device value\n"
            "      Device index, from 0 to N-1 (with N being the number of Dektec devices\n"
            "      in the system). Use the command \"tsdektec -a [-v]\" to have a\n"
            "      complete list of devices in the system. By default, use the first\n"
            "      output Dektec device.\n"
            "\n"
            "  --dmb-constellation value\n"
            "      DMB-T/H, ADTB-T modulators: indicate the constellation type. Must be one\n"
            "      of: 4-QAM-NR, 4-QAM, 16-QAM, 32-QAM, 64-QAM. The default is 64-QAM.\n"
            "      4-QAM-NR and 32-QAM can be used only with --dmb-fec 0.8.\n"
            "\n"
            "  --dmb-fec value\n"
            "      DMB-T/H, ADTB-T modulators: indicate the FEC code rate. Must be one of\n"
            "      0.4, 0.6, 0.8. The default is 0.8.\n"
            "\n"
            "  --dmb-frame-numbering\n"
            "      DMB-T/H, ADTB-T modulators: indicate to use frame numbering. The default\n"
            "      is to use no frame numbering.\n"
            "\n"
            "  --dmb-header value\n"
            "      DMB-T/H, ADTB-T modulators: indicate the FEC frame header mode. Must be\n"
            "      one of PN420, PN595 (ADTB-T only) or PN945. The default is PN945.\n"
            "\n"
            "  --dmb-interleaver value\n"
            "      DMB-T/H, ADTB-T modulators: indicate the interleaver mode. Must be one\n"
            "      1 (B=54, M=240) or 2 (B=54, M=720). The default is 1.\n"
            "\n"
            "  --fef\n"
            "      DVB-T2 modulators: enable insertion of FEF's (Future Extension Frames).\n"
            "      Not enabled by default.\n"
            "\n"
            "  --fef-interval value\n"
            "      DVB-T2 modulators: indicate the number of T2 frames between two FEF\n"
            "      parts. The valid range is 1 to 255 and --t2-fpsf shall be divisible by\n"
            "      --fef-interval. The default is 1.\n"
            "\n"
            "  --fef-length value\n"
            "      DVB-T2 modulators: indicate the length of a FEF-part in number of T-units\n"
            "      (= samples). The valid range is 0 to 0x3FFFFF. The default is 1.\n"
            "\n"
            "  --fef-s1 value\n"
            "      DVB-T2 modulators: indicate the S1-field value in the P1 signalling data.\n"
            "      Valid values: 2, 3, 4, 5, 6 and 7. The default is 2.\n"
            "\n"
            "  --fef-s2 value\n"
            "      DVB-T2 modulators: indicate the S2-field value in the P1 signalling data.\n"
            "      Valid values: 1, 3, 5, 7, 9, 11, 13 and 15. The default is 1.\n"
            "\n"
            "  --fef-signal value\n"
            "      DVB-T2 modulators: indicate the type of signal generated during the FEF\n"
            "      period. Must be one of \"0\" (zero I/Q samples during FEF), \"1K\" (1K\n"
            "      OFDM symbols with 852 active carriers containing BPSK symbols, same PRBS\n"
            "      as the T2 dummy cells, not reset between symbols) or \"1K-384\" (1K OFDM\n"
            "      symbols with 384 active carriers containing BPSK symbols).\n"
            "      The default is 0.\n"
            "\n"
            "  --fef-type value\n"
            "      DVB-T2 modulators: indicate the FEF type. The valid range is 0 ... 15.\n"
            "      The default is 0.\n"
            "\n"
            "  --fft-mode value\n"
            "      DVB-T2 modulators: indicate the FFT mode. Must be one of 1K, 2K, 4K, 8K,\n"
            "      16K or 32K. The default is 32K.\n"
            "\n"
            "  -f value\n"
            "  --frequency value\n"
            "      All modulator devices: indicate the frequency, in Hz, of the output\n"
            "      carrier. There is no default. For OFDM modulators, the options\n"
            "      --uhf-channel or --vhf-channel and --offset-count may be used instead.\n"
            "      For DVB-S/S2 modulators, the specified frequency is the \"intermediate\"\n"
            "      frequency. For convenience, the option --satellite-frequency can be used\n"
            "      instead of --frequency when the intermediate frequency is unknown.\n"
            "      For DTA-107 modulators, the valid range is 950 MHz to 2150 MHz.\n"
            "      For DTA-110 and 110T modulators, the valid range is 400 MHz to 862 MHz.\n"
            "      For DTA-115 modulators, the valid range is 47 MHz to 862 MHz.\n"
            "\n"
            "  -g value\n"
            "  --guard-interval value\n"
            "      DVB-T modulators: indicate the guard interval. Must be one\n"
            "      of: 1/32, 1/16, 1/8, 1/4. The default is 1/32.\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  --indepth-interleave\n"
            "      DVB-T modulators: indicate to use in-depth interleave.\n"
            "      The default is native interleave.\n"
            "\n"
            "  -i\n"
            "  --input-modulation\n"
            "      All modulators devices: try to guess modulation parameters from input\n"
            "      stream. If the input plugin is \"dvb\", use its modulation parameters.\n"
#if defined (__windows)
            "      Warning: not always accurate on Windows systems.\n"
#endif
            "      Otherwise, if the specified modulation is DVB-T, try to guess\n"
            "      some modulation parameters from the bitrate.\n"
            "\n"
            "  --instant-detach\n"
            "      At end of stream, perform an \"instant detach\" of the output channel.\n"
            "      The default is to wait until all bytes are sent. The default is fine\n"
            "      for ASI devices. With modulators, the \"wait until sent\" mode may\n"
            "      hang at end of stream and --instant-detach avoids this.\n"
            "\n"
            "  --inversion\n"
            "      All modulators devices: enable spectral inversion.\n"
            "\n"
            "  --j83 annex\n"
            "      QAM modulators: indicate the ITU-T J.83 annex to use. Must be one of\n"
            "      \"A\" (DVB-C), \"B\" (American QAM) or \"C\" (Japanese QAM). The default is A.\n"
            "\n"
            "  -l value\n"
            "  --level value\n"
            "      Modulators: indicate the output level in units of 0.1 dBm (e.g.\n"
            "      --level -30 means -3 dBm). Not supported by all devices.\n"
            "      For DTA-107 modulators, the valid range is -47.0 to -27.0 dBm.\n"
            "      For DTA-115, QAM, the valid range is -35.0 to 0.0 dBm.\n"
            "      For DTA-115, OFDM, ISDB-T, the valid range is -38.0 to -3.0 dBm.\n"
            "\n"
            "  --lnb string\n"
            "      DVB-S/S2 modulators: description of the LNB which is used to convert the\n"
            "      --satellite-frequency into an intermediate frequency. This option is\n"
            "      useless when --satellite-frequency is not specified. The format of the\n"
            "      string is \"low_freq[,high_freq[,switch_freq]]\" where all frequencies\n"
            "      are in MHz. The characteristics of the default universal LNB are\n"
            "      low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.\n"
            "\n"
            "  --miso value\n"
            "      DVB-T2 modulators: indicate the MISO mode. Must be one of OFF, 1, 2 or\n"
            "      BOTH. The default si OFF. This mode can be used to simulate antenna 1,\n"
            "      antenna 2 or the average of antenna 1 and antenna 2 to simulate reception\n"
            "      halfway between the antennas.\n"
            "\n"
            "  -m value\n"
            "  --modulation value\n"
            "      For modulators, indicate the modulation type. Must be one of: \n"
            "      4-QAM, 16-QAM, 32-QAM, 64-QAM, 128-QAM, 256-QAM, ADTB-T, ATSC-VSB, CMMB,\n"
            "      DMB-T, DVB-S, DVB-S-QPSK (same as DVB-S), DVB-S-BPSK, DVB-S2, DVB-S2-QPSK\n"
            "      (same as DVB-S2), DVB-S2-8PSK, DVB-S2-16APSK, DVB-S2-32APSK, DVB-T, \n"
            "      DVB-T2, ISDB-T. For DVB-H, specify DVB-T. For DMB-H, specify DMB-T.\n"
            "      The supported modulation types depend on the device model.\n"
            "      The default modulation type is:\n"
            "        DTA-107:   DVB-S-QPSK\n"
            "        DTA-107S2: DVB-S2-QPSK\n"
            "        DTA-110:   64-QAM\n"
            "        DTA-110T:  DVB-T\n"
            "        DTA-115:   DVB-T\n"
            "\n"
            "  --mpe-fec\n"
            "      DVB-T/H modulators: indicate that at least one elementary stream uses\n"
            "      MPE-FEC (DVB-H signalling).\n"
            "\n"
            "  -o value\n"
            "  --offset-count value\n"
            "      UHF and VHF modulators: indicate the number of offsets from the UHF or\n"
            "      VHF channel. The default is zero. See options --uhf-channel and\n"
            "      --vhf-channel.\n"
            "\n"
            "  --papr value\n"
            "      DVB-T2 modulators: indicate the Peak to Average Power Reduction method.\n"
            "      Must be one of NONE, ACE (Active Constellation Extension), TR (power\n"
            "      reduction with reserved carriers) or BOTH (both ACE and TS). The default\n"
            "      is NONE.\n"
            "\n"
            "  --pilots\n"
            "      DVB-S2 and ADTB-T modulators: enable pilots (default: no pilot).\n"
            "\n"
            "  -p value\n"
            "  --pilot-pattern value\n"
            "      DVB-T2 modulators: indicate the pilot pattern to use, a value in the\n"
            "      range 1 to 8. The default is 7.\n"
            "\n"
            "  --plp0-code-rate value\n"
            "      DVB-T2 modulators: indicate the convolutional coding rate used by the\n"
            "      PLP #0. Must be one of 1/2, 3/5, 2/3, 3/4, 4/5, 5/6. The default is 2/3.\n"
            "\n"
            "  --plp0-fec-type value\n"
            "      DVB-T2 modulators: indicate the FEC type used by the PLP #0. Must be one\n"
            "      of 16K, 64K. The default is 64K LPDC.\n"
            "\n"
            "  --plp0-group-id value\n"
            "      DVB-T2 modulators: indicate the PLP group with which the PLP #0 is\n"
            "      associated. The valid range is 0 to 255. The default is 0.\n"
            "\n"
            "  --plp0-high-efficiency\n"
            "      DVB-T2 modulators: indicate that the PLP #0 uses High Efficiency Mode\n"
            "      (HEM). Otherwise Normal Mode (NM) is used.\n"
            "\n"
            "  --plp0-id value\n"
            "      DVB-T2 modulators: indicate the unique identification of the PLP #0\n"
            "      within the T2 system. The valid range is 0 to 255. The default is 0.\n"
            "\n"
            "  --plp0-il-length value\n"
            "      DVB-T2 modulators: indicate the time interleaving length for PLP #0.\n"
            "      If --plp0-il-type is set to \"ONE-TO-ONE\" (the default), this parameter\n"
            "      specifies the number of TI-blocks per interleaving frame.\n"
            "      If --plp0-il-type is set to \"MULTI\", this parameter specifies the\n"
            "      number of T2 frames to which each interleaving frame is mapped.\n"
            "      The valid range is 0 to 255. The default is 3.\n"
            "\n"
            "  --plp0-il-type value\n"
            "      DVB-T2 modulators: indicate the type of interleaving used by the PLP #0.\n"
            "      Must be one of \"ONE-TO-ONE\" (one interleaving frame corresponds to one\n"
            "      T2 frame) or \"MULTI\" (one interleaving frame is carried in multiple T2\n"
            "      frames). The default is ONE-TO-ONE.\n"
            "\n"
            "  --plp0-in-band\n"
            "      DVB-T2 modulators: indicate that the in-band flag is set and in-band\n"
            "      signalling information is inserted in PLP #0.\n"
            "\n"
            "  --plp0-issy value\n"
            "      DVB-T2 modulators: type of ISSY field to compute and inserte in PLP #0.\n"
            "      Must be one of \"NONE\", \"SHORT\", \"LONG\". The default is NONE.\n"
            "\n"
            "  --plp0-modulation value\n"
            "      DVB-T2 modulators: indicate the modulation used by PLP #0. Must be one of\n"
            "      BPSK, QPSK, 16-QAM, 64-QAM, 256-QAM. The default is 256-QAM.\n"
            "\n"
            "  --plp0-null-packet-deletion\n"
            "      DVB-T2 modulators: indicate that null-packet deletion is active in\n"
            "      PLP #0. Otherwise it is not active.\n"
            "\n"
            "  --plp0-rotation\n"
            "      DVB-T2 modulators: indicate that constellation rotation is used for\n"
            "      PLP #0. Otherwise not.\n"
            "\n"
            "  --plp0-type value\n"
            "      DVB-T2 modulators: indicate the PLP type for PLP #0. Must be one of\n"
            "      \"COMMON\", \"1\", \"2\". The default is COMMON.\n"
            "\n"
            "  -q value\n"
            "  --qam-b value\n"
            "      QAM modulators: with --j83 B, indicate the QAM-B interleaver mode.\n"
            "      Must be one of: I128-J1D, I64-J2, I32-J4, I16-J8, I8-J16, I128-J1,\n"
            "      I128-J2, I128-J3, I128-J4, I128-J5, I128-J6, I128-J7, I128-J8.\n"
            "      The default is I128-J1D.\n"
            "\n"
            "  --s2-gold-code value\n"
            "      DVB-S2 modulators: indicate the physical layer scrambling initialization\n"
            "      sequence, aka \"gold code\".\n"
            "\n"
            "  --s2-short-fec-frame\n"
            "      DVB-S2 modulators: use short FEC frames, 16 200 bits (default: long FEC\n"
            "      frames, 64 800 bits).\n"
            "\n"
            "  --satellite-frequency value\n"
            "      DVB-S/S2 modulators: indicate the target satellite frequency, in Hz, of\n"
            "      the output carrier. The actual frequency at the output of the modulator\n"
            "      is the \"intermediate\" frequency which is computed based on the\n"
            "      characteristics of the LNB (see option --lnb). This option is useful\n"
            "      when the satellite frequency is better known than the intermediate\n"
            "      frequency. The options --frequency and --satellite-frequency are mutually\n"
            "      exclusive.\n"
            "\n"
            "  -s\n"
            "  --stuffing\n"
            "      Automatically generate stuffing packets if we fail to provide\n"
            "      packets fast enough.\n"
            "\n"
            "  --symbol-rate value\n"
            "      DVB-C/S/S2 modulators: Specify the symbol rate in symbols/second.\n"
            "      By default, the symbol rate is implicitely computed from the convolutional\n"
            "      rate, the modulation type and the bitrate. But when --symbol-rate is\n"
            "      specified, the input bitrate is ignored and the output bitrate is forced\n"
            "      to the value resulting from the combination of the specified symbol rate,\n"
            "      convolutional rate and modulation type.\n"
            "      The options --symbol-rate and --bitrate are mutually exclusive.\n"
            "\n"
            "  --t2-fpsf value\n"
            "      DVB-T2 modulators: indicate the number of T2 frames per super-frame.\n"
            "      Must be in the range 1 to 255. The default is 2.\n"
            "\n"
            "  --t2-guard-interval value\n"
            "      DVB-T2 modulators: indicates the guard interval. Must be one of:\n"
            "      1/128, 1/32, 1/16, 19/256, 1/8, 19/128, 1/4. The default is 1/128.\n"
            "\n"
            "  --t2-l1-modulation value\n"
            "      DVB-T2 modulators: indicate the modulation type used for the L1-post\n"
            "      signalling block. Must be one of BPSK, QPSK, 16-QAM, 64-QAM. The default\n"
            "      is 16-QAM.\n"
            "\n"
            "  --t2-network-id value\n"
            "      DVB-T2 modulators: indicate the DVB-T2 network identification.\n"
            "      The default is 0.\n"
            "\n"
            "  --t2-system-id value\n"
            "      DVB-T2 modulators: indicate the DVB-T2 system identification.\n"
            "      The default is 0.\n"
            "\n"
            "  --time-slice\n"
            "      DVB-T/H modulators: indicate that at least one elementary stream uses\n"
            "      time slicing (DVB-H signalling).\n"
            "\n"
            "  -t value\n"
            "  --transmission-mode value\n"
            "      DVB-T modulators: indicate the transmission mode. Must be one of\n"
            "      2K, 4K or 8K. The default is 8K.\n"
            "\n"
            "  -u value\n"
            "  --uhf-channel value\n"
            "      UHF modulators: indicate the UHF channel number of the output carrier.\n"
            "      Can be used in replacement to --frequency. Can be combined with an\n"
            "      --offset-count option. The resulting frequency is\n"
            "      306 MHz + (uhf-channel * 8 MHz) + (offset-count * 166.6 kHz).\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n"
            "\n"
            "  -v value\n"
            "  --vhf-channel value\n"
            "      VHF modulators: indicate the VHF channel number of the output carrier.\n"
            "      Can be used in replacement to --frequency. Can be combined with an\n"
            "      --offset-count option. The resulting frequency is\n"
            "      142.5 MHz + (vhf-channel * 7 MHz) + (offset-count * 166.6 kHz).\n"
            "\n"
            "  --vsb value\n"
            "      ATSC modulators: indicate the VSB constellation. Must be one of\n"
            "      8 (19,392,658 Mb/s) or 16 (38,785,317 Mb/s). The default is 8.\n"
            "\n"
            "  --vsb-taps value\n"
            "      ATSC modulators: indicate the number of taps of each phase of the\n"
            "      root-raised cosine filter that is used to shape the spectrum of the\n"
            "      output signal. The number of taps can have any value between 2 and 256\n"
            "      (the implementation is optimized for powers of 2). Specifying more taps\n"
            "      improves the spectrum, but increases processor overhead. The recommend\n"
            "      (and default) number of taps is 64 taps. If insufficient CPU power is\n"
            "      available, 32 taps produces acceptable results, too.\n");
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::start()
{
    if (_guts->is_started) {
        tsp->error("already started");
        return false;
    }

    // Get command line arguments
    _guts->dev_index = intValue<int>("device", -1);
    _guts->chan_index = intValue<int>("channel", -1);
    _guts->opt_bitrate = intValue<BitRate>("bitrate", 0);
    _guts->detach_mode = present("instant-detach") ? DTAPI_INSTANT_DETACH : DTAPI_WAIT_UNTIL_SENT;
    _guts->mute_on_stop = false;

    // Get initial bitrate
    _guts->cur_bitrate = _guts->opt_bitrate != 0 ? _guts->opt_bitrate : tsp->bitrate();

    // Locate the device
    if (!_guts->device.getDevice(_guts->dev_index, _guts->chan_index, false, *tsp)) {
        return false;
    }

    // Open the device
    Dtapi::DTAPI_RESULT status = _guts->dtdev.AttachToSerial(_guts->device.desc.m_Serial);
    if (status != DTAPI_OK) {
        tsp->error(Format("error attaching output Dektec device %d (%s): ", _guts->dev_index, _guts->device.model.c_str()) + DektecStrError(status));
        return false;
    }

    // Open the channel
    status = _guts->chan.AttachToPort(&_guts->dtdev, _guts->device.output[_guts->chan_index].m_Port);
    if (status != DTAPI_OK) {
        tsp->error(Format("error attaching output channel %d of Dektec device %d (%s): ", _guts->chan_index, _guts->dev_index, _guts->device.model.c_str()) + DektecStrError(status));
        _guts->dtdev.Detach();
        return false;
    }

    // Get the Vital Product Data (VPD)
    const DektecVPD vpd(_guts->dtdev);

    // Check if the device is a modulator.
    const bool is_modulator = (_guts->device.output[_guts->chan_index].m_Flags & DTAPI_CAP_MOD) != 0;
    _guts->mute_on_stop = false;

    // Determine channel capabilities.
    Dtapi::DtCaps dt_flags = _guts->device.output[_guts->chan_index].m_Flags;

    // Set default modulation for multi-standard modulators.
    // Also adjust device capabilities since m_Flags field is not
    // always set (DTAPI bug? maybe fixed since first found)
    int modulation_type = -1;
    switch (_guts->device.desc.m_TypeNumber) {
        case 107: {
            // DTA-107 or DTA-107S2: QPSK modulator
            if (::strcmp(vpd.pn, "DTA-107S2") == 0) {
                modulation_type = DTAPI_MOD_DVBS2_QPSK;
                dt_flags |= DTAPI_CAP_TX_DVBS | DTAPI_CAP_TX_DVBS2;
            }
            else {
                modulation_type = DTAPI_MOD_DVBS_QPSK;
                dt_flags |= DTAPI_CAP_TX_DVBS;
            }
            break;
        }
        case 110: {
            // DTA-110 or DTA-110T: QAM or OFDM modulator
            if (::strcmp(vpd.pn, "DTA-110T") == 0) {
                // Part number (PN) is DTA-110T
                modulation_type = DTAPI_MOD_DVBT;
                dt_flags |= DTAPI_CAP_TX_DVBT;
            }
            else {
                modulation_type = DTAPI_MOD_QAM64;
                dt_flags |= DTAPI_CAP_TX_QAMA;
            }
            break;
        }
        case 115: {
            // DTA-115, multi-standard, depend on embedded licences.
            // DVB-T always supported (?) and is default.
            modulation_type = DTAPI_MOD_DVBT;
            _guts->mute_on_stop = true;
            break;
        }
        default:
            // Unknown device.
            modulation_type = -1;
            break;
    }

    // Reset output channel
    status = _guts->chan.Reset(DTAPI_FULL_RESET);
    if (status != DTAPI_OK) {
        return startError("output device reset error", status);
    }

    // Get current FIFO size, for information only, ignore errors
    int fifo_size = 0;
    int max_fifo_size = 0;
    status = _guts->chan.GetFifoSize(fifo_size);
    if (status == DTAPI_OK) {
        status = _guts->chan.GetMaxFifoSize(max_fifo_size);
    }
    if (status == DTAPI_OK) {
        tsp->verbose("output fifo size: " + Decimal(fifo_size) + " bytes, max: " + Decimal(max_fifo_size) + " bytes");
    }

    // Set 188/204-byte output packet format and stuffing
    status = _guts->chan.SetTxMode(present("204") ? DTAPI_TXMODE_ADD16 : DTAPI_TXMODE_188, present("stuffing") ? 1 : 0);
    if (status != DTAPI_OK) {
        return startError("output device SetTxMode error", status);
    }

    // Set modulation parameters for modulators
    if (is_modulator && !setModulation(modulation_type)) {
        return false;
    }

    // Set output level.
    if (present("level")) {
        status = _guts->chan.SetOutputLevel(intValue<int>("level"));
        if (status != DTAPI_OK) {
            // In case of error, report it but do not fail.
            // This feature is not supported on all modulators and
            // it seems severe to fail if unsupported.
            tsp->error("set modulator output level: " + DektecStrError(status));
        }
    }

    // Set output bitrate
    status = _guts->chan.SetTsRateBps(int(_guts->cur_bitrate));
    if (status != DTAPI_OK) {
        return startError("output device set bitrate error", status);
    }

    // Start the transmission on the output device.
    // With ASI device, we can start transmission right now.
    // With modulator devices, we need to load the FIFO first.
    _guts->starting = is_modulator;
    status = _guts->chan.SetTxControl(_guts->starting ? DTAPI_TXCTRL_HOLD : DTAPI_TXCTRL_SEND);
    if (status != DTAPI_OK) {
        return startError("output device start send error", status);
    }

    tsp->verbose("initial output bitrate: " + Decimal(_guts->cur_bitrate) + " b/s");
    _guts->is_started = true;
    return true;
}


//----------------------------------------------------------------------------
// Output start error method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::startError(const std::string& message, unsigned int status)
{
    if (status == DTAPI_OK) {
        tsp->error(message);
    }
    else {
        tsp->error(message + ": " + DektecStrError(status));
    }
    _guts->chan.Detach(DTAPI_INSTANT_DETACH);
    _guts->dtdev.Detach();
    return false;
}


//----------------------------------------------------------------------------
// Update, when possible, the _opt_bitrate and _cur_bitrate fields based on a
// user-specified symbol rate (and other modulation parameters). Return false
// and close channel on error. Return true if the bitrate was successfully
// computed.
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setBitrate(int symbol_rate, int dt_modulation, int param0, int param1, int param2)
{
    int bitrate = -1;
    Dtapi::DTAPI_RESULT status = Dtapi::DtapiModPars2TsRate(bitrate, dt_modulation, param0, param1, param2, symbol_rate);
    if (status != DTAPI_OK) {
        return startError("Error computing bitrate from symbol rate", status);
    }
    else {
        _guts->opt_bitrate = _guts->cur_bitrate = BitRate(bitrate);
        return true;
    }
}


//----------------------------------------------------------------------------
// Set modulation parameters (modulators only).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setModulation (int& modulation_type)
{
    // Get input plugin modulation parameters if required
    const bool use_input_modulation = present("input-modulation");
    const ObjectPtr input_params(use_input_modulation ? Object::RetrieveFromRepository("tsp.dvb.params") : 0);

    // Various views of the input modulation parameters (at most one is non-zero)
    const TunerParameters*     input_dvb  = dynamic_cast <const TunerParameters*>(input_params.pointer());
    const TunerParametersDVBS* input_dvbs = dynamic_cast <const TunerParametersDVBS*>(input_dvb);
    const TunerParametersDVBC* input_dvbc = dynamic_cast <const TunerParametersDVBC*>(input_dvb);
    const TunerParametersDVBT* input_dvbt = dynamic_cast <const TunerParametersDVBT*>(input_dvb);
    const TunerParametersATSC* input_atsc = dynamic_cast <const TunerParametersATSC*>(input_dvb);

    // Adjust default modulation type from input plugin
    if (input_dvb != 0) {
        tsp->debug("found input modulator parameters: " + TunerTypeEnum.name(input_dvb->tunerType()) + " " + input_dvb->toPluginOptions());
        if (input_dvbs != 0) {
            if (input_dvbs->delivery_system == DS_DVB_S) {
                modulation_type = DTAPI_MOD_DVBS_QPSK;
            }
            else if (input_dvbs->delivery_system == DS_DVB_S2 && input_dvbs->modulation == QPSK) {
                modulation_type = DTAPI_MOD_DVBS2_QPSK;
            }
            else if (input_dvbs->delivery_system == DS_DVB_S2 && input_dvbs->modulation == PSK_8) {
                modulation_type = DTAPI_MOD_DVBS2_8PSK;
            }
        }
        else if (input_dvbc != 0) {
            switch (input_dvbc->modulation) {
                case QAM_16:  modulation_type = DTAPI_MOD_QAM16;  break;
                case QAM_32:  modulation_type = DTAPI_MOD_QAM32;  break;
                case QAM_64:  modulation_type = DTAPI_MOD_QAM64;  break;
                case QAM_128: modulation_type = DTAPI_MOD_QAM128; break;
                case QAM_256: modulation_type = DTAPI_MOD_QAM256; break;
                default: break;
            }
        }
        else if (input_dvbt != 0) {
            modulation_type = DTAPI_MOD_DVBT;
        }
        else if (input_atsc != 0) {
            modulation_type = DTAPI_MOD_ATSC;
        }
    }

    // Get user-specified modulation
    modulation_type = intValue<int>("modulation", modulation_type);
    if (modulation_type < 0) {
        return startError("unspecified modulation type for " + _guts->device.model, DTAPI_OK);
    }

    // Get user-specified symbol rate, used only with DVB-S/S2/C.
    int symbol_rate = intValue<int>("symbol-rate", -1);
    if (present("bitrate") && present("symbol-rate")) {
        return startError("options --symbol-rate and --bitrate are mutually exclusive", DTAPI_OK);
    }

    // Get LNB description, in case --satellite-frequency is used
    LNB lnb; // Universal LNB by default
    if (present("lnb")) {
        const std::string s(value("lnb"));
        LNB l(s);
        if (!l.isValid()) {
            return startError("invalid LNB description " + s, DTAPI_OK);
        }
        else {
            lnb = l;
        }
    }

    // Compute carrier frequency
    uint64_t frequency = 0;
    if (present("frequency") + present("satellite-frequency") + present("uhf-channel") + present("vhf-channel") > 1) {
        return startError("options --frequency, --satellite-frequency, --uhf-channel, --vhf-channel are mutually exclusive", DTAPI_OK);
    }
    if (present("uhf-channel")) {
        frequency = UHF::Frequency(intValue<int>("uhf-channel", 0), intValue<int>("offset-count", 0));
    }
    else if (present("vhf-channel")) {
        frequency = VHF::Frequency(intValue<int>("vhf-channel", 0), intValue<int>("offset-count", 0));
    }
    else if (present("satellite-frequency")) {
        uint64_t sat_frequency = intValue<uint64_t> ("satellite-frequency", 0);
        if (sat_frequency > 0) {
            frequency = lnb.intermediateFrequency(sat_frequency);
        }
    }
    else if (present ("frequency")) {
        frequency = intValue<uint64_t>("frequency", 0);
    }
    else if (input_dvbs != 0) {
        frequency = input_dvbs->frequency;
    }
    else if (input_dvbt != 0) {
        frequency = input_dvbt->frequency;
    }
    else if (input_dvbc != 0) {
        frequency = input_dvbc->frequency;
    }
    else if (input_atsc != 0) {
        frequency = input_atsc->frequency;
    }
    if (frequency == 0) {
        return startError ("unspecified frequency (required for modulator devices)", DTAPI_OK);
    }

    // Set modulation parameters
    Dtapi::DTAPI_RESULT status = DTAPI_OK;
    switch (modulation_type) {

        case DTAPI_MOD_DVBS_QPSK:
        case DTAPI_MOD_DVBS_BPSK: {
            // Various types of DVB-S
            int fec = DTAPI_MOD_3_4;
            if (input_dvbs != 0) {
                symbol_rate = input_dvbs->symbol_rate;
                switch (input_dvbs->inner_fec) {
                    case FEC_1_2: fec = DTAPI_MOD_1_2; break;
                    case FEC_2_3: fec = DTAPI_MOD_2_3; break;
                    case FEC_3_4: fec = DTAPI_MOD_3_4; break;
                    case FEC_4_5: fec = DTAPI_MOD_4_5; break;
                    case FEC_5_6: fec = DTAPI_MOD_5_6; break;
                    case FEC_6_7: fec = DTAPI_MOD_6_7; break;
                    case FEC_7_8: fec = DTAPI_MOD_7_8; break;
                    default: break;
                }
            }
            fec = intValue<int> ("convolutional-rate", fec);
            tsp->verbose ("using DVB-S FEC " + DektecFEC.name (fec));
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate > 0 && !setBitrate (symbol_rate, modulation_type, fec, 0, 0)) {
                return false;
            }
            status = _guts->chan.SetModControl (modulation_type, fec, 0, 0);
            break;
        }

        case DTAPI_MOD_DVBS2_QPSK:
        case DTAPI_MOD_DVBS2_8PSK:
        case DTAPI_MOD_DVBS2_16APSK:
        case DTAPI_MOD_DVBS2_32APSK: {
            // Various types of DVB-S2
            int fec = DTAPI_MOD_3_4;
            int pilots = present("pilots") ? DTAPI_MOD_S2_PILOTS : DTAPI_MOD_S2_NOPILOTS;
            if (input_dvbs != 0) {
                symbol_rate = input_dvbs->symbol_rate;
                switch (input_dvbs->pilots) {
                    case PILOT_ON:  pilots = DTAPI_MOD_S2_PILOTS; break;
                    case PILOT_OFF: pilots = DTAPI_MOD_S2_NOPILOTS; break;
                    case PILOT_AUTO: break;
                    default: break;
                }
                switch (input_dvbs->inner_fec) {
                    case FEC_1_2: fec = DTAPI_MOD_1_2; break;
                    case FEC_1_3: fec = DTAPI_MOD_1_3; break;
                    case FEC_1_4: fec = DTAPI_MOD_1_4; break;
                    case FEC_2_3: fec = DTAPI_MOD_2_3; break;
                    case FEC_2_5: fec = DTAPI_MOD_2_5; break;
                    case FEC_3_4: fec = DTAPI_MOD_3_4; break;
                    case FEC_3_5: fec = DTAPI_MOD_3_5; break;
                    case FEC_4_5: fec = DTAPI_MOD_4_5; break;
                    case FEC_5_6: fec = DTAPI_MOD_5_6; break;
                    case FEC_6_7: fec = DTAPI_MOD_6_7; break;
                    case FEC_7_8: fec = DTAPI_MOD_7_8; break;
                    case FEC_8_9: fec = DTAPI_MOD_8_9; break;
                    case FEC_9_10: fec = DTAPI_MOD_9_10; break;
                    default: break;
                }
            }
            fec = intValue<int>("convolutional-rate", fec);
            const int fec_frame = present("s2-short-fec-frame") ? DTAPI_MOD_S2_SHORTFRM : DTAPI_MOD_S2_LONGFRM;
            const int gold_code = intValue<int>("s2-gold-code", 0);
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate > 0 && !setBitrate(symbol_rate, modulation_type, fec, pilots | fec_frame, gold_code)) {
                return false;
            }
            status = _guts->chan.SetModControl(modulation_type, fec, pilots | fec_frame, gold_code);
            break;
        }

        case DTAPI_MOD_QAM4:
        case DTAPI_MOD_QAM16:
        case DTAPI_MOD_QAM32:
        case DTAPI_MOD_QAM64:
        case DTAPI_MOD_QAM128:
        case DTAPI_MOD_QAM256: {
            // Various types of DVB-C
            const int j83 = intValue<int> ("j83", DTAPI_MOD_J83_A);
            const int qam_b = j83 != DTAPI_MOD_J83_B ? 0 : intValue<int> ("qam-b", DTAPI_MOD_QAMB_I128_J1D);
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate > 0 && !setBitrate(symbol_rate, modulation_type, j83, qam_b, 0)) {
                return false;
            }
            status = _guts->chan.SetModControl(modulation_type, j83, qam_b, 0);
            break;
        }

        case DTAPI_MOD_DVBT: {
            // DVB-T
            int fec = DTAPI_MOD_3_4;
            int bw = DTAPI_MOD_DVBT_8MHZ;
            int constel = DTAPI_MOD_DVBT_QAM64;
            int guard = DTAPI_MOD_DVBT_G_1_32;
            int tr_mode = DTAPI_MOD_DVBT_8K;
            TunerParametersBitrateDiffDVBT params;
            if (use_input_modulation && input_dvbt == 0 && _guts->cur_bitrate > 0) {
                // --input-modulation is specified but input plugin is not a DVB-T tuner,
                // use input bitrate to determine modulation parameters.
                TunerParametersBitrateDiffDVBTList params_list;
                TunerParametersBitrateDiffDVBT::EvaluateToBitrate(params_list, _guts->cur_bitrate);
                if (!params_list.empty()) {
                    params = params_list.front();
                    input_dvbt = &params;
                }
            }
            if (input_dvbt != 0) {
                switch (input_dvbt->fec_hp) {
                    case FEC_1_2: fec = DTAPI_MOD_1_2; break;
                    case FEC_2_3: fec = DTAPI_MOD_2_3; break;
                    case FEC_3_4: fec = DTAPI_MOD_3_4; break;
                    case FEC_5_6: fec = DTAPI_MOD_5_6; break;
                    case FEC_7_8: fec = DTAPI_MOD_7_8; break;
                    default: break;
                }
                switch (input_dvbt->bandwidth) {
                    case BW_8_MHZ: bw = DTAPI_MOD_DVBT_8MHZ; break;
                    case BW_7_MHZ: bw = DTAPI_MOD_DVBT_7MHZ; break;
                    case BW_6_MHZ: bw = DTAPI_MOD_DVBT_6MHZ; break;
                    case BW_5_MHZ: bw = DTAPI_MOD_DVBT_5MHZ; break;
                    default: break;
                }
                switch (input_dvbt->modulation) {
                    case QPSK:   constel = DTAPI_MOD_DVBT_QPSK;  break;
                    case QAM_16: constel = DTAPI_MOD_DVBT_QAM16; break;
                    case QAM_64: constel = DTAPI_MOD_DVBT_QAM64; break;
                    default: break;
                }
                switch (input_dvbt->guard_interval) {
                    case GUARD_1_32: guard = DTAPI_MOD_DVBT_G_1_32; break;
                    case GUARD_1_16: guard = DTAPI_MOD_DVBT_G_1_16; break;
                    case GUARD_1_8:  guard = DTAPI_MOD_DVBT_G_1_8;  break;
                    case GUARD_1_4:  guard = DTAPI_MOD_DVBT_G_1_4;  break;
                    default: break;
                }
                switch (input_dvbt->transmission_mode) {
                    case TM_2K: tr_mode = DTAPI_MOD_DVBT_2K; break;
                    case TM_4K: tr_mode = DTAPI_MOD_DVBT_4K; break;
                    case TM_8K: tr_mode = DTAPI_MOD_DVBT_8K; break;
                    default: break;
                }
            }
            fec = intValue<int>("convolutional-rate", fec);
            bw = intValue<int>("bandwidth", bw);
            constel = intValue<int>("constellation", constel);
            guard = intValue<int>("guard-interval", guard);
            tr_mode = intValue<int>("transmission-mode", tr_mode);
            const int interleave = present("indepth-interleave") ? DTAPI_MOD_DVBT_INDEPTH : DTAPI_MOD_DVBT_NATIVE;
            const bool time_slice = present("time-slice");
            const bool mpe_fec = present("mpe-fec");
            const int dvb_h = time_slice || mpe_fec ? DTAPI_MOD_DVBT_ENA4849 : DTAPI_MOD_DVBT_DIS4849;
            const int s48 = time_slice ? DTAPI_MOD_DVBT_S48 : DTAPI_MOD_DVBT_S48_OFF;
            const int s49 = mpe_fec ? DTAPI_MOD_DVBT_S49 : DTAPI_MOD_DVBT_S49_OFF;
            const int cell_id = intValue<int>("cell-id", -1);
            tsp->verbose("using DVB-T FEC " + DektecFEC.name(fec) +
                         ", bandwidth " + DektecDVBTProperty.name(bw) +
                         ", constellation " + DektecDVBTProperty.name(constel) +
                         ", guard " + DektecDVBTProperty.name(guard) +
                         ", transmission " + DektecDVBTProperty.name(tr_mode));
            const int param1 = bw | constel | guard | interleave | tr_mode | dvb_h | s48 | s49;
            // Compute exact expected bitrate (no symbol rate on DVB-T)
            if (!setBitrate(-1, modulation_type, fec, param1, cell_id)) {
                return false;
            }
            // bw constel guard tr_mode
            status = _guts->chan.SetModControl(modulation_type, fec, param1, cell_id);
            break;
        }

        case DTAPI_MOD_DVBT2: {
            Dtapi::DtDvbT2Pars pars;
            pars.Init(); // default values
            pars.m_Bandwidth = intValue<int>("bandwidth", DTAPI_DVBT2_8MHZ);
            pars.m_FftMode = intValue<int>("fft-mode", DTAPI_DVBT2_FFT_32K);
            pars.m_Miso = intValue<int>("miso", DTAPI_DVBT2_MISO_OFF);
            pars.m_GuardInterval = intValue<int>("t2-guard-interval", DTAPI_DVBT2_GI_1_128);
            pars.m_Papr = intValue<int>("papr", DTAPI_DVBT2_PAPR_NONE);
            pars.m_BwtExt = present("bandwidth-extension") ? DTAPI_DVBT2_BWTEXT_ON : DTAPI_DVBT2_BWTEXT_OFF;
            pars.m_PilotPattern = intValue<int>("pilot-pattern", DTAPI_DVBT2_PP_7);
            pars.m_NumT2Frames = intValue<int>("t2-fpsf", 2);
            pars.m_L1Modulation = intValue<int>("t2-l1-modulation", DTAPI_DVBT2_QAM16);
            pars.m_FefEnable = present("fef");
            pars.m_FefType = intValue<int>("fef-type", 0);
            pars.m_FefLength = intValue<int>("fef-length", 1);
            pars.m_FefS1 = intValue<int>("fef-s1", 2);
            pars.m_FefS2 = intValue<int>("fef-s2", 1);
            pars.m_FefInterval = intValue<int>("fef-interval", 1);
            pars.m_FefSignal = intValue<int>("fef-signal", DTAPI_DVBT2_FEF_ZERO);
            pars.m_CellId = intValue<int>("cell-id", 0);
            pars.m_NetworkId = intValue<int>("t2-network-id", 0);
            pars.m_T2SystemId = intValue<int>("t2-system-id", 0);
            // Obsolete field in DTAPI 4.10.0.145:
            // pars.m_Frequency = int (frequency);
            pars.m_NumPlps = 1; // This version supports single-PLP only
            pars.m_Plps[0].Init(); // default values
            pars.m_Plps[0].m_Hem = present("plp0-high-efficiency");
            pars.m_Plps[0].m_Npd = present("plp0-null-packet-deletion");
            pars.m_Plps[0].m_Issy = intValue<int>("plp0-issy", DTAPI_DVBT2_ISSY_NONE);
            pars.m_Plps[0].m_Id = intValue<int>("plp0-id", 0);
            pars.m_Plps[0].m_GroupId = intValue<int>("plp0-group-id", 0);
            pars.m_Plps[0].m_Type = intValue<int>("plp0-type", DTAPI_DVBT2_PLP_TYPE_COMM);
            pars.m_Plps[0].m_CodeRate = intValue<int>("plp0-code-rate", DTAPI_DVBT2_COD_2_3);
            pars.m_Plps[0].m_Modulation = intValue<int>("plp0-modulation", DTAPI_DVBT2_QAM256);
            pars.m_Plps[0].m_Rotation = present("plp0-rotation");
            pars.m_Plps[0].m_FecType = intValue<int>("plp0-fec-type", DTAPI_DVBT2_LDPC_64K);
            pars.m_Plps[0].m_TimeIlLength = intValue<int>("plp0-il-length", 3);
            pars.m_Plps[0].m_TimeIlType = intValue<int>("plp0-il-type", DTAPI_DVBT2_IL_ONETOONE);
            pars.m_Plps[0].m_InBandAFlag = present("plp0-in-band");
            // Compute other fields
            Dtapi::DtDvbT2ParamInfo info;
            status = pars.OptimisePlpNumBlocks(info, pars.m_Plps[0].m_NumBlocks, pars.m_NumDataSyms);
            if (status != DTAPI_OK) {
                return startError("error computing PLP parameters", status);
            }
            // Report actual parameters in debug mode
            tsp->debug("DVB-T2: DtDvbT2Pars = {");
            DektecDevice::Report(pars, *tsp, Severity::Debug, "  ");
            tsp->debug("}");
            tsp->debug("DVB-T2: DtDvbT2ParamInfo = {");
            DektecDevice::Report(info, *tsp, Severity::Debug, "  ");
            tsp->debug("}");
            // Check validity of T2 parameters
            status = pars.CheckValidity();
            if (status != DTAPI_OK) {
                return startError("invalid combination of DVB-T2 parameters", status);
            }
            // Set modulation parameters
            status = _guts->chan.SetModControl(pars);
            break;
        }

        case DTAPI_MOD_ATSC: {
            int constel = DTAPI_MOD_ATSC_VSB8;
            if (input_atsc != 0) {
                switch (input_atsc->modulation) {
                    case VSB_8:  constel = DTAPI_MOD_ATSC_VSB8;  break;
                    case VSB_16: constel = DTAPI_MOD_ATSC_VSB16; break;
                    default: break;
                }
            }
            constel = intValue<int>("vsb", constel);
            const int taps = intValue<int>("vsb-taps", 64);
            tsp->verbose("using ATSC " + DektecVSB.name(constel));
            status = _guts->chan.SetModControl(modulation_type, constel, taps, 0);
            break;
        }

        case DTAPI_MOD_ADTBT:
        case DTAPI_MOD_DMBTH: {
            const int bw = intValue<int>("bandwidth", DTAPI_MOD_DTMB_8MHZ);
            const int constel = intValue<int>("dmb-constellation", DTAPI_MOD_DTMB_QAM64);
            const int fec = intValue<int>("dmb-fec", DTAPI_MOD_DTMB_0_8);
            const int header = intValue<int>("dmb-header", DTAPI_MOD_DTMB_PN945);
            const int interleaver = intValue<int>("dmb-interleaver", DTAPI_MOD_DTMB_IL_1);
            const int pilots = present("pilots") ? DTAPI_MOD_DTMB_PILOTS : DTAPI_MOD_DTMB_NO_PILOTS;
            const int frame_num = present("dmb-frame-numbering") ? DTAPI_MOD_DTMB_USE_FRM_NO : DTAPI_MOD_DTMB_NO_FRM_NO;
            status = _guts->chan.SetModControl(modulation_type, bw | constel | fec | header | interleaver | pilots | frame_num, 0, 0);
            break;
        }

        case DTAPI_MOD_CMMB: {
            if (_guts->cur_bitrate <= 0) {
                return startError("unknown bitrate, required with CMMB modulation, use --bitrate option", DTAPI_OK);
            }
            if (!present("cmmb-pid")) {
                return startError("option --cmmb-pid is required with CMMB modulation", DTAPI_OK);
            }
            Dtapi::DtCmmbPars pars;
            pars.m_Bandwidth = intValue<int>("cmmb-bandwidth", DTAPI_CMMB_BW_8MHZ);
            pars.m_TsRate = int(_guts->cur_bitrate);
            pars.m_TsPid = intValue<int>("cmmb-pid", 0);
            pars.m_AreaId = intValue<int>("cmmb-area-id", 0);
            pars.m_TxId = intValue<int>("cmmb-transmitter-id", 0);
            status = _guts->chan.SetModControl(pars);
            break;
        }

        case DTAPI_MOD_ISDBT: {
            return startError("ISDB-T modulation not yet supported", DTAPI_OK);
            break;
        }

        case -1: {
            // No modulation specified
            status = DTAPI_OK;
            break;
        }

        default: {
            return startError("unsupported modulation type", DTAPI_OK);
        }
    }

    if (status != DTAPI_OK) {
        return startError("error while setting modulation mode", status);
    }

    // Set carrier frequency.
    // Make sure to use "__int64" and not "int64_t" in SetRfControl to avoid
    // ambiguous overloading.
    tsp->verbose("setting output carrier frequency to " + Decimal(frequency) + " Hz");
    status = _guts->chan.SetRfControl(__int64(frequency));
    if (status != DTAPI_OK) {
        return startError("set modulator frequency error", status);
    }
    status = _guts->chan.SetRfMode(DTAPI_UPCONV_NORMAL | (present("inversion") ? DTAPI_UPCONV_SPECINV : 0));
    if (status != DTAPI_OK) {
        return startError("set modulator RF mode", status);
    }

    // Finally ok
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::stop()
{
    if (_guts->is_started) {
        tsp->verbose("terminating " + _guts->device.model + " output");

        // Mute output signal for modulators which support this
        if (_guts->mute_on_stop) {
            Dtapi::DTAPI_RESULT status = _guts->chan.SetRfMode(DTAPI_UPCONV_MUTE);
            if (status != DTAPI_OK) {
                tsp->error("error muting modulator output: " + DektecStrError(status));
            }
        }

        // Detach the channel and the device
        _guts->chan.Detach(_guts->detach_mode);
        _guts->dtdev.Detach();

        _guts->is_started = false;
        tsp->verbose(_guts->device.model + " output terminated");
    }
    return true;
}


//----------------------------------------------------------------------------
// Output destructor
//----------------------------------------------------------------------------

ts::DektecOutputPlugin::~DektecOutputPlugin()
{
    if (_guts != 0) {
        stop();
        delete _guts;
        _guts = 0;
    }
}


//----------------------------------------------------------------------------
// Get output bitrate
//----------------------------------------------------------------------------

ts::BitRate ts::DektecOutputPlugin::getBitrate()
{
    int bitrate = 0;
    if (_guts->is_started) {
        Dtapi::DTAPI_RESULT status = _guts->chan.GetTsRateBps(bitrate);
        if (status != DTAPI_OK) {
            tsp->error("error getting Dektec device output bitrate: " + DektecStrError(status));
            bitrate = 0;
        }
    }
    return bitrate;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::send(const TSPacket* buffer, size_t packet_count)
{
    if (!_guts->is_started) {
        return false;
    }

    char* data = reinterpret_cast<char*>(const_cast<TSPacket*> (buffer));
    int remain = int(packet_count * PKT_SIZE);
    Dtapi::DTAPI_RESULT status;

    // If no bitrate was specified on the command line, adjust the bitrate
    // when input bitrate changes.
    BitRate new_bitrate;
    if (_guts->opt_bitrate == 0 && _guts->cur_bitrate != (new_bitrate = tsp->bitrate())) {
        status = _guts->chan.SetTsRateBps(int(new_bitrate));
        if (status != DTAPI_OK) {
            tsp->error("error setting output bitrate on Dektec device: " + DektecStrError(status));
        }
        else {
            _guts->cur_bitrate = new_bitrate;
            tsp->log(Severity::Verbose, "new output bitrate: " + Decimal(_guts->cur_bitrate) + " b/s");
        }
    }

    // Loop on write until everything is gone.
    while (remain > 0) {

        // Maximum size of next I/O
        int max_io_size = DTA_MAX_IO_SIZE;

        // In starting phase, we load the FIFO without transmitting.
        if (_guts->starting) {

            // Get current load in FIFO
            int fifo_load;
            status = _guts->chan.GetFifoLoad(fifo_load);
            if (status != DTAPI_OK) {
                tsp->error("error getting output fifo load: " + DektecStrError(status));
                return false;
            }

            // We consider the FIFO is loaded when 80% full.
            const int max_size = (8 * DTA_FIFO_SIZE) / 10;
            if (fifo_load < max_size - int(PKT_SIZE)) {
                // Remain in starting phase, limit next I/O size
                max_io_size = max_size - fifo_load;
            }
            else {
                // FIFO now full enough to start transmitting
                tsp->verbose(_guts->device.model + " output FIFO load is " + Decimal(fifo_load) + " bytes, starting transmission");
                status = _guts->chan.SetTxControl(DTAPI_TXCTRL_SEND);
                if (status != DTAPI_OK) {
                    tsp->error ("output device start send error: " + DektecStrError (status));
                    return false;
                }
                // Now fully started
                _guts->starting = false;
            }
        }

        // Limit the transfer size by the maximum I/O size on the device
        int cursize = RoundDown(std::min(remain, max_io_size), int(PKT_SIZE));

        status = _guts->chan.Write(data, cursize);
        if (status != DTAPI_OK) {
            tsp->error("transmission error on Dektec device: " + DektecStrError(status));
            return false;
        }

        data += cursize;
        remain -= cursize;
    }

    return true;
}

#endif // TS_NO_DTAPI
