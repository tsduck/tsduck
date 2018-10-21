//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Stubs when compiled without Dektec support.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

ts::DektecOutputPlugin::DektecOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a Dektec DVB-ASI or modulator device", u"[options]")
{
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
    Guts();                             // Constructor
    bool                 starting;      // Starting phase (loading FIFO, no transmit)
    bool                 is_started;    // Device started
    bool                 mute_on_stop;  // Device supports output muting
    int                  dev_index;     // Dektec device index
    int                  chan_index;    // Device output channel index
    DektecDevice         device;        // Device characteristics
    Dtapi::DtDevice      dtdev;         // Device descriptor
    Dtapi::DtOutpChannel chan;          // Output channel
    int                  detach_mode;   // Detach mode
    BitRate              opt_bitrate;   // Bitrate option (0 means unspecified)
    BitRate              cur_bitrate;   // Current output bitrate
    int                  max_fifo_size; // Maximum FIFO size
    int                  fifo_size;     // Actual FIFO size
};


ts::DektecOutputPlugin::Guts::Guts() :
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
    cur_bitrate(0),
    max_fifo_size(0),
    fifo_size(0)
{
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::DektecOutputPlugin::DektecOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a Dektec DVB-ASI or modulator device", u"[options]"),
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
    option(u"204");
    help(u"204",
         u"ASI devices: Send 204-byte packets (188 meaningful bytes plus 16 "
         u"stuffing bytes for RS coding). By default, send 188-byte packets.");

    option(u"bandwidth", 0, Enumeration({
        {u"1.7", DTAPI_DVBT2_1_7MHZ},
        {u"5",   DTAPI_DVBT2_5MHZ},
        {u"6",   DTAPI_DVBT2_6MHZ},
        {u"7",   DTAPI_DVBT2_7MHZ},
        {u"8",   DTAPI_DVBT2_8MHZ},
        {u"10",  DTAPI_DVBT2_10MHZ},
    }));
    help(u"bandwidth",
         u"DVB-T/H, DVB-T2, ADTB-T and DMB-T/H modulators: indicate bandwidth "
         u"in MHz. The default is 8 MHz. "
         u"The bandwidth values 1.7 and 10 MHz are valid for DVB-T2 only.");

    option(u"bandwidth-extension");
    help(u"bandwidth-extension",
         u"DVB-T2 modulators: indicate that the extended carrier mode is used. "
         u"By default, use normal carrier mode.");

    option(u"bitrate", 'b', POSITIVE);
    help(u"bitrate",
         u"Specify output bitrate in bits/second. By default, use the input "
         u"device bitrate or, if the input device cannot report bitrate, analyze "
         u"some PCR's at the beginning of the input stream to evaluate the "
         u"original bitrate of the transport stream.");

    option(u"cell-id", 0,  UINT16);
    help(u"cell-id",
         u"DVB-T and DVB-T2 modulators: indicate the cell identifier to set in the "
         u"transmition parameters signaling (TPS). Disabled by default with DVB-T. "
         u"Default value is 0 with DVB-T2.");

    option(u"channel", 'c', UNSIGNED);
    help(u"channel",
         u"Channel index on the output Dektec device. By default, use the "
         u"first output channel on the device.");

    option(u"cmmb-area-id", 0, INTEGER, 0, 1, 0, 127);
    help(u"cmmb-area-id",
         u"CMMB modulators: indicate the area id. The valid range is 0 to 127. "
         u"The default is zero.");

    option(u"cmmb-bandwidth", 0, Enumeration({
        {u"2", DTAPI_CMMB_BW_2MHZ},
        {u"8", DTAPI_CMMB_BW_8MHZ},
    }));
    help(u"cmmb-bandwidth",
         u"CMMB modulators: indicate bandwidth in MHz. The default is 8 MHz.");

    option(u"cmmb-pid", 0, PIDVAL);
    help(u"cmmb-pid",
         u"CMMB modulators: indicate the PID of the CMMB stream in the transport "
         u"stream. This is a required parameter for CMMB modulation.");

    option(u"cmmb-transmitter-id", 0, INTEGER, 0, 1, 0, 127);
    help(u"cmmb-transmitter-id",
         u"CMMB modulators: indicate the transmitter id. The valid range is 0 to "
         u"127. The default is zero.");

    option(u"constellation", 0, Enumeration({
        {u"QPSK",   DTAPI_MOD_DVBT_QPSK},
        {u"16-QAM", DTAPI_MOD_DVBT_QAM16},
        {u"64-QAM", DTAPI_MOD_DVBT_QAM64},
    }));
    help(u"constellation",
         u"DVB-T modulators: indicate the constellation type. Must be one of "
         u"QPSK, 16-QAM, 64-QAM. The default is 64-QAM.");

    option(u"convolutional-rate", 'r', Enumeration({
        {u"1/2",  DTAPI_MOD_1_2},
        {u"1/3",  DTAPI_MOD_1_3},  // DVB-S.2 only
        {u"1/4",  DTAPI_MOD_1_4},  // DVB-S.2 only
        {u"2/3",  DTAPI_MOD_2_3},
        {u"2/5",  DTAPI_MOD_2_5},  // DVB-S.2 only
        {u"3/4",  DTAPI_MOD_3_4},
        {u"3/5",  DTAPI_MOD_3_5},  // DVB-S.2 only
        {u"4/5",  DTAPI_MOD_4_5},
        {u"5/6",  DTAPI_MOD_5_6},
        {u"6/7",  DTAPI_MOD_6_7},
        {u"7/8",  DTAPI_MOD_7_8},
        {u"8/9",  DTAPI_MOD_8_9},  // DVB-S.2 only
        {u"9/10", DTAPI_MOD_9_10}, // DVB-S.2 only
    }));
    help(u"convolutional-rate",
         u"For modulators devices only: specify the convolutional rate. "
         u"The specified value depends on the modulation type.\n"
         u"DVB-S: 1/2, 2/3, 3/4, 4/5, 5/6, 6/7, 7/8.\n"
         u"DVB-S2: 1/2, 1/3, 1/4, 2/3, 2/5, 3/4, 3/5, 4/5, 5/6, 6/7, 7/8, 8/9, 9/10.\n"
         u"DVB-T: 1/2, 2/3, 3/4, 5/6, 7/8.\n"
         u"The default is 3/4.");

    option(u"device", 'd', UNSIGNED);
    help(u"device",
         u"Device index, from 0 to N-1 (with N being the number of Dektec devices "
         u"in the system). Use the command \"tsdektec -a [-v]\" to have a "
         u"complete list of devices in the system. By default, use the first "
         u"output Dektec device.");

    option(u"dmb-constellation", 0, Enumeration({
        {u"4-QAM-NR", DTAPI_MOD_DTMB_QAM4NR},
        {u"4-QAM",    DTAPI_MOD_DTMB_QAM4},
        {u"16-QAM",   DTAPI_MOD_DTMB_QAM16},
        {u"32-QAM",   DTAPI_MOD_DTMB_QAM32},
        {u"64-QAM",   DTAPI_MOD_DTMB_QAM64},
    }));
    help(u"dmb-constellation",
         u"DMB-T/H, ADTB-T modulators: indicate the constellation type. The default is 64-QAM. "
         u"4-QAM-NR and 32-QAM can be used only with --dmb-fec 0.8.");

    option(u"dmb-fec", 0, Enumeration({
        {u"0.4", DTAPI_MOD_DTMB_0_4},
        {u"0.6", DTAPI_MOD_DTMB_0_6},
        {u"0.8", DTAPI_MOD_DTMB_0_8},
    }));
    help(u"dmb-fec",
         u"DMB-T/H, ADTB-T modulators: indicate the FEC code rate. The default is 0.8. ");

    option(u"dmb-frame-numbering");
    help(u"dmb-frame-numbering",
         u"DMB-T/H, ADTB-T modulators: indicate to use frame numbering. The default "
         u"is to use no frame numbering.");

    option(u"dmb-header", 0, Enumeration({
        {u"PN420", DTAPI_MOD_DTMB_PN420},
        {u"PN595", DTAPI_MOD_DTMB_PN595},
        {u"PN945", DTAPI_MOD_DTMB_PN945},
    }));
    help(u"dmb-header",
         u"DMB-T/H, ADTB-T modulators: indicate the FEC frame header mode. "
         u"The default is PN945.");

    option(u"dmb-interleaver", 0, Enumeration({
        {u"1", DTAPI_MOD_DTMB_IL_1},
        {u"2", DTAPI_MOD_DTMB_IL_2},
    }));
    help(u"dmb-interleaver",
         u"DMB-T/H, ADTB-T modulators: indicate the interleaver mode. Must be one "
         u"1 (B=54, M=240) or 2 (B=54, M=720). The default is 1.");

    option(u"fef");
    help(u"fef",
         u"DVB-T2 modulators: enable insertion of FEF's (Future Extension Frames). "
         u"Not enabled by default.");

    option(u"fef-interval", 0, INTEGER, 0, 1, 1, 255);
    help(u"fef-interval",
         u"DVB-T2 modulators: indicate the number of T2 frames between two FEF "
         u"parts. The valid range is 1 to 255 and --t2-fpsf shall be divisible by "
         u"--fef-interval. The default is 1.");

    option(u"fef-length", 0, INTEGER, 0, 1, 0, 0x003FFFFF);
    help(u"fef-length",
         u"DVB-T2 modulators: indicate the length of a FEF-part in number of T-units "
         u"(= samples). The valid range is 0 to 0x3FFFFF. The default is 1.");

    option(u"fef-s1", 0, INTEGER, 0, 1, 2, 7);
    help(u"fef-s1",
         u"-T2 modulators: indicate the S1-field value in the P1 signalling data. "
         u"Valid values: 2, 3, 4, 5, 6 and 7. The default is 2.");

    option(u"fef-s2", 0, INTEGER, 0, 1, 1, 15);
    help(u"fef-s2",
         u"DVB-T2 modulators: indicate the S2-field value in the P1 signalling data. "
         u"Valid values: 1, 3, 5, 7, 9, 11, 13 and 15. The default is 1.");

    option(u"fef-signal", 0, Enumeration({
        {u"0",      DTAPI_DVBT2_FEF_ZERO},
        {u"1K",     DTAPI_DVBT2_FEF_1K_OFDM},
        {u"1K-384", DTAPI_DVBT2_FEF_1K_OFDM_384},
    }));
    help(u"fef-signal",
         u"DVB-T2 modulators: indicate the type of signal generated during the FEF "
         u"period. Must be one of \"0\" (zero I/Q samples during FEF), \"1K\" (1K "
         u"OFDM symbols with 852 active carriers containing BPSK symbols, same PRBS "
         u"as the T2 dummy cells, not reset between symbols) or \"1K-384\" (1K OFDM "
         u"symbols with 384 active carriers containing BPSK symbols). "
         u"The default is 0.");
    
    option(u"fef-type", 0, INTEGER, 0, 1, 0, 15);
    help(u"fef-type",
         u"DVB-T2 modulators: indicate the FEF type. The valid range is 0 ... 15. "
         u"The default is 0.");

    option(u"fft-mode", 0, Enumeration({
        {u"1K",  DTAPI_DVBT2_FFT_1K},
        {u"2K",  DTAPI_DVBT2_FFT_2K},
        {u"4K",  DTAPI_DVBT2_FFT_4K},
        {u"8K",  DTAPI_DVBT2_FFT_8K},
        {u"16K", DTAPI_DVBT2_FFT_16K},
        {u"32K", DTAPI_DVBT2_FFT_32K},
    }));
    help(u"fft-mode",
         u"DVB-T2 modulators: indicate the FFT mode. The default is 32K.");

    option(u"fifo-size", 0, INTEGER, 0, 1, 1024, UNLIMITED_VALUE);
    help(u"fifo-size",
         u"Set the FIFO size in bytes of the output channel in the Dektec device. The "
         u"default value depends on the device type.");

    option(u"frequency", 'f', POSITIVE);
    help(u"frequency",
         u"All modulator devices: indicate the frequency, in Hz, of the output "
         u"carrier. There is no default. For OFDM modulators, the options "
         u"--uhf-channel or --vhf-channel and --offset-count may be used instead. "
         u"For DVB-S/S2 modulators, the specified frequency is the \"intermediate\" "
         u"frequency. For convenience, the option --satellite-frequency can be used "
         u"instead of --frequency when the intermediate frequency is unknown. "
         u"For DTA-107 modulators, the valid range is 950 MHz to 2150 MHz. "
         u"For DTA-110 and 110T modulators, the valid range is 400 MHz to 862 MHz. "
         u"For DTA-115 modulators, the valid range is 47 MHz to 862 MHz.");

    option(u"guard-interval", 'g', Enumeration({
        {u"1/32", DTAPI_MOD_DVBT_G_1_32},
        {u"1/16", DTAPI_MOD_DVBT_G_1_16},
        {u"1/8",  DTAPI_MOD_DVBT_G_1_8},
        {u"1/4",  DTAPI_MOD_DVBT_G_1_4},
    }));
    help(u"guard-interval",
         u"DVB-T modulators: indicate the guard interval. The default is 1/32.");

    option(u"indepth-interleave");
    help(u"indepth-interleave",
         u"DVB-T modulators: indicate to use in-depth interleave. "
         u"The default is native interleave.");

    option(u"input-modulation", 'i');
    help(u"input-modulation",
         u"All modulators devices: try to guess modulation parameters from input "
         u"stream. If the input plugin is \"dvb\", use its modulation parameters. "
#if defined(TS_WINDOWS)
         u"Warning: not always accurate on Windows systems. "
#endif
         u"Otherwise, if the specified modulation is DVB-T, try to guess "
         u"some modulation parameters from the bitrate.");

    option(u"instant-detach");
    help(u"instant-detach",
         u"At end of stream, perform an \"instant detach\" of the output channel. "
         u"The default is to wait until all bytes are sent. The default is fine "
         u"for ASI devices. With modulators, the \"wait until sent\" mode may "
         u"hang at end of stream and --instant-detach avoids this.");

    option(u"inversion");
    help(u"inversion", u"All modulators devices: enable spectral inversion.");

    option(u"j83", 0, Enumeration({
        {u"A", DTAPI_MOD_J83_A},
        {u"B", DTAPI_MOD_J83_B},
        {u"C", DTAPI_MOD_J83_C},
    }));
    help(u"j83",
         u"QAM modulators: indicate the ITU-T J.83 annex to use. Must be one of "
         u"\"A\" (DVB-C), \"B\" (American QAM) or \"C\" (Japanese QAM). The default is A.");

    option(u"level", 'l', INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    help(u"level",
         u"Modulators: indicate the output level in units of 0.1 dBm (e.g. "
         u"--level -30 means -3 dBm). Not supported by all devices. "
         u"For DTA-107 modulators, the valid range is -47.0 to -27.0 dBm. "
         u"For DTA-115, QAM, the valid range is -35.0 to 0.0 dBm. "
         u"For DTA-115, OFDM, ISDB-T, the valid range is -38.0 to -3.0 dBm.");

    option(u"lnb", 0, Args::STRING);
    help(u"lnb",
         u"DVB-S/S2 modulators: description of the LNB which is used to convert the "
         u"--satellite-frequency into an intermediate frequency. This option is "
         u"useless when --satellite-frequency is not specified. The format of the "
         u"string is \"low_freq[,high_freq[,switch_freq]]\" where all frequencies "
         u"are in MHz. The characteristics of the default universal LNB are "
         u"low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.");

    option(u"miso", 0, Enumeration({
        {u"OFF",  DTAPI_DVBT2_MISO_OFF},
        {u"1",    DTAPI_DVBT2_MISO_TX1},
        {u"2",    DTAPI_DVBT2_MISO_TX2},
        {u"BOTH", DTAPI_DVBT2_MISO_TX1TX2},
    }));
    help(u"miso",
         u"DVB-T2 modulators: indicate the MISO mode. "
         u"The default si OFF. This mode can be used to simulate antenna 1, "
         u"antenna 2 or the average of antenna 1 and antenna 2 to simulate reception "
         u"halfway between the antennas.");

    option(u"modulation", 'm', Enumeration({
        {u"DVB-S",         DTAPI_MOD_DVBS_QPSK},
        {u"DVB-S-QPSK",    DTAPI_MOD_DVBS_QPSK},
        {u"DVB-S-BPSK",    DTAPI_MOD_DVBS_BPSK},
        {u"DVB-S2",        DTAPI_MOD_DVBS2_QPSK},
        {u"DVB-S2-QPSK",   DTAPI_MOD_DVBS2_QPSK},
        {u"DVB-S2-8PSK",   DTAPI_MOD_DVBS2_8PSK},
        {u"DVB-S2-16APSK", DTAPI_MOD_DVBS2_16APSK},
        {u"DVB-S2-32APSK", DTAPI_MOD_DVBS2_32APSK},
        {u"DVB-T",         DTAPI_MOD_DVBT},
        {u"DVB-T2",        DTAPI_MOD_DVBT2},
        {u"ATSC-VSB",      DTAPI_MOD_ATSC},
        {u"4-QAM",         DTAPI_MOD_QAM4},
        {u"16-QAM",        DTAPI_MOD_QAM16},
        {u"32-QAM",        DTAPI_MOD_QAM32},
        {u"64-QAM",        DTAPI_MOD_QAM64},
        {u"128-QAM",       DTAPI_MOD_QAM128},
        {u"256-QAM",       DTAPI_MOD_QAM256},
        {u"ISDB-T",        DTAPI_MOD_ISDBT},
        {u"DMB-T",         DTAPI_MOD_DMBTH},
        {u"ADTB-T",        DTAPI_MOD_ADTBT},
        {u"CMMB",          DTAPI_MOD_CMMB},
    }));
    help(u"modulation",
         u"For modulators, indicate the modulation type. Must be one of:  "
         u"4-QAM, 16-QAM, 32-QAM, 64-QAM, 128-QAM, 256-QAM, ADTB-T, ATSC-VSB, CMMB, "
         u"DMB-T, DVB-S, DVB-S-QPSK (same as DVB-S), DVB-S-BPSK, DVB-S2, DVB-S2-QPSK "
         u"(same as DVB-S2), DVB-S2-8PSK, DVB-S2-16APSK, DVB-S2-32APSK, DVB-T,  "
         u"DVB-T2, ISDB-T. For DVB-H, specify DVB-T. For DMB-H, specify DMB-T. "
         u"The supported modulation types depend on the device model. "
         u"The default modulation type is:\n"
         u"DTA-107:   DVB-S-QPSK\n"
         u"DTA-107S2: DVB-S2-QPSK\n"
         u"DTA-110:   64-QAM\n"
         u"DTA-110T:  DVB-T\n"
         u"DTA-115:   DVB-T");

    option(u"mpe-fec");
    help(u"mpe-fec",
         u"DVB-T/H modulators: indicate that at least one elementary stream uses "
         u"MPE-FEC (DVB-H signalling).");

    option(u"offset-count", 'o', INTEGER, 0, 1, -3, 3);
    help(u"offset-count",
         u"UHF and VHF modulators: indicate the number of offsets from the UHF or "
         u"VHF channel. The default is zero. See options --uhf-channel and "
         u"--vhf-channel.");

    option(u"papr", 0, Enumeration({
        {u"NONE", DTAPI_DVBT2_PAPR_NONE},
        {u"ACE",  DTAPI_DVBT2_PAPR_ACE},
        {u"TR",   DTAPI_DVBT2_PAPR_TR},
        {u"BOTH", DTAPI_DVBT2_PAPR_ACE_TR},
    }));
    help(u"papr",
         u"DVB-T2 modulators: indicate the Peak to Average Power Reduction method. "
         u"Must be one of NONE, ACE (Active Constellation Extension), TR (power "
         u"reduction with reserved carriers) or BOTH (both ACE and TS). The default "
         u"is NONE.");

    option(u"pilots", 0);
    help(u"pilots", u"DVB-S2 and ADTB-T modulators: enable pilots (default: no pilot).");

    option(u"pilot-pattern", 'p', Enumeration({
        {u"1", DTAPI_DVBT2_PP_1},
        {u"2", DTAPI_DVBT2_PP_2},
        {u"3", DTAPI_DVBT2_PP_3},
        {u"4", DTAPI_DVBT2_PP_4},
        {u"5", DTAPI_DVBT2_PP_5},
        {u"6", DTAPI_DVBT2_PP_6},
        {u"7", DTAPI_DVBT2_PP_7},
        {u"8", DTAPI_DVBT2_PP_8},
    }));
    help(u"pilot-pattern",
         u"DVB-T2 modulators: indicate the pilot pattern to use, a value in the "
         u"range 1 to 8. The default is 7.");

    option(u"plp0-code-rate", 0, Enumeration({
        {u"1/2", DTAPI_DVBT2_COD_1_2},
        {u"3/5", DTAPI_DVBT2_COD_3_5},
        {u"2/3", DTAPI_DVBT2_COD_2_3},
        {u"3/4", DTAPI_DVBT2_COD_3_4},
        {u"4/5", DTAPI_DVBT2_COD_4_5},
        {u"5/6", DTAPI_DVBT2_COD_5_6},
    }));
    help(u"plp0-code-rate",
         u"DVB-T2 modulators: indicate the convolutional coding rate used by the "
         u"PLP #0. The default is 2/3.");

    option(u"plp0-fec-type", 0, Enumeration({
        {u"16K", DTAPI_DVBT2_LDPC_16K},
        {u"64K", DTAPI_DVBT2_LDPC_64K},
    }));
    help(u"plp0-fec-type",
         u"DVB-T2 modulators: indicate the FEC type used by the PLP #0. The default is 64K LPDC.");

    option(u"plp0-group-id", 0, UINT8);
    help(u"plp0-group-id",
         u"DVB-T2 modulators: indicate the PLP group with which the PLP #0 is "
         u"associated. The valid range is 0 to 255. The default is 0.");

    option(u"plp0-high-efficiency");
    help(u"plp0-high-efficiency",
         u"DVB-T2 modulators: indicate that the PLP #0 uses High Efficiency Mode "
         u"(HEM). Otherwise Normal Mode (NM) is used.");

    option(u"plp0-id", 0, UINT8);
    help(u"plp0-id",
         u"DVB-T2 modulators: indicate the unique identification of the PLP #0 "
         u"within the T2 system. The valid range is 0 to 255. The default is 0.");

    option(u"plp0-il-length", 0, UINT8);
    help(u"plp0-il-length",
         u"DVB-T2 modulators: indicate the time interleaving length for PLP #0. "
         u"If --plp0-il-type is set to \"ONE-TO-ONE\" (the default), this parameter "
         u"specifies the number of TI-blocks per interleaving frame. "
         u"If --plp0-il-type is set to \"MULTI\", this parameter specifies the "
         u"number of T2 frames to which each interleaving frame is mapped. "
         u"The valid range is 0 to 255. The default is 3.");

    option(u"plp0-il-type", 0, Enumeration({
        {u"ONE-TO-ONE", DTAPI_DVBT2_IL_ONETOONE},
        {u"MULTI",      DTAPI_DVBT2_IL_MULTI},
    }));
    help(u"plp0-il-type",
         u"DVB-T2 modulators: indicate the type of interleaving used by the PLP #0. "
         u"Must be one of \"ONE-TO-ONE\" (one interleaving frame corresponds to one "
         u"T2 frame) or \"MULTI\" (one interleaving frame is carried in multiple T2 "
         u"frames). The default is ONE-TO-ONE.");

    option(u"plp0-in-band");
    help(u"plp0-in-band",
         u"DVB-T2 modulators: indicate that the in-band flag is set and in-band "
         u"signalling information is inserted in PLP #0.");

    option(u"plp0-issy", 0, Enumeration({
        {u"NONE",  DTAPI_DVBT2_ISSY_NONE},
        {u"SHORT", DTAPI_DVBT2_ISSY_SHORT},
        {u"LONG",  DTAPI_DVBT2_ISSY_LONG},
    }));
    help(u"plp0-issy",
         u"DVB-T2 modulators: type of ISSY field to compute and inserte in PLP #0. "
         u"The default is NONE.");

    option(u"plp0-modulation", 0, Enumeration({
        {u"BPSK",    DTAPI_DVBT2_BPSK},
        {u"QPSK",    DTAPI_DVBT2_QPSK},
        {u"16-QAM",  DTAPI_DVBT2_QAM16},
        {u"64-QAM",  DTAPI_DVBT2_QAM64},
        {u"256-QAM", DTAPI_DVBT2_QAM256},
    }));
    help(u"plp0-modulation",
         u"DVB-T2 modulators: indicate the modulation used by PLP #0. The default is 256-QAM.");

    option(u"plp0-null-packet-deletion");
    help(u"plp0-null-packet-deletion",
         u"DVB-T2 modulators: indicate that null-packet deletion is active in "
         u"PLP #0. Otherwise it is not active.");

    option(u"plp0-rotation");
    help(u"plp0-rotation",
         u"DVB-T2 modulators: indicate that constellation rotation is used for "
         u"PLP #0. Otherwise not.");

    option(u"plp0-type", 0, Enumeration({
        {u"COMMON", DTAPI_DVBT2_PLP_TYPE_COMM},
        {u"1",      DTAPI_DVBT2_PLP_TYPE_1},
        {u"2",      DTAPI_DVBT2_PLP_TYPE_2},
    }));
    help(u"plp0-type",
         u"DVB-T2 modulators: indicate the PLP type for PLP #0. The default is COMMON.");

    option(u"qam-b", 'q', Enumeration({
        {u"I128-J1D", DTAPI_MOD_QAMB_I128_J1D},
        {u"I64-J2",   DTAPI_MOD_QAMB_I64_J2},
        {u"I32-J4",   DTAPI_MOD_QAMB_I32_J4},
        {u"I16-J8",   DTAPI_MOD_QAMB_I16_J8},
        {u"I8-J16",   DTAPI_MOD_QAMB_I8_J16},
        {u"I128-J1",  DTAPI_MOD_QAMB_I128_J1},
        {u"I128-J2",  DTAPI_MOD_QAMB_I128_J2},
        {u"I128-J3",  DTAPI_MOD_QAMB_I128_J3},
        {u"I128-J4",  DTAPI_MOD_QAMB_I128_J4},
        {u"I128-J5",  DTAPI_MOD_QAMB_I128_J5},
        {u"I128-J6",  DTAPI_MOD_QAMB_I128_J6},
        {u"I128-J7",  DTAPI_MOD_QAMB_I128_J7},
        {u"I128-J8",  DTAPI_MOD_QAMB_I128_J8},
    }));
    help(u"qam-b",
         u"QAM modulators: with --j83 B, indicate the QAM-B interleaver mode. "
         u"The default is I128-J1D. ");

    option(u"s2-gold-code", 0, INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    help(u"s2-gold-code",
         u"DVB-S2 modulators: indicate the physical layer scrambling initialization "
         u"sequence, aka \"gold code\".");

    option(u"s2-short-fec-frame");
    help(u"s2-short-fec-frame",
         u"DVB-S2 modulators: use short FEC frames, 16 200 bits (default: long FEC "
         u"frames, 64 800 bits).");

    option(u"satellite-frequency", 0, POSITIVE);
    help(u"satellite-frequency",
         u"DVB-S/S2 modulators: indicate the target satellite frequency, in Hz, of "
         u"the output carrier. The actual frequency at the output of the modulator "
         u"is the \"intermediate\" frequency which is computed based on the "
         u"characteristics of the LNB (see option --lnb). This option is useful "
         u"when the satellite frequency is better known than the intermediate "
         u"frequency. The options --frequency and --satellite-frequency are mutually "
         u"exclusive.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Automatically generate stuffing packets if we fail to provide "
         u"packets fast enough.");

    option(u"symbol-rate", 0, POSITIVE);
    help(u"symbol-rate",
         u"DVB-C/S/S2 modulators: Specify the symbol rate in symbols/second. "
         u"By default, the symbol rate is implicitely computed from the convolutional "
         u"rate, the modulation type and the bitrate. But when --symbol-rate is "
         u"specified, the input bitrate is ignored and the output bitrate is forced "
         u"to the value resulting from the combination of the specified symbol rate, "
         u"convolutional rate and modulation type. "
         u"The options --symbol-rate and --bitrate are mutually exclusive.");

    option(u"t2-fpsf", 0, INTEGER, 0, 1, 1, 255);
    help(u"t2-fpsf",
         u"DVB-T2 modulators: indicate the number of T2 frames per super-frame. "
         u"Must be in the range 1 to 255. The default is 2.");

    option(u"t2-guard-interval", 0, Enumeration({
        {u"1/128", DTAPI_DVBT2_GI_1_128},
        {u"1/32", DTAPI_DVBT2_GI_1_32},
        {u"1/16", DTAPI_DVBT2_GI_1_16},
        {u"19/256", DTAPI_DVBT2_GI_19_256},
        {u"1/8", DTAPI_DVBT2_GI_1_8},
        {u"19/128", DTAPI_DVBT2_GI_19_128},
        {u"1/4", DTAPI_DVBT2_GI_1_4},
    }));
    help(u"t2-guard-interval",
         u"DVB-T2 modulators: indicates the guard interval. The default is 1/128.");

    option(u"t2-l1-modulation", 0, Enumeration({
        {u"BPSK",   DTAPI_DVBT2_BPSK},
        {u"QPSK",   DTAPI_DVBT2_QPSK},
        {u"16-QAM", DTAPI_DVBT2_QAM16},
        {u"64-QAM", DTAPI_DVBT2_QAM64},
    }));
    help(u"t2-l1-modulation",
         u"DVB-T2 modulators: indicate the modulation type used for the L1-post "
         u"signalling block. The default is 16-QAM.");

    option(u"t2-network-id", 0, UINT32);
    help(u"t2-network-id",
         u"DVB-T2 modulators: indicate the DVB-T2 network identification. "
         u"The default is 0.");

    option(u"t2-system-id", 0, UINT32);
    help(u"t2-system-id",
         u"DVB-T2 modulators: indicate the DVB-T2 system identification. "
         u"The default is 0.");

    option(u"time-slice");
    help(u"time-slice",
         u"DVB-T/H modulators: indicate that at least one elementary stream uses "
         u"time slicing (DVB-H signalling).");

    option(u"transmission-mode", 't', Enumeration({
        {u"2K", DTAPI_MOD_DVBT_2K},
        {u"4K", DTAPI_MOD_DVBT_4K},
        {u"8K", DTAPI_MOD_DVBT_8K},
    }));
    help(u"transmission-mode",
         u"DVB-T modulators: indicate the transmission mode. The default is 8K.");

    option(u"uhf-channel", 'u', INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
    help(u"uhf-channel",
         u"UHF modulators: indicate the UHF channel number of the output carrier. "
         u"Can be used in replacement to --frequency. Can be combined with an "
         u"--offset-count option. The resulting frequency is "
         u"306 MHz + (uhf-channel * 8 MHz) + (offset-count * 166.6 kHz).");

    option(u"vhf-channel", 'v', INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);
    help(u"vhf-channel",
         u"VHF modulators: indicate the VHF channel number of the output carrier. "
         u"Can be used in replacement to --frequency. Can be combined with an "
         u"--offset-count option. The resulting frequency is "
         u"142.5 MHz + (vhf-channel * 7 MHz) + (offset-count * 166.6 kHz).");

    option(u"vsb", 0, Enumeration({
        {u"8",  DTAPI_MOD_ATSC_VSB8},
        {u"16", DTAPI_MOD_ATSC_VSB16},
    }));
    help(u"vsb",
         u"ATSC modulators: indicate the VSB constellation. Must be one of "
         u"8 (19,392,658 Mb/s) or 16 (38,785,317 Mb/s). The default is 8.");

    option(u"vsb-taps", 0, INTEGER, 0, 1, 2, 256);
    help(u"vsb-taps",
         u"ATSC modulators: indicate the number of taps of each phase of the "
         u"root-raised cosine filter that is used to shape the spectrum of the "
         u"output signal. The number of taps can have any value between 2 and 256 "
         u"(the implementation is optimized for powers of 2). Specifying more taps "
         u"improves the spectrum, but increases processor overhead. The recommend "
         u"(and default) number of taps is 64 taps. If insufficient CPU power is "
         u"available, 32 taps produces acceptable results, too. ");
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::start()
{
    if (_guts->is_started) {
        tsp->error(u"already started");
        return false;
    }

    // Get command line arguments
    _guts->dev_index = intValue<int>(u"device", -1);
    _guts->chan_index = intValue<int>(u"channel", -1);
    _guts->opt_bitrate = intValue<BitRate>(u"bitrate", 0);
    _guts->detach_mode = present(u"instant-detach") ? DTAPI_INSTANT_DETACH : DTAPI_WAIT_UNTIL_SENT;
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
        tsp->error(u"error attaching output Dektec device %d (%s): %s", {_guts->dev_index, _guts->device.model, DektecStrError(status)});
        return false;
    }

    // Open the channel
    status = _guts->chan.AttachToPort(&_guts->dtdev, _guts->device.output[_guts->chan_index].m_Port);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching output channel %d of Dektec device %d (%s): %s", {_guts->chan_index, _guts->dev_index, _guts->device.model, DektecStrError(status)});
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
        return startError(u"output device reset error", status);
    }

    // Set 188/204-byte output packet format and stuffing
    status = _guts->chan.SetTxMode(present(u"204") ? DTAPI_TXMODE_ADD16 : DTAPI_TXMODE_188, present(u"stuffing") ? 1 : 0);
    if (status != DTAPI_OK) {
        return startError(u"output device SetTxMode error", status);
    }

    // Set modulation parameters for modulators
    if (is_modulator && !setModulation(modulation_type)) {
        return false;
    }

    // Set output level.
    if (present(u"level")) {
        status = _guts->chan.SetOutputLevel(intValue<int>(u"level"));
        if (status != DTAPI_OK) {
            // In case of error, report it but do not fail.
            // This feature is not supported on all modulators and
            // it seems severe to fail if unsupported.
            tsp->error(u"set modulator output level: " + DektecStrError(status));
        }
    }

    // Get max FIFO size.
    _guts->max_fifo_size = 0;
    status = _guts->chan.GetFifoSizeMax(_guts->max_fifo_size);
    if (status != DTAPI_OK || _guts->max_fifo_size == 0) {
        // Not supported on this device, use hard-coded value.
        _guts->max_fifo_size = int(DTA_FIFO_SIZE);
        tsp->verbose(u"max fifo size not supported, using %'d bytes", {_guts->max_fifo_size});
    }

    // Get typical FIFO size, for information only, ignore errors
    int typ_fifo_size = 0;
    _guts->chan.GetFifoSizeTyp(typ_fifo_size);

    // Set channel FIFO size.
    if (present(u"fifo-size")) {
        // Get the requested FIFO size value. Round it downward to a multiple of 16.
        // Limit the value to the maximum FIFO size of the device.
        const int size = std::min(intValue<int>(u"fifo-size"), _guts->max_fifo_size) & ~0x0F;
        if (size > 0) {
            tsp->verbose(u"setting output fifo size to %'d bytes", {size});
            status = _guts->chan.SetFifoSize(size);
            if (status != DTAPI_OK) {
                return startError(u"error setting FIFO size", status);
            }
        }
    }

    // Get current FIFO size.
    _guts->fifo_size = 0;
    status = _guts->chan.GetFifoSize(_guts->fifo_size);
    if (status != DTAPI_OK) {
        return startError(u"error getting FIFO size", status);
    }
    tsp->verbose(u"output fifo size: %'d bytes, max: %'d bytes, typical: %'d bytes", {_guts->fifo_size, _guts->max_fifo_size, typ_fifo_size});

    // Set output bitrate
    status = _guts->chan.SetTsRateBps(int(_guts->cur_bitrate));
    if (status != DTAPI_OK) {
        return startError(u"output device set bitrate error", status);
    }

    // Start the transmission on the output device.
    // With ASI device, we can start transmission right now.
    // With modulator devices, we need to load the FIFO first.
    _guts->starting = is_modulator;
    status = _guts->chan.SetTxControl(_guts->starting ? DTAPI_TXCTRL_HOLD : DTAPI_TXCTRL_SEND);
    if (status != DTAPI_OK) {
        return startError(u"output device start send error", status);
    }

    tsp->verbose(u"initial output bitrate: %'d b/s", {_guts->cur_bitrate});
    _guts->is_started = true;
    return true;
}


//----------------------------------------------------------------------------
// Output start error method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::startError(const UString& message, unsigned int status)
{
    if (status == DTAPI_OK) {
        tsp->error(message);
    }
    else {
        tsp->error(message + u": " + DektecStrError(status));
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
        return startError(u"Error computing bitrate from symbol rate", status);
    }
    else {
        tsp->verbose(u"setting output TS bitrate to %'d b/s", {bitrate});
        _guts->opt_bitrate = _guts->cur_bitrate = BitRate(bitrate);
        return true;
    }
}


//----------------------------------------------------------------------------
// Compute and display symbol rate if not explicitly specified by the user.
//----------------------------------------------------------------------------

void ts::DektecOutputPlugin::displaySymbolRate(int ts_bitrate, int dt_modulation, int param0, int param1, int param2)
{
    if (ts_bitrate > 0) {
        int symrate = -1;
        Dtapi::DTAPI_RESULT status = Dtapi::DtapiModPars2SymRate(symrate, dt_modulation, param0, param1, param2, ts_bitrate);
        if (status != DTAPI_OK) {
            tsp->verbose(u"error computing symbol rate: ", {DektecStrError(status)});
        }
        else {
            tsp->verbose(u"output symbol rate: %'d symbols/second", {symrate});
        }
    }
}


//----------------------------------------------------------------------------
// Set modulation parameters (modulators only).
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setModulation(int& modulation_type)
{
    // Get input plugin modulation parameters if required
    const bool use_input_modulation = present(u"input-modulation");
    const ObjectPtr input_params(use_input_modulation ? Object::RetrieveFromRepository(u"tsp.dvb.params") : nullptr);

    // Various views of the input modulation parameters (at most one is non-zero)
    const TunerParameters*     input_dvb  = dynamic_cast <const TunerParameters*>(input_params.pointer());
    const TunerParametersDVBS* input_dvbs = dynamic_cast <const TunerParametersDVBS*>(input_dvb);
    const TunerParametersDVBC* input_dvbc = dynamic_cast <const TunerParametersDVBC*>(input_dvb);
    const TunerParametersDVBT* input_dvbt = dynamic_cast <const TunerParametersDVBT*>(input_dvb);
    const TunerParametersATSC* input_atsc = dynamic_cast <const TunerParametersATSC*>(input_dvb);

    // Adjust default modulation type from input plugin
    if (input_dvb != nullptr) {
        tsp->debug(u"found input modulator parameters: %s %s", {TunerTypeEnum.name(input_dvb->tunerType()), input_dvb->toPluginOptions()});
        if (input_dvbs != nullptr) {
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
        else if (input_dvbc != nullptr) {
            switch (input_dvbc->modulation) {
                case QAM_16:  modulation_type = DTAPI_MOD_QAM16;  break;
                case QAM_32:  modulation_type = DTAPI_MOD_QAM32;  break;
                case QAM_64:  modulation_type = DTAPI_MOD_QAM64;  break;
                case QAM_128: modulation_type = DTAPI_MOD_QAM128; break;
                case QAM_256: modulation_type = DTAPI_MOD_QAM256; break;
                default: break;
            }
        }
        else if (input_dvbt != nullptr) {
            modulation_type = DTAPI_MOD_DVBT;
        }
        else if (input_atsc != nullptr) {
            modulation_type = DTAPI_MOD_ATSC;
        }
    }

    // Get user-specified modulation
    modulation_type = intValue<int>(u"modulation", modulation_type);
    if (modulation_type < 0) {
        return startError(u"unspecified modulation type for " + _guts->device.model, DTAPI_OK);
    }

    // Get user-specified symbol rate, used only with DVB-S/S2/C.
    int symbol_rate = intValue<int>(u"symbol-rate", -1);
    if (present(u"bitrate") && present(u"symbol-rate")) {
        return startError(u"options --symbol-rate and --bitrate are mutually exclusive", DTAPI_OK);
    }

    // Get LNB description, in case --satellite-frequency is used
    LNB lnb; // Universal LNB by default
    if (present(u"lnb")) {
        const UString s(value(u"lnb"));
        LNB l(s);
        if (!l.isValid()) {
            return startError(u"invalid LNB description " + s, DTAPI_OK);
        }
        else {
            lnb = l;
        }
    }

    // Compute carrier frequency
    uint64_t frequency = 0;
    if (present(u"frequency") + present(u"satellite-frequency") + present(u"uhf-channel") + present(u"vhf-channel") > 1) {
        return startError(u"options --frequency, --satellite-frequency, --uhf-channel, --vhf-channel are mutually exclusive", DTAPI_OK);
    }
    if (present(u"uhf-channel")) {
        frequency = UHF::Frequency(intValue<int>(u"uhf-channel", 0), intValue<int>(u"offset-count", 0));
    }
    else if (present(u"vhf-channel")) {
        frequency = VHF::Frequency(intValue<int>(u"vhf-channel", 0), intValue<int>(u"offset-count", 0));
    }
    else if (present(u"satellite-frequency")) {
        uint64_t sat_frequency = intValue<uint64_t>(u"satellite-frequency", 0);
        if (sat_frequency > 0) {
            frequency = lnb.intermediateFrequency(sat_frequency);
        }
    }
    else if (present(u"frequency")) {
        frequency = intValue<uint64_t>(u"frequency", 0);
    }
    else if (input_dvbs != nullptr) {
        frequency = input_dvbs->frequency;
    }
    else if (input_dvbt != nullptr) {
        frequency = input_dvbt->frequency;
    }
    else if (input_dvbc != nullptr) {
        frequency = input_dvbc->frequency;
    }
    else if (input_atsc != nullptr) {
        frequency = input_atsc->frequency;
    }
    if (frequency == 0) {
        return startError(u"unspecified frequency (required for modulator devices)", DTAPI_OK);
    }

    // Set modulation parameters
    Dtapi::DTAPI_RESULT status = DTAPI_OK;
    switch (modulation_type) {

        case DTAPI_MOD_DVBS_QPSK:
        case DTAPI_MOD_DVBS_BPSK: {
            // Various types of DVB-S
            int fec = DTAPI_MOD_3_4;
            if (input_dvbs != nullptr) {
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
            fec = intValue<int>(u"convolutional-rate", fec);
            tsp->verbose(u"using DVB-S FEC " + DektecFEC.name(fec));
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate <= 0) {
                displaySymbolRate(_guts->opt_bitrate, modulation_type, fec, 0, 0);
            }
            else if (!setBitrate(symbol_rate, modulation_type, fec, 0, 0)) {
                return false;
            }
            status = _guts->chan.SetModControl(modulation_type, fec, 0, 0);
            break;
        }

        case DTAPI_MOD_DVBS2_QPSK:
        case DTAPI_MOD_DVBS2_8PSK:
        case DTAPI_MOD_DVBS2_16APSK:
        case DTAPI_MOD_DVBS2_32APSK: {
            // Various types of DVB-S2
            int fec = DTAPI_MOD_3_4;
            int pilots = present(u"pilots") ? DTAPI_MOD_S2_PILOTS : DTAPI_MOD_S2_NOPILOTS;
            if (input_dvbs != nullptr) {
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
            fec = intValue<int>(u"convolutional-rate", fec);
            const int fec_frame = present(u"s2-short-fec-frame") ? DTAPI_MOD_S2_SHORTFRM : DTAPI_MOD_S2_LONGFRM;
            const int gold_code = intValue<int>(u"s2-gold-code", 0);
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate <= 0) {
                displaySymbolRate(_guts->opt_bitrate, modulation_type, fec, pilots | fec_frame, gold_code);
            }
            else if (!setBitrate(symbol_rate, modulation_type, fec, pilots | fec_frame, gold_code)) {
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
            const int j83 = intValue<int>(u"j83", DTAPI_MOD_J83_A);
            const int qam_b = j83 != DTAPI_MOD_J83_B ? 0 : intValue<int>(u"qam-b", DTAPI_MOD_QAMB_I128_J1D);
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate <= 0) {
                displaySymbolRate(_guts->opt_bitrate, modulation_type, j83, qam_b, 0);
            }
            else if (!setBitrate(symbol_rate, modulation_type, j83, qam_b, 0)) {
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
            if (use_input_modulation && input_dvbt == nullptr && _guts->cur_bitrate > 0) {
                // --input-modulation is specified but input plugin is not a DVB-T tuner,
                // use input bitrate to determine modulation parameters.
                TunerParametersBitrateDiffDVBTList params_list;
                TunerParametersBitrateDiffDVBT::EvaluateToBitrate(params_list, _guts->cur_bitrate);
                if (!params_list.empty()) {
                    params = params_list.front();
                    input_dvbt = &params;
                }
            }
            if (input_dvbt != nullptr) {
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
            fec = intValue<int>(u"convolutional-rate", fec);
            bw = intValue<int>(u"bandwidth", bw);
            constel = intValue<int>(u"constellation", constel);
            guard = intValue<int>(u"guard-interval", guard);
            tr_mode = intValue<int>(u"transmission-mode", tr_mode);
            const int interleave = present(u"indepth-interleave") ? DTAPI_MOD_DVBT_INDEPTH : DTAPI_MOD_DVBT_NATIVE;
            const bool time_slice = present(u"time-slice");
            const bool mpe_fec = present(u"mpe-fec");
            const int dvb_h = time_slice || mpe_fec ? DTAPI_MOD_DVBT_ENA4849 : DTAPI_MOD_DVBT_DIS4849;
            const int s48 = time_slice ? DTAPI_MOD_DVBT_S48 : DTAPI_MOD_DVBT_S48_OFF;
            const int s49 = mpe_fec ? DTAPI_MOD_DVBT_S49 : DTAPI_MOD_DVBT_S49_OFF;
            const int cell_id = intValue<int>(u"cell-id", -1);
            tsp->verbose(u"using DVB-T FEC %s, bandwidth %s, constellation %s, guard %s, transmission %s",
                         {DektecFEC.name(fec),
                          DektecDVBTProperty.name(bw),
                          DektecDVBTProperty.name(constel),
                          DektecDVBTProperty.name(guard),
                          DektecDVBTProperty.name(tr_mode)});
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
            pars.m_Bandwidth = intValue<int>(u"bandwidth", DTAPI_DVBT2_8MHZ);
            pars.m_FftMode = intValue<int>(u"fft-mode", DTAPI_DVBT2_FFT_32K);
            pars.m_Miso = intValue<int>(u"miso", DTAPI_DVBT2_MISO_OFF);
            pars.m_GuardInterval = intValue<int>(u"t2-guard-interval", DTAPI_DVBT2_GI_1_128);
            pars.m_Papr = intValue<int>(u"papr", DTAPI_DVBT2_PAPR_NONE);
            pars.m_BwtExt = present(u"bandwidth-extension") ? DTAPI_DVBT2_BWTEXT_ON : DTAPI_DVBT2_BWTEXT_OFF;
            pars.m_PilotPattern = intValue<int>(u"pilot-pattern", DTAPI_DVBT2_PP_7);
            pars.m_NumT2Frames = intValue<int>(u"t2-fpsf", 2);
            pars.m_L1Modulation = intValue<int>(u"t2-l1-modulation", DTAPI_DVBT2_QAM16);
            pars.m_FefEnable = present(u"fef");
            pars.m_FefType = intValue<int>(u"fef-type", 0);
            pars.m_FefLength = intValue<int>(u"fef-length", 1);
            pars.m_FefS1 = intValue<int>(u"fef-s1", 2);
            pars.m_FefS2 = intValue<int>(u"fef-s2", 1);
            pars.m_FefInterval = intValue<int>(u"fef-interval", 1);
            pars.m_FefSignal = intValue<int>(u"fef-signal", DTAPI_DVBT2_FEF_ZERO);
            pars.m_CellId = intValue<int>(u"cell-id", 0);
            pars.m_NetworkId = intValue<int>(u"t2-network-id", 0);
            pars.m_T2SystemId = intValue<int>(u"t2-system-id", 0);
            // Obsolete field in DTAPI 4.10.0.145:
            // pars.m_Frequency = int(frequency);
            pars.m_NumPlps = 1; // This version supports single-PLP only
            pars.m_Plps[0].Init(); // default values
            pars.m_Plps[0].m_Hem = present(u"plp0-high-efficiency");
            pars.m_Plps[0].m_Npd = present(u"plp0-null-packet-deletion");
            pars.m_Plps[0].m_Issy = intValue<int>(u"plp0-issy", DTAPI_DVBT2_ISSY_NONE);
            pars.m_Plps[0].m_Id = intValue<int>(u"plp0-id", 0);
            pars.m_Plps[0].m_GroupId = intValue<int>(u"plp0-group-id", 0);
            pars.m_Plps[0].m_Type = intValue<int>(u"plp0-type", DTAPI_DVBT2_PLP_TYPE_COMM);
            pars.m_Plps[0].m_CodeRate = intValue<int>(u"plp0-code-rate", DTAPI_DVBT2_COD_2_3);
            pars.m_Plps[0].m_Modulation = intValue<int>(u"plp0-modulation", DTAPI_DVBT2_QAM256);
            pars.m_Plps[0].m_Rotation = present(u"plp0-rotation");
            pars.m_Plps[0].m_FecType = intValue<int>(u"plp0-fec-type", DTAPI_DVBT2_LDPC_64K);
            pars.m_Plps[0].m_TimeIlLength = intValue<int>(u"plp0-il-length", 3);
            pars.m_Plps[0].m_TimeIlType = intValue<int>(u"plp0-il-type", DTAPI_DVBT2_IL_ONETOONE);
            pars.m_Plps[0].m_InBandAFlag = present(u"plp0-in-band");
            // Compute other fields
            Dtapi::DtDvbT2ParamInfo info;
            status = pars.OptimisePlpNumBlocks(info, pars.m_Plps[0].m_NumBlocks, pars.m_NumDataSyms);
            if (status != DTAPI_OK) {
                return startError(u"error computing PLP parameters", status);
            }
            // Report actual parameters in debug mode
            tsp->debug(u"DVB-T2: DtDvbT2Pars = {");
            DektecDevice::ReportDvbT2Pars(pars, *tsp, Severity::Debug, u"");
            tsp->debug(u"}");
            tsp->debug(u"DVB-T2: DtDvbT2ParamInfo = {");
            DektecDevice::ReportDvbT2ParamInfo(info, *tsp, Severity::Debug, u"  ");
            tsp->debug(u"}");
            // Check validity of T2 parameters
            status = pars.CheckValidity();
            if (status != DTAPI_OK) {
                return startError(u"invalid combination of DVB-T2 parameters", status);
            }
            // Set modulation parameters
            status = _guts->chan.SetModControl(pars);
            break;
        }

        case DTAPI_MOD_ATSC: {
            int constel = DTAPI_MOD_ATSC_VSB8;
            if (input_atsc != nullptr) {
                switch (input_atsc->modulation) {
                    case VSB_8:  constel = DTAPI_MOD_ATSC_VSB8;  break;
                    case VSB_16: constel = DTAPI_MOD_ATSC_VSB16; break;
                    default: break;
                }
            }
            constel = intValue<int>(u"vsb", constel);
            const int taps = intValue<int>(u"vsb-taps", 64);
            tsp->verbose(u"using ATSC " + DektecVSB.name(constel));
            status = _guts->chan.SetModControl(modulation_type, constel, taps, 0);
            break;
        }

        case DTAPI_MOD_ADTBT:
        case DTAPI_MOD_DMBTH: {
            const int bw = intValue<int>(u"bandwidth", DTAPI_MOD_DTMB_8MHZ);
            const int constel = intValue<int>(u"dmb-constellation", DTAPI_MOD_DTMB_QAM64);
            const int fec = intValue<int>(u"dmb-fec", DTAPI_MOD_DTMB_0_8);
            const int header = intValue<int>(u"dmb-header", DTAPI_MOD_DTMB_PN945);
            const int interleaver = intValue<int>(u"dmb-interleaver", DTAPI_MOD_DTMB_IL_1);
            const int pilots = present(u"pilots") ? DTAPI_MOD_DTMB_PILOTS : DTAPI_MOD_DTMB_NO_PILOTS;
            const int frame_num = present(u"dmb-frame-numbering") ? DTAPI_MOD_DTMB_USE_FRM_NO : DTAPI_MOD_DTMB_NO_FRM_NO;
            status = _guts->chan.SetModControl(modulation_type, bw | constel | fec | header | interleaver | pilots | frame_num, 0, 0);
            break;
        }

        case DTAPI_MOD_CMMB: {
            if (_guts->cur_bitrate <= 0) {
                return startError(u"unknown bitrate, required with CMMB modulation, use --bitrate option", DTAPI_OK);
            }
            if (!present(u"cmmb-pid")) {
                return startError(u"option --cmmb-pid is required with CMMB modulation", DTAPI_OK);
            }
            Dtapi::DtCmmbPars pars;
            pars.m_Bandwidth = intValue<int>(u"cmmb-bandwidth", DTAPI_CMMB_BW_8MHZ);
            pars.m_TsRate = int(_guts->cur_bitrate);
            pars.m_TsPid = intValue<int>(u"cmmb-pid", 0);
            pars.m_AreaId = intValue<int>(u"cmmb-area-id", 0);
            pars.m_TxId = intValue<int>(u"cmmb-transmitter-id", 0);
            status = _guts->chan.SetModControl(pars);
            break;
        }

        case DTAPI_MOD_ISDBT: {
            return startError(u"ISDB-T modulation not yet supported", DTAPI_OK);
            break;
        }

        case -1: {
            // No modulation specified
            status = DTAPI_OK;
            break;
        }

        default: {
            return startError(u"unsupported modulation type", DTAPI_OK);
        }
    }

    if (status != DTAPI_OK) {
        return startError(u"error while setting modulation mode", status);
    }

    // Set carrier frequency.
    // Make sure to use "__int64" and not "int64_t" in SetRfControl to avoid
    // ambiguous overloading.
    tsp->verbose(u"setting output carrier frequency to %'d Hz", {frequency});
    status = _guts->chan.SetRfControl(__int64(frequency));
    if (status != DTAPI_OK) {
        return startError(u"set modulator frequency error", status);
    }
    status = _guts->chan.SetRfMode(DTAPI_UPCONV_NORMAL | (present(u"inversion") ? DTAPI_UPCONV_SPECINV : 0));
    if (status != DTAPI_OK) {
        return startError(u"set modulator RF mode", status);
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
        tsp->verbose(u"terminating %s output", {_guts->device.model});

        // Mute output signal for modulators which support this
        if (_guts->mute_on_stop) {
            Dtapi::DTAPI_RESULT status = _guts->chan.SetRfMode(DTAPI_UPCONV_MUTE);
            if (status != DTAPI_OK) {
                tsp->error(u"error muting modulator output: " + DektecStrError(status));
            }
        }

        // Detach the channel and the device
        _guts->chan.Detach(_guts->detach_mode);
        _guts->dtdev.Detach();

        _guts->is_started = false;
        tsp->verbose(u"%s output terminated", {_guts->device.model});
    }
    return true;
}


//----------------------------------------------------------------------------
// Output destructor
//----------------------------------------------------------------------------

ts::DektecOutputPlugin::~DektecOutputPlugin()
{
    if (_guts != nullptr) {
        stop();
        delete _guts;
        _guts = nullptr;
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
            tsp->error(u"error getting Dektec device output bitrate: " + DektecStrError(status));
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

    char* data = reinterpret_cast<char*>(const_cast<TSPacket*>(buffer));
    int remain = int(packet_count * PKT_SIZE);
    Dtapi::DTAPI_RESULT status;

    // If no bitrate was specified on the command line, adjust the bitrate
    // when input bitrate changes.
    BitRate new_bitrate;
    if (_guts->opt_bitrate == 0 && _guts->cur_bitrate != (new_bitrate = tsp->bitrate())) {
        status = _guts->chan.SetTsRateBps(int(new_bitrate));
        if (status != DTAPI_OK) {
            tsp->error(u"error setting output bitrate on Dektec device: " + DektecStrError(status));
        }
        else {
            _guts->cur_bitrate = new_bitrate;
            tsp->verbose(u"new output bitrate: %'d b/s", {_guts->cur_bitrate});
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
                tsp->error(u"error getting output fifo load: " + DektecStrError(status));
                return false;
            }

            // We consider the FIFO is loaded when 80% full.
            const int max_size = (8 * _guts->fifo_size) / 10;
            if (fifo_load < max_size - int(PKT_SIZE)) {
                // Remain in starting phase, limit next I/O size
                max_io_size = max_size - fifo_load;
            }
            else {
                // FIFO now full enough to start transmitting
                tsp->verbose(u"%s output FIFO load is %'d bytes, starting transmission", {_guts->device.model, fifo_load});
                status = _guts->chan.SetTxControl(DTAPI_TXCTRL_SEND);
                if (status != DTAPI_OK) {
                    tsp->error(u"output device start send error: " + DektecStrError(status));
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
            tsp->error(u"transmission error on Dektec device: " + DektecStrError(status));
            return false;
        }

        data += cursize;
        remain -= cursize;
    }

    return true;
}

#endif // TS_NO_DTAPI
