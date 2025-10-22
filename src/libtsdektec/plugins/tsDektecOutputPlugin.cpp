//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDektecOutputPlugin.h"
#include "tsObjectRepository.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsDektecArgsUtils.h"
#include "tsHFBand.h"
#include "tsDVBT2ParamsEvaluator.h"
#include "tsModulation.h"
#include "tsIntegerUtils.h"
#include "tsFatal.h"

#define DEFAULT_PRELOAD_FIFO_PERCENTAGE 80
#define DEFAULT_MAINTAIN_PRELOAD_THRESHOLD_SIZE 20116 // a little over 20k in packets, byte size for exactly 107 packets


//----------------------------------------------------------------------------
// Class internals.
//----------------------------------------------------------------------------

class ts::DektecOutputPlugin::Guts
{
    TS_NOCOPY(Guts);
public:
    Guts() = default;                               // Constructor
    bool                 starting = false;          // Starting phase (loading FIFO, no transmit)
    bool                 is_started = false;        // Device started
    bool                 mute_on_stop = false;      // Device supports output muting
    int                  dev_index = -1;            // Dektec device index
    int                  chan_index = -1;           // Device output channel index
    DektecDevice         device {};                 // Device characteristics
    Dtapi::DtDevice      dtdev {};                  // Device descriptor
    Dtapi::DtOutpChannel chan {};                   // Output channel
    int                  detach_mode = 0;           // Detach mode
    int                  iostd_value = -1;          // Value parameter for SetIoConfig on I/O standard.
    int                  iostd_subvalue = -1;       // SubValue parameter for SetIoConfig on I/O standard.
    BitRate              opt_bitrate = 0;           // Bitrate option (0 means unspecified)
    BitRate              cur_bitrate = 0;           // Current output bitrate
    int                  max_fifo_size = 0;         // Maximum FIFO size
    int                  fifo_size = 0;             // Actual FIFO size
    bool                 preload_fifo = false;      // Preload FIFO before starting transmission
    int                  preload_fifo_size = 0;     // Size of FIFO to preload before starting transmission
    uint64_t             preload_fifo_delay = 0;    // Preload FIFO such that it starts transmission after specified delay in ms
    bool                 maintain_preload = false;  // Roughly maintain the buffer size if the FIFO is preloaded prior to starting transmission
    bool                 drop_to_maintain = false;  // Drop packets as necessary to maintain preload
    int                  maintain_threshold = 0;    // Threshold in FIFO beyond preload_fifo_size before it starts dropping packets if drop_to_maintain enabled
    bool                 drop_to_preload = false;   // Drop sufficient packets to get back to preload FIFO size--only set to true at run-time if would exceed preload plus threshold
    bool                 carrier_only = false;      // Output carrier frequency only, no modulated TS
    int                  power_mode = -1;           // Power mode to set on DTU-315
};


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::isRealTime()
{
    return true;
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
    DefineDektecIOStandardArgs(*this);
    DefineDektecIPArgs(*this, false); // false = transmit

    option(u"204");
    help(u"204",
         u"ASI devices: Send 204-byte packets (188 meaningful bytes plus 16 "
         u"stuffing bytes for RS coding). By default, send 188-byte packets.");

    option(u"bandwidth", 0, Names({
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

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specify output bitrate in bits/second. By default, use the input "
         u"device bitrate or, if the input device cannot report bitrate, analyze "
         u"some PCR's at the beginning of the input stream to evaluate the "
         u"original bitrate of the transport stream.");

    option(u"carrier-only");
    help(u"carrier-only",
         u"Modulators: output the carrier only, without modulated transport stream. "
         u"All output packets are dropped. "
         u"To generate an empty carrier and wait forever, use the following sample command:\n"
         u"tsp --final-wait 0 -I null 1 -O dektec --carrier-only --frequency ...");

    option(u"cell-id", 0,  UINT16);
    help(u"cell-id",
         u"DVB-T and DVB-T2 modulators: indicate the cell identifier to set in the "
         u"transmission parameters signaling (TPS). Disabled by default with DVB-T. "
         u"Default value is 0 with DVB-T2.");

    option(u"channel", 'c', UNSIGNED);
    help(u"channel",
         u"Channel index on the output Dektec device. By default, use the "
         u"first output channel on the device.");

    option(u"constellation", 0, Names({
        {u"QPSK",   DTAPI_MOD_DVBT_QPSK},
        {u"16-QAM", DTAPI_MOD_DVBT_QAM16},
        {u"64-QAM", DTAPI_MOD_DVBT_QAM64},
    }));
    help(u"constellation",
         u"DVB-T modulators: indicate the constellation type. The default is 64-QAM.");

    option(u"convolutional-rate", 'r', Names({
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

    option(u"dmb-constellation", 0, Names({
        {u"4-QAM-NR", DTAPI_MOD_DTMB_QAM4NR},
        {u"4-QAM",    DTAPI_MOD_DTMB_QAM4},
        {u"16-QAM",   DTAPI_MOD_DTMB_QAM16},
        {u"32-QAM",   DTAPI_MOD_DTMB_QAM32},
        {u"64-QAM",   DTAPI_MOD_DTMB_QAM64},
    }));
    help(u"dmb-constellation",
         u"DMB-T/H, ADTB-T modulators: indicate the constellation type. The default is 64-QAM. "
         u"4-QAM-NR and 32-QAM can be used only with --dmb-fec 0.8.");

    option(u"dmb-fec", 0, Names({
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

    option(u"dmb-header", 0, Names({
        {u"PN420", DTAPI_MOD_DTMB_PN420},
        {u"PN595", DTAPI_MOD_DTMB_PN595},
        {u"PN945", DTAPI_MOD_DTMB_PN945},
    }));
    help(u"dmb-header",
         u"DMB-T/H, ADTB-T modulators: indicate the FEC frame header mode. "
         u"The default is PN945.");

    option(u"dmb-interleaver", 0, Names({
        {u"1", DTAPI_MOD_DTMB_IL_1},
        {u"2", DTAPI_MOD_DTMB_IL_2},
    }));
    help(u"dmb-interleaver",
         u"DMB-T/H, ADTB-T modulators: indicate the interleaver mode. Must be one "
         u"1 (B=54, M=240) or 2 (B=54, M=720). The default is 1.");

    option(u"drop-to-maintain-preload");
    help(u"drop-to-maintain-preload",
        u"If the FIFO were preloaded, and maintaining the preload via option "
        u"--maintain-preload, drop any packets that would exceed the preload "
        u"FIFO size plus a small threshold.");

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

    option(u"fef-signal", 0, Names({
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

    option(u"fft-mode", 0, Names({
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

    option(u"guard-interval", 'g', Names({
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
         u"Otherwise, if the specified modulation is DVB-T or DVB-T2, try to guess "
         u"some modulation parameters from the bitrate.");

    option(u"instant-detach");
    help(u"instant-detach",
         u"At end of stream, perform an \"instant detach\" of the output channel. "
         u"The transmit FIFO is immediately cleared without waiting for all data to be transmitted. "
         u"With some Dektec devices, the default mode may hang at end of stream and --instant-detach avoids this. "
         u"The options --instant-detach and --wait-detach are mutually exclusive.");

    option(u"inversion");
    help(u"inversion", u"All modulators devices: enable spectral inversion.");

    option(u"j83", 0, Names({
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

    option(u"lnb", 0, STRING);
    help(u"lnb", u"name",
         u"DVB-S/S2 modulators: description of the LNB which is used to convert the "
         u"--satellite-frequency into an intermediate frequency. This option is "
         u"useless when --satellite-frequency is not specified. "
         u"The specified string is the name (or an alias for that name) "
         u"of a preconfigured LNB in the configuration file tsduck.lnbs.xml. "
         u"For compatibility, the legacy format 'low_freq[,high_freq,switch_freq]' is also accepted "
         u"(all frequencies are in MHz). The default is a universal extended LNB.");

    option(u"maintain-preload");
    help(u"maintain-preload",
         u"If the FIFO were preloaded, roughly maintain the FIFO buffer size in order "
         u"to maintain the delay from real-time. If the FIFO size drops to zero bytes, "
         u"pause transmission till it gets back to the preload FIFO size.");

    option(u"miso", 0, Names({
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

    option(u"modulation", 'm', Names({
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
    }));
    help(u"modulation",
         u"For modulators, indicate the modulation type. "
         u"For DVB-H, specify DVB-T. For DMB-H, specify DMB-T. "
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

    option(u"papr", 0, Names({
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

    option(u"pilot-pattern", 'p', Names({
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

    option(u"plp0-code-rate", 0, Names({
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

    option(u"plp0-fec-type", 0, Names({
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

    option(u"plp0-il-type", 0, Names({
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

    option(u"plp0-issy", 0, Names({
        {u"NONE",  DTAPI_DVBT2_ISSY_NONE},
        {u"SHORT", DTAPI_DVBT2_ISSY_SHORT},
        {u"LONG",  DTAPI_DVBT2_ISSY_LONG},
    }));
    help(u"plp0-issy",
         u"DVB-T2 modulators: type of ISSY field to compute and insert in PLP #0. "
         u"The default is NONE.");

    option(u"plp0-modulation", 0, Names({
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
         u"DVB-T2 modulators: indicate that null-packet deletion is active in PLP #0. Otherwise it is not active.");

    option(u"plp0-rotation");
    help(u"plp0-rotation",
         u"DVB-T2 modulators: indicate that constellation rotation is used for PLP #0. Otherwise not.");

    option(u"plp0-tsrate", 0, UNSIGNED);
    help(u"plp0-tsrate",
         u"DVB-T2 modulators: PLP #0 bitrate. The default is 0 (all available).");

    option(u"plp0-type", 0, Names({
        {u"COMMON", DTAPI_DVBT2_PLP_TYPE_COMM},
        {u"1",      DTAPI_DVBT2_PLP_TYPE_1},
        {u"2",      DTAPI_DVBT2_PLP_TYPE_2},
    }));
    help(u"plp0-type",
         u"DVB-T2 modulators: indicate the PLP type for PLP #0. The default is COMMON.");

    option(u"power-mode", 0, DektecPowerMode());
    help(u"power-mode", u"DTU-315 modulators: set the power mode to the specified value.");

    option(u"preload-fifo");
    help(u"preload-fifo",
         u"Preload FIFO (hardware buffer) before starting transmission. Preloading the FIFO "
         u"will introduce a variable delay to the start of transmission, _if_ the delivery of "
         u"packets to the plug-in is pre-regulated, based on the size of the FIFO, the TS bit "
         u"rate, and the size of the FIFO to preload, as controlled by the "
         u"--preload-fifo-percentage or --preload-fifo-delay options. If the delivery of "
         u"packets to the plug-in isn't self-regulated (i.e. they are delivered faster than "
         u"real-time, as might occur when loading from file), there is no benefit to preloading "
         u"the FIFO, because in that case, the FIFO will fill up quickly anyway. On implicitly "
         u"when using a modulator for output.");

    option(u"preload-fifo-percentage", 0, INTEGER, 0, 1, 1, 100);
    help(u"preload-fifo-percentage",
         u"Percentage of size of FIFO to preload prior to starting transmission "
         u"(default: " + UString::Decimal(DEFAULT_PRELOAD_FIFO_PERCENTAGE) + u"%).");

    option(u"preload-fifo-delay", 0, INTEGER, 0, 1, 100, 100000);
    help(u"preload-fifo-delay",
         u"The use of this option indicates that the size of the FIFO to preload prior to "
         u"starting transmission should be calculated based on the specified delay, in "
         u"milliseconds, and the configured bitrate. That is, transmission will start after "
         u"the specified delay worth of media has been preloaded. This option takes precedence "
         u"over the --preload-fifo-percentage option. There is no default value, and the valid "
         u"range is 100-100000.");

    option(u"qam-b", 'q', Names({
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
         u"The default is I128-J1D.");

    option(u"roll-off", 0, Names({
        {u"auto", DTAPI_MOD_ROLLOFF_AUTO},
        {u"none", DTAPI_MOD_ROLLOFF_NONE},
        {u"0.03", DTAPI_MOD_ROLLOFF_3},
        {u"0.05", DTAPI_MOD_ROLLOFF_5},
        {u"0.10", DTAPI_MOD_ROLLOFF_10},
        {u"0.15", DTAPI_MOD_ROLLOFF_15},
        {u"0.20", DTAPI_MOD_ROLLOFF_20},
        {u"0.25", DTAPI_MOD_ROLLOFF_25},
        {u"0.35", DTAPI_MOD_ROLLOFF_35},
    }));
    help(u"roll-off",
         u"DVB-S2/S2X modulators: indicate the roll-off factor. The default is auto.");

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
         u"By default, the symbol rate is implicitly computed from the convolutional "
         u"rate, the modulation type and the bitrate. But when --symbol-rate is "
         u"specified, the input bitrate is ignored and the output bitrate is forced "
         u"to the value resulting from the combination of the specified symbol rate, "
         u"convolutional rate and modulation type. "
         u"The options --symbol-rate and --bitrate are mutually exclusive.");

    option(u"t2-fpsf", 0, INTEGER, 0, 1, 1, 255);
    help(u"t2-fpsf",
         u"DVB-T2 modulators: indicate the number of T2 frames per super-frame. "
         u"Must be in the range 1 to 255. The default is 2.");

    option(u"t2-guard-interval", 0, Names({
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

    option(u"t2-l1-modulation", 0, Names({
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

    option(u"transmission-mode", 't', Names({
        {u"2K", DTAPI_MOD_DVBT_2K},
        {u"4K", DTAPI_MOD_DVBT_4K},
        {u"8K", DTAPI_MOD_DVBT_8K},
    }));
    help(u"transmission-mode",
         u"DVB-T modulators: indicate the transmission mode. The default is 8K.");

    option(u"hf-band-region", 0, STRING);
    help(u"hf-band-region", u"name",
         u"Specify the region for UHF/VHF band frequency layout.");

    option(u"uhf-channel", 'u', POSITIVE);
    help(u"uhf-channel",
         u"UHF modulators: indicate the UHF channel number of the output carrier. "
         u"Can be used in replacement to --frequency. "
         u"Can be combined with an --offset-count option. "
         u"The UHF frequency layout depends on the region, see --hf-band-region option.");

    option(u"vhf-channel", 'v', POSITIVE);
    help(u"vhf-channel",
         u"VHF modulators: indicate the VHF channel number of the output carrier. "
         u"Can be used in replacement to --frequency. "
         u"Can be combined with an --offset-count option. "
         u"The VHF frequency layout depends on the region, see --hf-band-region option.");

    option(u"vsb", 0, Names({
        {u"8",  DTAPI_MOD_ATSC_VSB8},
        {u"16", DTAPI_MOD_ATSC_VSB16},
    }));
    help(u"vsb",
         u"ATSC modulators: indicate the VSB constellation. The default is 8.");

    option(u"vsb-taps", 0, INTEGER, 0, 1, 2, 256);
    help(u"vsb-taps",
         u"ATSC modulators: indicate the number of taps of each phase of the "
         u"root-raised cosine filter that is used to shape the spectrum of the "
         u"output signal. The number of taps can have any value between 2 and 256 "
         u"(the implementation is optimized for powers of 2). Specifying more taps "
         u"improves the spectrum, but increases processor overhead. The recommend "
         u"(and default) number of taps is 64 taps. If insufficient CPU power is "
         u"available, 32 taps produces acceptable results, too. ");

    option(u"wait-detach");
    help(u"wait-detach",
         u"At end of stream, the plugin waits until all bytes in the transmit FIFO are sent. "
         u"The options --instant-detach and --wait-detach are mutually exclusive.");
}


//----------------------------------------------------------------------------
// Output destructor
//----------------------------------------------------------------------------

ts::DektecOutputPlugin::~DektecOutputPlugin()
{
    if (_guts != nullptr) {
        DektecOutputPlugin::stop();
        delete _guts;
        _guts = nullptr;
    }
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
    getIntValue(_guts->dev_index, u"device", -1);
    getIntValue(_guts->chan_index, u"channel", -1);
    getValue(_guts->opt_bitrate, u"bitrate", 0);
    _guts->detach_mode = present(u"instant-detach") ? DTAPI_INSTANT_DETACH : (present(u"wait-detach") ? DTAPI_WAIT_UNTIL_SENT : 0);
    _guts->mute_on_stop = false;
    _guts->preload_fifo = present(u"preload-fifo");
    _guts->maintain_preload = present(u"maintain-preload");
    _guts->drop_to_maintain = present(u"drop-to-maintain-preload");
    _guts->carrier_only = present(u"carrier-only");
    getIntValue(_guts->power_mode, u"power-mode", -1);
    GetDektecIOStandardArgs(*this, _guts->iostd_value, _guts->iostd_subvalue);

    // Check options consistency.
    if (present(u"instant-detach") && present(u"wait-detach")) {
        tsp->error(u"options --instant-detach and --wait-detach are mutually exclusive.");
        return false;
    }

    // Get initial bitrate
    _guts->cur_bitrate = _guts->opt_bitrate != 0 ? _guts->opt_bitrate : tsp->bitrate();

    // Locate the device
    if (!_guts->device.getDevice(_guts->dev_index, _guts->chan_index, false, *tsp)) {
        return false;
    }

    // Open the device
    tsp->debug(u"attaching to device %s serial 0x%X", _guts->device.model, _guts->device.desc.m_Serial);
    Dtapi::DTAPI_RESULT status = _guts->dtdev.AttachToSerial(_guts->device.desc.m_Serial);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching output Dektec device %d (%s): %s", _guts->dev_index, _guts->device.model, DektecStrError(status));
        return false;
    }

    // Determine port number and channel capabilities.
    const int port = _guts->device.output[_guts->chan_index].m_Port;
    Dtapi::DtCaps dt_flags = _guts->device.output[_guts->chan_index].m_Flags;

    // Set power mode.
    if (_guts->power_mode >= 0) {
        tsp->debug(u"SetIoConfig(port: %d, group: %d, value: %d)", port, DTAPI_IOCONFIG_PWRMODE, _guts->power_mode);
        status = _guts->dtdev.SetIoConfig(port, DTAPI_IOCONFIG_PWRMODE, _guts->power_mode);
        if (status != DTAPI_OK) {
            return startError(u"set power mode", status);
        }
    }

    // Open the channel
    tsp->debug(u"attaching to port %d", port);
    status = _guts->chan.AttachToPort(&_guts->dtdev, port);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching output channel %d of Dektec device %d (%s): %s", _guts->chan_index, _guts->dev_index, _guts->device.model, DektecStrError(status));
        _guts->dtdev.Detach();
        return false;
    }

    // Get the Vital Product Data (VPD)
    const DektecVPD vpd(_guts->dtdev);

    // Check if the device is a modulator.
    const bool is_modulator = (dt_flags & DTAPI_CAP_MOD) != 0;
    _guts->mute_on_stop = is_modulator;

    // Set default modulation for multi-standard modulators. This may be obsolete now, more modulator models exist.
    // Also adjust device capabilities since m_Flags field is not always set (DTAPI bug? maybe fixed since first found).
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
            // Mute on stop used to be unsupported on that device, maybe no longer true.
            _guts->mute_on_stop = false;
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
            // Mute on stop used to be unsupported on that device, maybe no longer true.
            _guts->mute_on_stop = false;
            break;
        }
        case 115: {
            // DTA-115, multi-standard, depend on embedded licences.
            // DVB-T always supported (?) and is default.
            modulation_type = DTAPI_MOD_DVBT;
            break;
        }
        default:
            // Unknown device.
            modulation_type = -1;
            break;
    }

    // Reset output channel
    tsp->debug(u"resetting channel, mode: %d", DTAPI_FULL_RESET);
    status = _guts->chan.Reset(DTAPI_FULL_RESET);
    if (status != DTAPI_OK) {
        return startError(u"output device reset error", status);
    }

    // Configure I/O standard if necessary.
    if (_guts->iostd_value >= 0) {
        tsp->debug(u"setting IO config of port %d, group: %d, value: %d, subvalue: %d", port, DTAPI_IOCONFIG_IOSTD, _guts->iostd_value, _guts->iostd_subvalue);
        status = _guts->chan.SetIoConfig(DTAPI_IOCONFIG_IOSTD, _guts->iostd_value, _guts->iostd_subvalue);
        if (status != DTAPI_OK) {
            return startError(u"error setting I/O standard", status);
        }
    }

    // Set 188/204-byte output packet format and stuffing
    const int tx_mode = present(u"204") ? DTAPI_TXMODE_ADD16 : DTAPI_TXMODE_188;
    const int stuff_mode = present(u"stuffing") ? 1 : 0;
    tsp->debug(u"setting TxMode, tx: %d, stuff: %d", tx_mode, stuff_mode);
    status = _guts->chan.SetTxMode(tx_mode, stuff_mode);
    if (status != DTAPI_OK) {
        return startError(u"output device SetTxMode error", status);
    }

    // Set modulation parameters for modulators.
    // Overwrite _guts->cur_bitrate and _guts->opt_bitrate with computed values from modulation parameters.
    if (is_modulator && !setModulation(modulation_type)) {
        return false;
    }

    // Set IP parameters for TS-over-IP.
    if ((dt_flags & DTAPI_CAP_IP) != 0) {
        Dtapi::DtIpPars2 ip_pars;
        if (!GetDektecIPArgs(*this, false, ip_pars)) {
            return startError(u"invalid TS-over-IP parameters", DTAPI_OK);
        }

        // Report actual parameters in debug mode
        tsp->debug(u"setting IP parameters: DtIpPars2 = {");
        DektecDevice::ReportIpPars(ip_pars, *tsp, Severity::Debug, u"  ");
        tsp->debug(u"}");

        status = _guts->chan.SetIpPars(&ip_pars);
        if (status != DTAPI_OK) {
            return startError(u"output device SetIpPars error", status);
        }
    }

    // Set output level.
    if (present(u"level")) {
        const int level = intValue<int>(u"level");
        tsp->debug(u"set output level to %d", level);
        status = _guts->chan.SetOutputLevel(level);
        if (status != DTAPI_OK) {
            // In case of error, report it but do not fail.
            // This feature is not supported on all modulators and
            // it seems severe to fail if unsupported.
            tsp->error(u"set modulator output level: %s", DektecStrError(status));
        }
    }

    // Get max FIFO size.
    _guts->max_fifo_size = 0;
    status = _guts->chan.GetFifoSizeMax(_guts->max_fifo_size);
    if (status != DTAPI_OK || _guts->max_fifo_size == 0) {
        // Not supported on this device, use hard-coded value.
        _guts->max_fifo_size = int(DTA_FIFO_SIZE);
        tsp->verbose(u"max fifo size not supported, using %'d bytes", _guts->max_fifo_size);
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
            tsp->verbose(u"setting output fifo size to %'d bytes", size);
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
    tsp->verbose(u"output fifo size: %'d bytes, max: %'d bytes, typical: %'d bytes", _guts->fifo_size, _guts->max_fifo_size, typ_fifo_size);

    if (present(u"preload-fifo-delay")) {
        getIntValue(_guts->preload_fifo_delay, u"preload-fifo-delay");
        if (_guts->preload_fifo_delay && !setPreloadFIFOSizeBasedOnDelay()) {
            // Can't set _guts->preload_fifo_size yet based on delay, because the bitrate hasn't been set yet.
            // for now, fall through to --preload-fifo-percentage, with expectation that it will
            // be calculated later when the caller sets the bitrate on the TSP object (and potentially
            // multiple times later if the bitrate changes multiple times during a session)
            tsp->verbose(u"For --preload-fifo-delay, no bitrate currently set, so will use --preload-fifo-percentage settings until a bitrate has been set.");
        }
    }

    if (!_guts->preload_fifo_size) {
        int preloadFifoPercentage = intValue(u"preload-fifo-percentage", DEFAULT_PRELOAD_FIFO_PERCENTAGE);
        _guts->preload_fifo_size = round_down((_guts->fifo_size * preloadFifoPercentage) / 100, int(PKT_SIZE));
        if (_guts->maintain_preload && _guts->drop_to_maintain) {
            _guts->maintain_threshold = DEFAULT_MAINTAIN_PRELOAD_THRESHOLD_SIZE;
            if ((_guts->preload_fifo_size + _guts->maintain_threshold) > _guts->fifo_size) {
                // Want at least the DEFAULT_MAINTAIN_PRELOAD_THRESHOLD_SIZE threshold when using a percentage of
                // the FIFO and wanting to drop packets.  Note that the preload-fifo-delay approach is preferable
                // when preloading the fifo because it takes the bitrate into question.
                int new_preload_size = round_down(_guts->fifo_size - _guts->maintain_threshold, int(PKT_SIZE));
                tsp->verbose(u"For --preload-fifo-percentage (%d), reducing calculated preload size from %'d bytes to %'d bytes to account for %'d byte threshold "
                    u"because both maintaining preload and dropping packets to maintain preload as necessary.",
                    preloadFifoPercentage, _guts->preload_fifo_size, new_preload_size, _guts->maintain_threshold);
                _guts->preload_fifo_size = new_preload_size;
            }
        }
    }

    // Set output bitrate.
    if (_guts->cur_bitrate == 0) {
        tsp->warning(u"no input bitrate is available, use --bitrate in case of output error");
    }
    else if (!setBitrate(_guts->cur_bitrate)) {
        return startError();
    }

    // Start the transmission on the output device.
    // With ASI devices, we can start transmission right now.
    // With modulator devices, we need to load the FIFO first.
    // However, there is benefit to preloading the FIFO prior
    // to start of transmission even when using ASI, because
    // doing so provides some cushion against variability in
    // thread timing.  Without a preloaded FIFO (hardware
    // buffer), if there are any hiccups caused by, say, context-
    // switching out of the thread that calls send(), such that
    // packets aren't delivered to the ASI channel with the precise
    // timings that they ought to be delivered, this can cause problems
    // for other devices that receive the ASI output channel.  This
    // shouldn't be a problem if a partially filled FIFO is maintained
    // throughout the transmission duration.
    _guts->starting = is_modulator || _guts->preload_fifo;
    // also, note the preload status by resetting _guts->preload_fifo--important to know if it
    // did a preload if the --maintain-preload option is used
    _guts->preload_fifo = _guts->starting;
    const int tx_control = _guts->starting ? DTAPI_TXCTRL_HOLD : DTAPI_TXCTRL_SEND;
    tsp->debug(u"setting TxControl to %d", tx_control);
    status = _guts->chan.SetTxControl(tx_control);
    if (status != DTAPI_OK) {
        return startError(u"output device start send error", status);
    }

    tsp->verbose(u"initial output bitrate: %'d b/s", _guts->cur_bitrate);
    if (_guts->starting) {
        tsp->verbose(u"Will preload FIFO before starting transmission. Preload FIFO size: %'d bytes.", _guts->preload_fifo_size);
    }
    else {
        tsp->verbose(u"Will start transmission immediately.");
    }
    _guts->is_started = true;
    return true;
}


//----------------------------------------------------------------------------
// Output start error method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::startError(const UString& message, unsigned int status)
{
    if (status != DTAPI_OK) {
        tsp->error(u"%s: %s", message, DektecStrError(status));
    }
    else if (!message.empty()){
        tsp->error(message);
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

bool ts::DektecOutputPlugin::computeBitrate(int symbol_rate, int dt_modulation, int param0, int param1, int param2)
{
    int bitrate = -1;
    tsp->debug(u"DtapiModPars2TsRate(..., %d, %d, %d, %d, %d)", dt_modulation, param0, param1, param2, symbol_rate);
    Dtapi::DTAPI_RESULT status = Dtapi::DtapiModPars2TsRate(bitrate, dt_modulation, param0, param1, param2, symbol_rate);
    if (status != DTAPI_OK) {
        return startError(u"Error computing bitrate from symbol rate", status);
    }
    else {
        tsp->verbose(u"setting output TS bitrate to %'d b/s", bitrate);
        _guts->opt_bitrate = _guts->cur_bitrate = BitRate(bitrate);
        return true;
    }
}


//----------------------------------------------------------------------------
// Compute and display symbol rate if not explicitly specified by the user.
//----------------------------------------------------------------------------

void ts::DektecOutputPlugin::displaySymbolRate(const BitRate& ts_bitrate, int dt_modulation, int param0, int param1, int param2)
{
    if (ts_bitrate > 0) {
        int symrate = -1;
        const Dtapi::DtFractionInt frac_bitrate(ToDektecFractionInt(ts_bitrate));
        tsp->debug(u"DtapiModPars2SymRate(..., %d, %d, %d, %d, %d/%d)", dt_modulation, param0, param1, param2, frac_bitrate.m_Num, frac_bitrate.m_Den);
        Dtapi::DTAPI_RESULT status = Dtapi::DtapiModPars2SymRate(symrate, dt_modulation, param0, param1, param2, frac_bitrate);
        if (status != DTAPI_OK) {
            tsp->debug(u"DtapiModPars2SymRate using DtFractionInt failed, using int: %'d", DektecStrError(status), ts_bitrate.toInt());
            tsp->debug(u"DtapiModPars2SymRate(..., %d, %d, %d, %d, %d)", dt_modulation, param0, param1, param2, ts_bitrate.toInt());
            status = Dtapi::DtapiModPars2SymRate(symrate, dt_modulation, param0, param1, param2, int(ts_bitrate.toInt()));
        }
        if (status != DTAPI_OK) {
            tsp->verbose(u"error computing symbol rate: ", DektecStrError(status));
        }
        else {
            tsp->verbose(u"output symbol rate: %'d symbols/second", symrate);
        }
    }
}


//----------------------------------------------------------------------------
// Set bitrate on the output channel.
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setBitrate(const BitRate& bitrate)
{
    const Dtapi::DtFractionInt frac_bitrate(ToDektecFractionInt(bitrate));
    tsp->debug(u"SetTsRateBps(%d/%d), ie. %f", frac_bitrate.m_Num, frac_bitrate.m_Den, bitrate);
    Dtapi::DTAPI_RESULT status = _guts->chan.SetTsRateBps(frac_bitrate);
    if (status == DTAPI_E_NOT_SUPPORTED) {
        tsp->debug(u"setting TsRateBps using DtFractionInt unsupported, using int, SetTsRateBps(%d),", bitrate.toInt());
        status = _guts->chan.SetTsRateBps(int(bitrate.toInt()));
    }
    if (status == DTAPI_OK) {
        return true;
    }
    else {
        tsp->error(u"output device set bitrate error: %s", DektecStrError(status));
        return false;
    }
}


//----------------------------------------------------------------------------
// Set modulation parameters (modulators only).
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setModulation(int& modulation_type)
{
    // Many switch/case structures here use only a subset of the enum type.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    // Get input plugin modulation parameters if required
    const bool use_input_modulation = present(u"input-modulation");
    const ObjectPtr input_params(use_input_modulation ? ObjectRepository::Instance().retrieve(u"tsp.dvb.params") : nullptr);
    const ModulationArgs* input = dynamic_cast<const ModulationArgs*>(input_params.get());
    ModulationArgs other_args;

    // Modulation type is initially unknown.
    modulation_type = DTAPI_MOD_TYPE_UNK;

    // Adjust default modulation type from input plugin
    if (input != nullptr) {
        tsp->debug(u"found input modulator parameters: %s", input->toPluginOptions());
        // Get corresponding Dektec modulation type.
        // The variable is unchanged if no valid value is found.
        GetDektecModulationType(modulation_type, *input);
    }

    // Get user-specified modulation
    modulation_type = intValue<int>(u"modulation", modulation_type);
    if (modulation_type == DTAPI_MOD_TYPE_UNK) {
        return startError(u"unspecified modulation type for " + _guts->device.model, DTAPI_OK);
    }

    // Get user-specified symbol rate, used only with DVB-S/S2/C.
    int symbol_rate = intValue<int>(u"symbol-rate", -1);
    if (present(u"bitrate") && present(u"symbol-rate")) {
        return startError(u"options --symbol-rate and --bitrate are mutually exclusive", DTAPI_OK);
    }
    else if (symbol_rate <= 0 && input != nullptr && input->symbol_rate.has_value()) {
        symbol_rate = int(input->symbol_rate.value());
    }

    // Get UHF/VHF frequency layout.
    const UString region(value(u"hf-band-region"));
    const HFBand* uhf = duck.uhfBand();
    const HFBand* vhf = duck.vhfBand();

    // Compute carrier frequency
    uint64_t frequency = 0;
    if (present(u"frequency") + present(u"satellite-frequency") + present(u"uhf-channel") + present(u"vhf-channel") > 1) {
        return startError(u"options --frequency, --satellite-frequency, --uhf-channel, --vhf-channel are mutually exclusive", DTAPI_OK);
    }
    if (present(u"uhf-channel")) {
        // Display error on invalid channel and return 0 as frequency.
        const uint32_t channel = intValue<uint32_t>(u"uhf-channel");
        uhf->isValidChannel(channel, *this);
        frequency = uhf->frequency(channel, intValue<int32_t>(u"offset-count"));
    }
    else if (present(u"vhf-channel")) {
        // Display error on invalid channel and return 0 as frequency.
        const uint32_t channel = intValue<uint32_t>(u"vhf-channel");
        vhf->isValidChannel(channel, *this);
        frequency = vhf->frequency(channel, intValue<int32_t>(u"offset-count"));
    }
    else if (present(u"satellite-frequency")) {
        const uint64_t sat_frequency = intValue<uint64_t>(u"satellite-frequency");
        if (sat_frequency > 0) {
            // Get LNB description.
            const LNB lnb(value(u"lnb"), *tsp);
            LNB::Transposition transposition;
            if (!lnb.isValid() || !lnb.transpose(transposition, sat_frequency, POL_NONE, *tsp)) {
                return startError(u"invalid LNB / satellite frequency", DTAPI_OK);
            }
            frequency = transposition.intermediate_frequency;
        }
    }
    else if (present(u"frequency")) {
        frequency = intValue<uint64_t>(u"frequency", 0);
    }
    else if (input != nullptr) {
        frequency = input->frequency.value_or(0);
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
            if (input != nullptr) {
                // fec is unmodified if no valid value is found.
                GetDektecCodeRate(fec, *input);
            }
            fec = intValue<int>(u"convolutional-rate", fec);
            tsp->verbose(u"using DVB-S FEC " + DektecFEC().name(fec));
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate <= 0) {
                displaySymbolRate(_guts->opt_bitrate, modulation_type, fec, 0, 0);
            }
            else if (!computeBitrate(symbol_rate, modulation_type, fec, 0, 0)) {
                return false;
            }
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, fec, 0, 0);
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
            if (input != nullptr) {
                // fec is unmodified if no valid value is found.
                GetDektecCodeRate(fec, *input);
                switch (input->pilots.value_or(PILOT_AUTO)) {
                    case PILOT_ON:  pilots = DTAPI_MOD_S2_PILOTS; break;
                    case PILOT_OFF: pilots = DTAPI_MOD_S2_NOPILOTS; break;
                    case PILOT_AUTO: break;
                    default: break;
                }
            }
            fec = intValue<int>(u"convolutional-rate", fec);
            const int fec_frame = present(u"s2-short-fec-frame") ? DTAPI_MOD_S2_SHORTFRM : DTAPI_MOD_S2_LONGFRM;
            const int gold_code = intValue<int>(u"s2-gold-code", 0);
            const int roll_off = intValue<int>(u"roll-off", DTAPI_MOD_ROLLOFF_AUTO);
            const int param1 = pilots | fec_frame | roll_off;
            // Compute expected bitrate if symbol rate is known
            if (symbol_rate <= 0) {
                displaySymbolRate(_guts->opt_bitrate, modulation_type, fec, param1, gold_code);
            }
            else if (!computeBitrate(symbol_rate, modulation_type, fec, param1, gold_code)) {
                return false;
            }
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, fec, param1, gold_code);
            status = _guts->chan.SetModControl(modulation_type, fec, param1, gold_code);
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
            else if (!computeBitrate(symbol_rate, modulation_type, j83, qam_b, 0)) {
                return false;
            }
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, j83, qam_b, 0);
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
            if (use_input_modulation && input == nullptr && _guts->cur_bitrate > 0) {
                // --input-modulation is specified but input plugin is not a DVB-T tuner,
                // use input bitrate to determine modulation parameters.
                BitrateDifferenceDVBTList params_list;
                BitrateDifferenceDVBT::EvaluateToBitrate(params_list, _guts->cur_bitrate);
                if (!params_list.empty()) {
                    // find the closest parameters set, that match user's specified values if there are any
                    for (const auto& params : params_list) {
                        if (ParamsMatchUserOverrides(params)) {
                            other_args = params.tune;
                            input = &other_args;
                            break;
                        }
                    }
                    if (input == nullptr) {
                        // if we couldn't find parameters matching user preference, fallback to best match
                        other_args = params_list.front().tune;
                        input = &other_args;
                    }
                }
            }
            if (input != nullptr) {
                ToDektecCodeRate(fec, input->fec_hp.value_or(FEC_NONE));
                if (input->bandwidth.has_value()) {
                    switch (input->bandwidth.value()) {
                        case 8000000: bw = DTAPI_MOD_DVBT_8MHZ; break;
                        case 7000000: bw = DTAPI_MOD_DVBT_7MHZ; break;
                        case 6000000: bw = DTAPI_MOD_DVBT_6MHZ; break;
                        case 5000000: bw = DTAPI_MOD_DVBT_5MHZ; break;
                        default: break;
                    }
                }
                if (input->modulation.has_value()) {
                    switch (input->modulation.value()) {
                        case QPSK:   constel = DTAPI_MOD_DVBT_QPSK;  break;
                        case QAM_16: constel = DTAPI_MOD_DVBT_QAM16; break;
                        case QAM_64: constel = DTAPI_MOD_DVBT_QAM64; break;
                        default: break;
                    }
                }
                if (input->guard_interval.has_value()) {
                    switch (input->guard_interval.value()) {
                        case GUARD_1_32: guard = DTAPI_MOD_DVBT_G_1_32; break;
                        case GUARD_1_16: guard = DTAPI_MOD_DVBT_G_1_16; break;
                        case GUARD_1_8:  guard = DTAPI_MOD_DVBT_G_1_8;  break;
                        case GUARD_1_4:  guard = DTAPI_MOD_DVBT_G_1_4;  break;
                        default: break;
                    }
                }
                if (input->transmission_mode.has_value()) {
                    switch (input->transmission_mode.value()) {
                        case TM_2K: tr_mode = DTAPI_MOD_DVBT_2K; break;
                        case TM_4K: tr_mode = DTAPI_MOD_DVBT_4K; break;
                        case TM_8K: tr_mode = DTAPI_MOD_DVBT_8K; break;
                        default: break;
                    }
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
                         DektecFEC().name(fec),
                         DektecDVBTProperty().name(bw),
                         DektecDVBTProperty().name(constel),
                         DektecDVBTProperty().name(guard),
                         DektecDVBTProperty().name(tr_mode));
            const int param1 = bw | constel | guard | interleave | tr_mode | dvb_h | s48 | s49;
            // Compute exact expected bitrate (no symbol rate on DVB-T)
            if (!computeBitrate(-1, modulation_type, fec, param1, cell_id)) {
                return false;
            }
            // bw constel guard tr_mode
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, fec, param1, cell_id);
            status = _guts->chan.SetModControl(modulation_type, fec, param1, cell_id);
            break;
        }

        case DTAPI_MOD_DVBT2: {
            Dtapi::DtDvbT2Pars pars;
            pars.Init(); // default values
            // m_T2Version
            // m_T2Profile
            // m_T2BaseLite
            pars.m_Bandwidth = intValue<int>(u"bandwidth", DTAPI_DVBT2_8MHZ);
            pars.m_FftMode = intValue<int>(u"fft-mode", DTAPI_DVBT2_FFT_32K);
            pars.m_Miso = intValue<int>(u"miso", DTAPI_DVBT2_MISO_OFF);
            pars.m_GuardInterval = intValue<int>(u"t2-guard-interval", DTAPI_DVBT2_GI_1_128);
            pars.m_Papr = intValue<int>(u"papr", DTAPI_DVBT2_PAPR_NONE);
            pars.m_BwtExt = present(u"bandwidth-extension") ? DTAPI_DVBT2_BWTEXT_ON : DTAPI_DVBT2_BWTEXT_OFF;
            pars.m_PilotPattern = intValue<int>(u"pilot-pattern", DTAPI_DVBT2_PP_7);
            pars.m_NumT2Frames = intValue<int>(u"t2-fpsf", 2);
            pars.m_L1Modulation = intValue<int>(u"t2-l1-modulation", DTAPI_DVBT2_QAM16);
            pars.m_CellId = intValue<int>(u"cell-id", 0);
            pars.m_NetworkId = intValue<int>(u"t2-network-id", 0);
            pars.m_T2SystemId = intValue<int>(u"t2-system-id", 0);
            // m_L1Repetition
            // m_NumT2Frames
            // m_NumDataSyms
            // m_NumSubslices
            // m_ComponentStartTime
            pars.m_FefEnable = present(u"fef");
            pars.m_FefType = intValue<int>(u"fef-type", 0);
            pars.m_FefS1 = intValue<int>(u"fef-s1", 2);
            pars.m_FefS2 = intValue<int>(u"fef-s2", 1);
            pars.m_FefSignal = intValue<int>(u"fef-signal", DTAPI_DVBT2_FEF_ZERO);
            pars.m_FefLength = intValue<int>(u"fef-length", 1);
            pars.m_FefInterval = intValue<int>(u"fef-interval", 1);
            // m_NumRfChans
            // m_RfChanFreqs
            // m_StartRfIdx
            // Obsolete field in DTAPI 4.10.0.145:
            // pars.m_Frequency = int(frequency);
            pars.m_NumPlps = 1; // This version supports single-PLP only
            pars.m_Plps[0].Init(); // default values
            pars.m_Plps[0].m_Hem = present(u"plp0-high-efficiency");
            pars.m_Plps[0].m_Npd = present(u"plp0-null-packet-deletion");
            pars.m_Plps[0].m_Issy = intValue<int>(u"plp0-issy", DTAPI_DVBT2_ISSY_NONE);
            pars.m_Plps[0].m_IssyBufs = (pars.m_Plps[0].m_Issy == DTAPI_DVBT2_ISSY_NONE) ? 0 : 2 * 1024 * 1024;
            pars.m_Plps[0].m_TsRate = intValue<int>(u"plp0-tsrate", 0);
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

            Dtapi::DtDvbT2ParamInfo info;
            if (use_input_modulation && input == nullptr && _guts->cur_bitrate > 0) {
                // --input-modulation is specified but input plugin is not a DVB-T2 tuner,
                // use input bitrate to determine modulation parameters.
                EvaluateDvbT2ParsForBitrate(pars, _guts->cur_bitrate);
            } else {
                // Compute other fields
                status = pars.OptimisePlpNumBlocks(info, pars.m_Plps[0].m_NumBlocks, pars.m_NumDataSyms);
                if (status != DTAPI_OK) {
                    return startError(u"error computing PLP parameters", status);
                }
            }
            // Report actual parameters in debug mode
            tsp->debug(u"DVB-T2: DtDvbT2Pars = {");
            DektecDevice::ReportDvbT2Pars(pars, *tsp, Severity::Debug, u"  ");
            tsp->debug(u"}");
            if (!(use_input_modulation && input == nullptr && _guts->cur_bitrate > 0)) {
                tsp->debug(u"DVB-T2: DtDvbT2ParamInfo = {");
                DektecDevice::ReportDvbT2ParamInfo(info, *tsp, Severity::Debug, u"  ");
                tsp->debug(u"}");
            }

            // Check validity of T2 parameters
            status = pars.CheckValidity();
            if (status != DTAPI_OK) {
                return startError(u"invalid combination of DVB-T2 parameters", status);
            }
            // Compute exact bitrate from DVB-T2 parameters.
            Dtapi::DtFractionInt frate;
            status = Dtapi::DtapiModPars2TsRate(frate, pars);
            if (status == DTAPI_OK && frate.m_Num > 0 && frate.m_Den > 0) {
                FromDektecFractionInt(_guts->cur_bitrate, frate);
                _guts->opt_bitrate = _guts->cur_bitrate;
            }
            else {
                // Fractional bitrate unsupported or incorrect.
                int irate = 0;
                status = Dtapi::DtapiModPars2TsRate(irate, pars);
                if (status != DTAPI_OK) {
                    return startError(u"Error computing bitrate from DVB-T2 parameters", status);
                }
                _guts->opt_bitrate = _guts->cur_bitrate = BitRate(irate);
            }
            tsp->verbose(u"setting output TS bitrate to %'d b/s", _guts->cur_bitrate);
            // Set modulation parameters
            status = _guts->chan.SetModControl(pars);
            break;
        }

        case DTAPI_MOD_ATSC: {
            int constel = DTAPI_MOD_ATSC_VSB8;
            if (input != nullptr && input->modulation.has_value()) {
                switch (input->modulation.value()) {
                    case VSB_8:  constel = DTAPI_MOD_ATSC_VSB8;  break;
                    case VSB_16: constel = DTAPI_MOD_ATSC_VSB16; break;
                    default: break;
                }
            }
            constel = intValue<int>(u"vsb", constel);
            const int taps = intValue<int>(u"vsb-taps", 64);
            tsp->verbose(u"using ATSC %s", DektecVSB().name(constel));
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, constel, taps, 0);
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
            tsp->debug(u"SetModControl(%d, %d, %d, %d)", modulation_type, bw | constel | fec | header | interleaver | pilots | frame_num, 0, 0);
            status = _guts->chan.SetModControl(modulation_type, bw | constel | fec | header | interleaver | pilots | frame_num, 0, 0);
            break;
        }

        case DTAPI_MOD_ISDBT: {
            return startError(u"ISDB-T modulation not yet supported", DTAPI_OK);
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
    // Make sure to use "__int64" and not "int64_t" in SetRfControl to avoid ambiguous overloading.
    tsp->verbose(u"setting output carrier frequency to %'d Hz", frequency);
    status = _guts->chan.SetRfControl(__int64(frequency));
    if (status != DTAPI_OK) {
        return startError(u"set modulator frequency error", status);
    }
    const int rf_mode = (_guts->carrier_only ? DTAPI_UPCONV_CW : DTAPI_UPCONV_NORMAL) | (present(u"inversion") ? DTAPI_UPCONV_SPECINV : 0);
    tsp->debug(u"SetRfMode(%d)", rf_mode);
    status = _guts->chan.SetRfMode(rf_mode);
    if (status != DTAPI_OK) {
        return startError(u"set modulator RF mode", status);
    }

    // Finally ok
    return true;

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::stop()
{
    if (_guts->is_started) {
        tsp->verbose(u"terminating %s output", _guts->device.model);

        // Mute output signal for modulators which support this
        if (_guts->mute_on_stop) {
            tsp->debug(u"SetRfMode(%d)", DTAPI_UPCONV_MUTE);
            Dtapi::DTAPI_RESULT status = _guts->chan.SetRfMode(DTAPI_UPCONV_MUTE);
            if (status != DTAPI_OK) {
                tsp->error(u"error muting modulator output: " + DektecStrError(status));
            }
        }

        // Detach the channel and the device
        tsp->debug(u"detach channel, mode: %d", _guts->detach_mode);
        _guts->chan.Detach(_guts->detach_mode);
        tsp->debug(u"detach device");
        _guts->dtdev.Detach();

        _guts->is_started = false;
        tsp->verbose(u"%s output terminated", _guts->device.model);
    }
    return true;
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
            tsp->error(u"error getting Dektec device output bitrate: %s", DektecStrError(status));
            bitrate = 0;
        }
    }
    return bitrate;
}

ts::BitRateConfidence ts::DektecOutputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the Dektec device hardware.
    return BitRateConfidence::HARDWARE;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    if (!_guts->is_started) {
        return false;
    }

    // In case of --carrier-only, we silently drop packets to maintain a carrier frequency without modulated TS.
    if (_guts->carrier_only) {
        return true;
    }

    char* data = reinterpret_cast<char*>(const_cast<TSPacket*>(buffer));
    int remain = int(packet_count * PKT_SIZE);
    Dtapi::DTAPI_RESULT status = DTAPI_OK;

    // If no bitrate was specified on the command line, adjust the bitrate when input bitrate changes.
    BitRate new_bitrate;
    if (_guts->opt_bitrate == 0 &&
        _guts->cur_bitrate != (new_bitrate = tsp->bitrate()) &&
        new_bitrate != 0 &&
        setBitrate(new_bitrate))
    {
        _guts->cur_bitrate = new_bitrate;
        tsp->verbose(u"new output bitrate: %'d b/s", _guts->cur_bitrate);

        if (setPreloadFIFOSizeBasedOnDelay()) {
            tsp->verbose(u"Due to new bitrate and specified delay of %d ms, preload FIFO size adjusted: %'d bytes.", _guts->preload_fifo_delay, _guts->preload_fifo_size);
            if (_guts->maintain_threshold) {
                tsp->verbose(u"Further, maintain preload threshold for dropping packets set to %'d bytes based on bitrate.", _guts->maintain_threshold);
            }
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
                tsp->error(u"error getting output fifo load: %s", DektecStrError(status));
                return false;
            }

            if (fifo_load < _guts->preload_fifo_size - int(PKT_SIZE)) {
                // Remain in starting phase, limit next I/O size
                max_io_size = _guts->preload_fifo_size - fifo_load;
            }
            else {
                // FIFO now full enough to start transmitting
                tsp->verbose(u"%s output FIFO load is %'d bytes, starting transmission", _guts->device.model, fifo_load);
                status = _guts->chan.SetTxControl(DTAPI_TXCTRL_SEND);
                if (status != DTAPI_OK) {
                    tsp->error(u"output device start send error: %s", DektecStrError(status));
                    return false;
                }
                // Now fully started
                _guts->starting = false;
            }
        }

        // Limit the transfer size by the maximum I/O size on the device
        int cursize = round_down(std::min(remain, max_io_size), int(PKT_SIZE));

        while (!_guts->starting) {
            int fifo_load;
            status = _guts->chan.GetFifoLoad(fifo_load);
            if (status != DTAPI_OK) {
                tsp->error(u"error getting output fifo load: %s", DektecStrError(status));
                return false;
            }

            if (_guts->preload_fifo && _guts->maintain_preload) {
                if (fifo_load == 0) {
                    // the approach of waiting till the FIFO size hits zero won't handle all cases
                    // in which it gets closer to real-time due to losing data temporarily. If it only
                    // loses data for a short amount of time and doesn't fully drain the FIFO as a result,
                    // the FIFO load won't hit zero, and it won't activate this code in that case.
                    // More intelligent schemes are possible, such as noticing that the FIFO
                    // size is decreasing below the preload FIFO size, and then recognizing when
                    // it levels out, which is an indication that new packets are starting to show up, and,
                    // at this point, it is reasonable to pause transmission. It is preferable not to simply
                    // pause transmission when the FIFO load, say, goes to 1/2 the preload FIFO size, because
                    // those packets that are still in the FIFO are likely connected with those that were just
                    // drained, and we want to keep them together in terms of when they are transmitted if possible.
                    status = _guts->chan.SetTxControl(DTAPI_TXCTRL_HOLD);
                    if (status != DTAPI_OK) {
                        tsp->error(u"output device start send error: %s", DektecStrError(status));
                        return false;
                    }
                    _guts->starting = true;
                    tsp->verbose(u"Pausing transmission temporarily in order to maintain preload");
                }
                else if (_guts->drop_to_maintain) {
                    if ((_guts->drop_to_preload && ((fifo_load + cursize) > _guts->preload_fifo_size)) ||
                        ((fifo_load + cursize) > (_guts->preload_fifo_size + _guts->maintain_threshold))) {
                        if (!_guts->drop_to_preload) {
                            // we would have exceeded the threshold--now drop sufficient packets to get back to
                            // the preload FIFO size
                            _guts->drop_to_preload = true;
                            tsp->verbose(u"Would have exceeded preload FIFO size (%'d bytes) + threshold (%'d bytes), dropping packets to get back to preload FIFO size",
                                         _guts->preload_fifo_size, _guts->maintain_threshold);
                        }

                        // Want to try to get FIFO back to preload_fifo_size, not preload_fifo_size + the threshold.
                        // Note:  the only reason a user would configure the plug-in to maintain the preload and drop packets
                        // is in the case that packets are being delivered to the output plug-in in a real-time fashion.
                        // If packets are, instead, being delivered from a file (i.e. not real-time, in general), then there is no
                        // point in using any of the preload options, as the FIFO can be fully filled up and utilized immediately for
                        // playback.  Because the various preload options would only be used for real-time packet delivery, there is no
                        // need to consider the case that send() is called with a huge buffer, as that can't realistically happen when
                        // dealing with real-time packet delivery.  That is, with real-time packet delivery, you can only deliver packets
                        // to the output plug-in no faster than the TS bitrate.
                        // In that case, how could you ever get to the drop code?  The only realistic situation when this could occur is
                        // if the real-time packets are being received over the network, network connectivity is lost temporarily, and
                        // then restored and delivers all the packets that should have been received during the lost connectivity all at
                        // once, which is possible with some network protocols.  In that case, the FIFO might be drained, depending on how
                        // long network connectivity were temporarily lost.  For example, let's say the FIFO size corrsponds to 3 seconds of
                        // TS media.  Network connectivity is temporarily lost for 4 seconds, which causes the FIFO to be fully drained.  In
                        // addition, the output plug-in, and Dektec hardware, aren't outputting any packets for an additional 1 second.
                        // After the 4 second network connectivity loss, all the missing packets are sent at once such that there are no gaps.
                        // Because maintain_preload is set, however, it will pause playback and prefill the FIFO (which will happen really
                        // quickly) to 3 seconds and then restart playback.  But, the caller of send() will continue to deliver packets, at least
                        // an additional second worth of media plus all packets that arrive afterwards associated with real-time packet delivery.
                        // In this sort of situation, unless packets are dropped, we will get further and further away from real-time.  Due to the
                        // existence of the 3.0 second preload FIFO, the output session is already configured to be 3 seconds behind real-time, but
                        // it would get worse and worse over time without dropping packets.
                        // The threshold is needed to make sure the code doesn't unnecessarily drop packets.  Under normal circumstances, some
                        // calls to send() may exceed the preload FIFO size slightly.
                        int excess = (fifo_load + cursize) - _guts->preload_fifo_size;
                        if (excess >= cursize) {
                            tsp->verbose(u"Dropping all remaining packets (%'d bytes) to maintain preload FIFO size (%'d, %'d, %'d).",
                                         remain, cursize, fifo_load, _guts->preload_fifo_size);
                            return true;
                        }

                        int new_cursize = round_down(cursize - excess, int(PKT_SIZE));
                        int discard = remain - new_cursize;

                        tsp->verbose(u"Dropping %'d bytes worth of packets to maintain preload FIFO size (%'d, %'d, %'d, %'d).",
                                     discard, fifo_load, cursize, remain, _guts->preload_fifo_size);

                        // just deliver as many packets as possible and drop the rest
                        // set remain to cursize so that it doesn't attempt this again with subsequent runs through the loop
                        cursize = new_cursize;
                        remain = cursize;
                    }
                    else if (_guts->drop_to_preload && ((fifo_load + cursize) <= _guts->preload_fifo_size)) {
                        tsp->verbose(u"Got FIFO load (%'d bytes) + new packet data (%'d bytes) back down to preload FIFO size (%'d bytes) by dropping packets.",
                                     fifo_load, cursize, _guts->preload_fifo_size);
                        _guts->drop_to_preload = false;
                    }
                }
            }

            if ((fifo_load + cursize) > _guts->fifo_size) {
                // Wait for the FIFO to be partially cleared to make room for
                // new packets.  Sleep for a short amount of time to minimize the chance
                // that packets are written slightly later than they ought to be written
                // to the output.  This approach mirrors that used in Dektec's DtPlay sample.
                // If packets are written too quickly, without checking the size of the FIFO,
                // it can result in overflows, per information in the DTAPI documentation.
                // Also, this approach fulfills the promise of a "real-time" plug-in, and the
                // Dektec output plug-in indicates that it is a real-time plug-in.
                std::this_thread::sleep_for(cn::milliseconds(1));
                continue;
            }

            break;
        }

        status = _guts->chan.Write(data, cursize);
        if (status != DTAPI_OK) {
            tsp->error(u"transmission error on Dektec device: %s", DektecStrError(status));
            return false;
        }

        if (!_guts->starting) {
            int statusFlags, latched;
            status = _guts->chan.GetFlags(statusFlags, latched);
            if (status == DTAPI_OK) {
                if (latched) {
                    if ((latched & DTAPI_TX_CPU_UFL) != 0) {
                        tsp->verbose(u"Got CPU underflow.");
                    }
                    if ((latched & DTAPI_TX_DMA_UFL) != 0) {
                        tsp->verbose(u"Got DMA underflow.");
                    }
                    if ((latched & DTAPI_TX_FIFO_UFL) != 0) {
                        tsp->verbose(u"Got FIFO underflow.");
                    }
                    _guts->chan.ClearFlags(latched);
                }
            }
        }

        data += cursize;
        remain -= cursize;
    }

    return true;
}

//----------------------------------------------------------------------------
// Set preload FIFO size based on delay, if requested, in ms
// Returns true if preload FIFO size altered, false otherwise
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::setPreloadFIFOSizeBasedOnDelay()
{
    if (_guts->preload_fifo_delay && _guts->cur_bitrate != 0) {
        // calculate new preload FIFO size based on new bitrate
        // to calculate the size, in bytes, based on the bitrate and the requested delay, it is:
        // <bitrate (in bits/s)> / <8 bytes / bit> * <delay (in ms)> / <1000 ms / s>
        // converting to uint64_t because multiplying the current bitrate by the delay may exceed the max value for a uint32_t
        uint64_t prelimPreloadFifoSize = round_down<uint64_t>(((_guts->cur_bitrate * _guts->preload_fifo_delay) / 8000).toInt(), PKT_SIZE);

        _guts->maintain_threshold = 0;
        if (_guts->maintain_preload && _guts->drop_to_maintain) {
            // use a threshold of 10 ms, which seems to work pretty well in practice
            _guts->maintain_threshold = round_down(int(((_guts->cur_bitrate * 10) / 8000).toInt()), int(PKT_SIZE));
        }

        if ((prelimPreloadFifoSize + uint64_t(_guts->maintain_threshold)) > uint64_t(_guts->fifo_size)) {
            _guts->preload_fifo_size = round_down(_guts->fifo_size - _guts->maintain_threshold, int(PKT_SIZE));
            if (_guts->maintain_threshold) {
                tsp->verbose(u"For --preload-fifo-delay, delay (%d ms) too large (%'d bytes), based on bitrate (%'d b/s) and FIFO size (%'d bytes). "
                             u"Using FIFO size - 10 ms maintain preload threshold for preload size instead (%'d bytes).",
                             _guts->preload_fifo_delay,
                             prelimPreloadFifoSize,
                             _guts->cur_bitrate,
                             _guts->fifo_size,
                             _guts->preload_fifo_size);
            }
            else {
                tsp->verbose(u"For --preload-fifo-delay, delay (%d ms) too large (%'d bytes), based on bitrate (%'d b/s) and FIFO size (%'d bytes). "
                             u"Using FIFO size for preload size instead.",
                             _guts->preload_fifo_delay,
                             prelimPreloadFifoSize,
                             _guts->cur_bitrate,
                             _guts->fifo_size);
            }
        }
        else {
            _guts->preload_fifo_size = int(prelimPreloadFifoSize);
        }

        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
// Checks whether calculated parameters for dvb-t do not override user specified params
//----------------------------------------------------------------------------

bool ts::DektecOutputPlugin::ParamsMatchUserOverrides(const ts::BitrateDifferenceDVBT& params)
{
    if (present(u"bandwidth")) {
        auto preferred_bandwidth = intValue<int>(u"bandwidth", 0);
        auto calculated_bandwidth = params.tune.bandwidth.value();
        switch (preferred_bandwidth) {
            case DTAPI_MOD_DVBT_8MHZ:
                if (calculated_bandwidth != 8000000) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_7MHZ:
                if (calculated_bandwidth != 7000000) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_6MHZ:
                if (calculated_bandwidth != 6000000) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_5MHZ:
                if (calculated_bandwidth != 5000000) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    if (present(u"convolutional-rate")) {
        auto preferred_convolutional_rate = intValue<int>(u"convolutional-rate", 0);
        int calculated_convolutional_rate = 0;
        ToDektecCodeRate(calculated_convolutional_rate, params.tune.fec_hp.value_or(FEC_NONE));
        if (calculated_convolutional_rate != preferred_convolutional_rate) {
            return false;
        }
    }
    if (present(u"constellation")) {
        auto preferred_constellation = intValue<int>(u"constellation", 0);
        auto calculated_constellation = params.tune.modulation.value();
        switch (preferred_constellation) {
            case DTAPI_MOD_DVBT_QPSK:
                if (calculated_constellation != ts::QPSK) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_QAM16:
                if (calculated_constellation != ts::QAM_16) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_QAM64:
                if (calculated_constellation != ts::QAM_64) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    if (present(u"guard-interval")) {
        auto preferred_guard_interval = intValue<int>(u"guard-interval", 0);
        auto calculated_guard_interval = params.tune.guard_interval.value();
        switch (preferred_guard_interval) {
            case DTAPI_MOD_DVBT_G_1_32:
                if (calculated_guard_interval != GUARD_1_32) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_G_1_16:
                if (calculated_guard_interval != GUARD_1_16) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_G_1_8:
                if (calculated_guard_interval != GUARD_1_8) {
                    return false;
                }
                break;
            case DTAPI_MOD_DVBT_G_1_4:
                if (calculated_guard_interval != GUARD_1_4) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    return true;
}
