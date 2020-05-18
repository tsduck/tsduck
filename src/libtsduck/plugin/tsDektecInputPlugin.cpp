//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsDektecInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsModulation.h"
#include "tsIntegerUtils.h"
#include "tsSysUtils.h"
#include "tsFatal.h"
#include "tsLNB.h"
TSDUCK_SOURCE;

TS_REGISTER_INPUT_PLUGIN(u"dektec", ts::DektecInputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::DektecInputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Class internals.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

class ts::DektecInputPlugin::Guts
{
};

#else

class ts::DektecInputPlugin::Guts
{
    TS_NOCOPY(Guts);
public:
    Guts();                           // Constructor.
    bool                is_started;   // Device started
    int                 dev_index;    // Dektec device index
    int                 chan_index;   // Device input channel index
    int                 timeout_ms;   // Receive timeout in milliseconds.
    DektecDevice        device;       // Device characteristics
    Dtapi::DtDevice     dtdev;        // Device descriptor
    Dtapi::DtInpChannel chan;         // Input channel
    int                 init_cnt;     // Count the first inputs
    BitRate             cur_bitrate;  // Current input bitrate
    bool                got_bitrate;  // Got bitrate at least once.
    uint64_t            demod_freq;   // Demodulation frequency in Hz
    Dtapi::DtDemodPars  demod_pars;   // Demodulation parameters
    int                 sat_number;   // Satellite number
    Polarization        polarity;     // Polarity.
    bool                high_band;    // Use LNB high frequency band.
    bool                lnb_setup;    // Need LNB setup.
};

ts::DektecInputPlugin::Guts::Guts() :
    is_started(false),
    dev_index(-1),
    chan_index(-1),
    timeout_ms(-1),
    device(),
    dtdev(),
    chan(),
    init_cnt(0),
    cur_bitrate(0),
    got_bitrate(false),
    demod_freq(0),
    demod_pars(),
    sat_number(0),
    polarity(POL_VERTICAL),
    high_band(false),
    lnb_setup(false)
{
}

#endif

//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::isRealTime()
{
    return true;
}

size_t ts::DektecInputPlugin::stackUsage() const
{
    return 512 * 1024; // 512 kB
}


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::DektecInputPlugin::DektecInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive packets from a Dektec DVB-ASI or demodulator device", u"[options]"),
    _guts(new Guts)
{
    CheckNonNull(_guts);

    // Share same option --dvbt-bandwidth for DVB-T2 and DVB-T.
    assert(DTAPI_DVBT2_5MHZ == DTAPI_MOD_DVBT_5MHZ);
    assert(DTAPI_DVBT2_6MHZ == DTAPI_MOD_DVBT_6MHZ);
    assert(DTAPI_DVBT2_7MHZ == DTAPI_MOD_DVBT_7MHZ);
    assert(DTAPI_DVBT2_8MHZ == DTAPI_MOD_DVBT_8MHZ);

    option(u"atsc3-bandwidth", 0, Enumeration({
        {u"6-MHz",  DTAPI_ATSC3_6MHZ},
        {u"7-MHz",  DTAPI_ATSC3_7MHZ},
        {u"8-MHz",  DTAPI_ATSC3_8MHZ},
    }));
    help(u"atsc3-bandwidth",
         u"ATSC demodulators: indicate the ATSC 3.0 bandwidth. The default is 8-MHz.");

    option(u"c2-bandwidth", 0, Enumeration({
        {u"6-MHz",  DTAPI_DVBC2_6MHZ},
        {u"8-MHz",  DTAPI_DVBC2_8MHZ},
    }));
    help(u"c2-bandwidth",
         u"DVB-C2 demodulators: indicate the DVB-C2 bandwidth. The default is 8-MHz.");

    option(u"channel", 'c', UNSIGNED);
    help(u"channel",
         u"Channel index on the input Dektec device. By default, use the "
         u"first input channel on the device.");

    option(u"code-rate", 0, Enumeration({
        {u"auto", DTAPI_MOD_CR_AUTO}, // auto detect
        {u"1/2",  DTAPI_MOD_1_2},     // DVB-S, S2, T
        {u"1/3",  DTAPI_MOD_1_3},     // DVB-S2
        {u"1/4",  DTAPI_MOD_1_4},     // DVB-S2
        {u"2/3",  DTAPI_MOD_2_3},     // DVB-S, S2, T
        {u"2/5",  DTAPI_MOD_2_5},     // DVB-S2
        {u"3/4",  DTAPI_MOD_3_4},     // DVB-S, S2, T
        {u"3/5",  DTAPI_MOD_3_5},     // DVB-S2
        {u"4/5",  DTAPI_MOD_4_5},     // DVB-S, S2
        {u"5/6",  DTAPI_MOD_5_6},     // DVB-S, S2, T
        {u"6/7",  DTAPI_MOD_6_7},     // DVB-S, S2
        {u"7/8",  DTAPI_MOD_7_8},     // DVB-S, S2, T
        {u"8/9",  DTAPI_MOD_8_9},     // DVB-S2
        {u"9/10", DTAPI_MOD_9_10},    // DVB-S2
    }));
    help(u"code-rate",
         u"For demodulators devices only: specify the code rate. "
         u"The specified value depends on the modulation type.\n"
         u"DVB-S: 1/2, 2/3, 3/4, 4/5, 5/6, 6/7, 7/8.\n"
         u"DVB-S2: 1/2, 1/3, 1/4, 2/3, 2/5, 3/4, 3/5, 4/5, 5/6, 6/7, 7/8, 8/9, 9/10.\n"
         u"DVB-T: 1/2, 2/3, 3/4, 5/6, 7/8.\n"
         u"The default is auto.");

    option(u"constellation", 0, Enumeration({
        {u"auto",   DTAPI_MOD_DVBT_CO_AUTO},
        {u"QPSK",   DTAPI_MOD_DVBT_QPSK},
        {u"16-QAM", DTAPI_MOD_DVBT_QAM16},
        {u"64-QAM", DTAPI_MOD_DVBT_QAM64},
    }));
    help(u"constellation",
         u"DVB-T demodulators: indicate the constellation type. The default is auto.");

    option(u"device", 'd', UNSIGNED);
    help(u"device",
         u"Device index, from 0 to N-1 (with N being the number of Dektec devices "
         u"in the system). Use the command \"tsdektec -a [-v]\" to have a "
         u"complete list of devices in the system. By default, use the first "
         u"input Dektec device.");

    option(u"dvbt-bandwidth", 0, Enumeration({
        {u"1.7", DTAPI_DVBT2_1_7MHZ},
        {u"5",   DTAPI_DVBT2_5MHZ},
        {u"6",   DTAPI_DVBT2_6MHZ},
        {u"7",   DTAPI_DVBT2_7MHZ},
        {u"8",   DTAPI_DVBT2_8MHZ},
        {u"10",  DTAPI_DVBT2_10MHZ},
    }));
    help(u"dvbt-bandwidth",
         u"DVB-T/T2 demodulators: indicate the bandwidth in MHz. The default is 8 MHz. "
         u"The bandwidth values 1.7, 5 and 10 MHz are valid for DVB-T2 only.");

    option(u"frequency", 'f', POSITIVE);
    help(u"frequency",
         u"All demodulators: indicate the frequency, in Hz, of the input carrier. There is no default. "
         u"For DVB-S/S2 receivers, the specified frequency is the \"intermediate\" "
         u"frequency. For convenience, the option --satellite-frequency can be used "
         u"instead of --frequency when the intermediate frequency is unknown. "
         u"For DTA-2137 receivers, the valid range is 950 MHz to 2150 MHz (L Band).");

    option(u"guard-interval", 0, Enumeration({
        {u"auto", DTAPI_MOD_DVBT_GU_AUTO},
        {u"1/32", DTAPI_MOD_DVBT_G_1_32},
        {u"1/16", DTAPI_MOD_DVBT_G_1_16},
        {u"1/8",  DTAPI_MOD_DVBT_G_1_8},
        {u"1/4",  DTAPI_MOD_DVBT_G_1_4},
    }));
    help(u"guard-interval",
         u"DVB-T demodulators: indicate the guard interval. The default is auto.");

    option(u"isdbt-bandwidth", 0, Enumeration({
        {u"5", DTAPI_ISDBT_BW_5MHZ},
        {u"6", DTAPI_ISDBT_BW_6MHZ},
        {u"7", DTAPI_ISDBT_BW_7MHZ},
        {u"8", DTAPI_ISDBT_BW_8MHZ},
    }));
    help(u"isdbt-bandwidth",
         u"ISDB-T demodulators: indicate the bandwidth in MHz. The default is 8 MHz.");

    option(u"isdbt-segments", 0, Enumeration({
        {u"1",  DTAPI_ISDBT_SEGM_1},
        {u"3",  DTAPI_ISDBT_SEGM_3},
        {u"13", DTAPI_ISDBT_SEGM_13},
    }));
    help(u"isdbt-segments",
         u"ISDB-T demodulators: indicate the number of segments. The default is 1.");

    option(u"isdbt-subchannel", 0, INTEGER, 0, 1, 0, 41);
    help(u"isdbt-subchannel",
         u"ISDB-T demodulators: indicate the sub-channel number (0..41) of the centre segment of the spectrum. "
         u"The default is 22.");

    option(u"j83", 0, Enumeration({
        {u"A", DTAPI_MOD_J83_A},
        {u"B", DTAPI_MOD_J83_B},
        {u"C", DTAPI_MOD_J83_C},
    }));
    help(u"j83",
         u"QAM demodulators: indicate the ITU-T J.83 annex to use. "
         u"A is DVB-C, B is American QAM, C is Japanese QAM. The default is A.");

    option(u"lnb", 0, STRING);
    help(u"lnb",
         u"DVB-S/S2 receivers: description of the LNB which is used to convert the "
         u"--satellite-frequency into an intermediate frequency. This option is "
         u"useless when --satellite-frequency is not specified. "
         u"The specified string is the name (or an alias for that name) "
         u"of a preconfigured LNB in the configuration file tsduck.lnbs.xml. "
         u"For compatibility, the legacy format 'low_freq[,high_freq,switch_freq]' is also accepted "
         u"(all frequencies are in MHz). The default is a universal extended LNB.");

    option(u"modulation", 'm', Enumeration({
        {u"ATSC-VSB",      DTAPI_MOD_ATSC},
        {u"ATSC-3.0",      DTAPI_MOD_ATSC3},
        {u"DAB",           DTAPI_MOD_DAB},
        {u"DVB-C2",        DTAPI_MOD_DVBC2},
        {u"DVB-S",         DTAPI_MOD_DVBS_QPSK},
        {u"DVB-S-QPSK",    DTAPI_MOD_DVBS_QPSK},
        {u"DVB-S2-8PSK",   DTAPI_MOD_DVBS2_8PSK},
        {u"DVB-S2-16APSK", DTAPI_MOD_DVBS2_16APSK},
        {u"DVB-S2-32APSK", DTAPI_MOD_DVBS2_32APSK},
        {u"DVB-S2",        DTAPI_MOD_DVBS2_QPSK},
        {u"DVB-S2-QPSK",   DTAPI_MOD_DVBS2_QPSK},
        {u"DVB-T",         DTAPI_MOD_DVBT},
        {u"DVB-T2",        DTAPI_MOD_DVBT2},
        {u"ISDB-T",        DTAPI_MOD_ISDBT},
        {u"16-QAM",        DTAPI_MOD_QAM16},
        {u"32-QAM",        DTAPI_MOD_QAM32},
        {u"64-QAM",        DTAPI_MOD_QAM64},
        {u"128-QAM",       DTAPI_MOD_QAM128},
        {u"256-QAM",       DTAPI_MOD_QAM256},
        {u"QAM",           DTAPI_MOD_QAM_AUTO},
    }));
    help(u"modulation",
         u"For demodulators, indicate the modulation type. "
         u"The supported modulation types depend on the device model. "
         u"The default modulation type is DVB-S.\n");

    option(u"polarity", 0, PolarizationEnum);
    help(u"polarity",
         u"DVB-S/S2 receivers: indicate the polarity. The default is \"vertical\".");

    option(u"qam-b", 0, Enumeration({
        {u"auto",     DTAPI_MOD_QAMB_IL_AUTO},
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
         u"QAM demodulators: with --j83 B, indicate the QAM-B interleaver mode. "
         u"The default is auto.");

    option(u"receive-timeout", 't', UNSIGNED);
    help(u"receive-timeout",
         u"Specify the data reception timeout in milliseconds. "
         u"This timeout applies to each receive operation, individually. "
         u"A zero timeout means non-blocking reception. "
         u"By default, receive operations wait for data, possibly forever.");

    option(u"satellite-frequency", 0, POSITIVE);
    help(u"satellite-frequency",
         u"DVB-S/S2 receivers: indicate the target satellite frequency, in Hz, of "
         u"the input carrier. The actual frequency at the input of the receiver "
         u"is the \"intermediate\" frequency which is computed based on the "
         u"characteristics of the LNB (see option --lnb). This option is useful "
         u"when the satellite frequency is better known than the intermediate "
         u"frequency. The options --frequency and --satellite-frequency are mutually "
         u"exclusive.");

    option(u"satellite-number", 0, INTEGER, 0, 1, 0, 3);
    help(u"satellite-number",
         u"DVB-S/S2 receivers: indicate the satellite/dish number. "
         u"Must be 0 to 3 with DiSEqC switches and 0 to 1 for non-DiSEqC switches. "
         u"The default is 0.");

    option(u"symbol-rate", 0, POSITIVE);
    help(u"symbol-rate",
         u"DVB-C/S/S2 demodulators: Specify the symbol rate in symbols/second. "
         u"By default, automatically detect the symbol rate.");

    option(u"t2-profile", 0, Enumeration({
        {u"base", DTAPI_DVBT2_PROFILE_BASE},
        {u"lite", DTAPI_DVBT2_PROFILE_LITE},
    }));
    help(u"t2-profile",
         u"DVB-T2 demodulators: indicate the DVB-T2 profile. The default is base.");

    option(u"transmission-mode", 0, Enumeration({
        {u"auto", DTAPI_MOD_DVBT_MD_AUTO},
        {u"2K",   DTAPI_MOD_DVBT_2K},
        {u"8K",   DTAPI_MOD_DVBT_8K},
    }));
    help(u"transmission-mode",
         u"DVB-T demodulators: indicate the transmission mode. The default is auto.");

    option(u"vsb", 0, Enumeration({
        {u"8",  DTAPI_MOD_ATSC_VSB8},
        {u"16", DTAPI_MOD_ATSC_VSB16},
    }));
    help(u"vsb",
         u"ATSC demodulators: indicate the VSB constellation. The default is 8.");
}


//----------------------------------------------------------------------------
// Input destructor
//----------------------------------------------------------------------------

ts::DektecInputPlugin::~DektecInputPlugin()
{
    if (_guts != nullptr) {
        stop();
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Stubs when compiled without Dektec support.
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

bool ts::DektecInputPlugin::getOptions()
{
    return true;
}

bool ts::DektecInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    return true;
}

bool ts::DektecInputPlugin::start()
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return false;
}

bool ts::DektecInputPlugin::configureLNB()
{
    return false;
}

bool ts::DektecInputPlugin::stop()
{
    return true;
}

ts::BitRate ts::DektecInputPlugin::getBitrate()
{
    return 0;
}

size_t ts::DektecInputPlugin::receive(TSPacket*, TSPacketMetadata*, size_t)
{
    tsp->error(TS_NO_DTAPI_MESSAGE);
    return 0;
}

#else

//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::getOptions()
{
    _guts->dev_index = intValue<int>(u"device", -1);
    _guts->chan_index = intValue<int>(u"channel", -1);
    _guts->timeout_ms = intValue<int>(u"receive-timeout", _guts->timeout_ms); // preserve previous value
    _guts->sat_number = intValue<int>(u"satellite-number", 0);
    _guts->polarity = enumValue<Polarization>(u"polarity", POL_VERTICAL);
    _guts->high_band = false;
    _guts->lnb_setup = false;

    // Compute carrier frequency
    if (present(u"frequency") && present(u"satellite-frequency")) {
        tsp->error(u"options --frequency and --satellite-frequency are mutually exclusive");
        return false;
    }
    uint64_t sat_frequency = intValue<uint64_t>(u"satellite-frequency", 0);
    if (sat_frequency > 0) {
        // Get LNB description.
        const LNB lnb(value(u"lnb"), *tsp);
        LNB::Transposition transposition;
        if (!lnb.isValid() || !lnb.transpose(transposition, sat_frequency, _guts->polarity, *tsp)) {
            tsp->error(u"invalid LNB / satellite frequency");
            return false;
        }
        _guts->demod_freq = transposition.intermediate_frequency;
        _guts->high_band = transposition.band_index > 0;
    }
    else {
        _guts->demod_freq = intValue<uint64_t>(u"frequency", 0);
    }

    // Demodulation parameters.
    if (_guts->demod_freq > 0) {

        Dtapi::DTAPI_RESULT status = _guts->demod_pars.SetModType(intValue<int>(u"modulation", DTAPI_MOD_DVBS_QPSK));
        if (status != DTAPI_OK) {
            tsp->error(u"error setting modulation type: %s", {DektecStrError(status)});
            return false;
        }

        bool ok = true;
        switch (_guts->demod_pars.GetModType()) {
            case DTAPI_MOD_ATSC: {
                Dtapi::DtDemodParsAtsc* atsc = _guts->demod_pars.Atsc();
                ok = atsc != nullptr;
                if (ok) {
                    atsc->m_Constellation = intValue<int>(u"vsb", DTAPI_MOD_ATSC_VSB8);
                }
                break;
            }
            case DTAPI_MOD_ATSC3: {
                Dtapi::DtDemodParsAtsc3* atsc3 = _guts->demod_pars.Atsc3();
                ok = atsc3 != nullptr;
                if (ok) {
                    atsc3->m_Bandwidth = intValue<int>(u"atsc3-bandwidth", DTAPI_ATSC3_8MHZ);
                    atsc3->m_AlpLenIncludesAhSi = false;
                }
                break;
            }
            case DTAPI_MOD_DAB: {
                // There is no parameter for DAB in Dektec demodulators (empty structure).
                Dtapi::DtDemodParsDab* dab = _guts->demod_pars.Dab();
                ok = dab != nullptr;
                break;
            }
            case DTAPI_MOD_DVBC2: {
                Dtapi::DtDemodParsDvbC2* dvbc2 = _guts->demod_pars.DvbC2();
                ok = dvbc2 != nullptr;
                if (ok) {
                    dvbc2->m_Bandwidth = intValue<int>(u"c2-bandwidth", DTAPI_DVBC2_8MHZ);
                    dvbc2->m_ScanL1Part2Data = false;
                }
                break;
            }
            case DTAPI_MOD_DVBS_QPSK: {
                Dtapi::DtDemodParsDvbS* dvbs = _guts->demod_pars.DvbS();
                ok = dvbs != nullptr;
                if (ok) {
                    dvbs->m_CodeRate = intValue<int>(u"code-rate", DTAPI_MOD_CR_AUTO);
                    dvbs->m_SymRate = intValue<int>(u"symbol-rate", DTAPI_MOD_SYMRATE_AUTO);
                    dvbs->m_SpecInv = DTAPI_MOD_S_S2_SPECINV_AUTO;
                }
                _guts->lnb_setup = true;
                break;
            }
            case DTAPI_MOD_DVBS2_8PSK:
            case DTAPI_MOD_DVBS2_16APSK:
            case DTAPI_MOD_DVBS2_32APSK:
            case DTAPI_MOD_DVBS2_QPSK: {
                Dtapi::DtDemodParsDvbS2* dvbs2 = _guts->demod_pars.DvbS2();
                ok = dvbs2 != nullptr;
                if (ok) {
                    dvbs2->m_CodeRate = intValue<int>(u"code-rate", DTAPI_MOD_CR_AUTO);
                    dvbs2->m_SymRate = intValue<int>(u"symbol-rate", DTAPI_MOD_SYMRATE_AUTO);
                    dvbs2->m_FecFrame = DTAPI_MOD_S2_FRM_AUTO;
                    dvbs2->m_Pilots = DTAPI_MOD_S2_PILOTS_AUTO;
                    dvbs2->m_SpecInv = DTAPI_MOD_S_S2_SPECINV_AUTO;
                }
                _guts->lnb_setup = true;
                break;
            }
            case DTAPI_MOD_DVBT: {
                Dtapi::DtDemodParsDvbT* dvbt = _guts->demod_pars.DvbT();
                ok = dvbt != nullptr;
                if (ok) {
                    dvbt->m_Bandwidth = intValue<int>(u"dvbt-bandwidth", DTAPI_MOD_DVBT_8MHZ);
                    dvbt->m_CodeRate = intValue<int>(u"code-rate", DTAPI_MOD_CR_AUTO);
                    dvbt->m_Constellation = intValue<int>(u"constellation", DTAPI_MOD_DVBT_CO_AUTO);
                    dvbt->m_Guard = intValue<int>(u"guard-interval", DTAPI_MOD_DVBT_GU_AUTO);
                    dvbt->m_Mode = intValue<int>(u"transmission-mode", DTAPI_MOD_DVBT_MD_AUTO);
                    dvbt->m_Interleaving = DTAPI_MOD_DVBT_IL_AUTO;
                }
                break;
            }
            case DTAPI_MOD_DVBT2: {
                Dtapi::DtDemodParsDvbT2* dvbt2 = _guts->demod_pars.DvbT2();
                ok = dvbt2 != nullptr;
                if (ok) {
                    dvbt2->m_Bandwidth = intValue<int>(u"dvbt-bandwidth", DTAPI_DVBT2_8MHZ);
                    dvbt2->m_T2Profile = intValue<int>(u"t2-profile", DTAPI_DVBT2_PROFILE_BASE);
                }
                break;
            }
            case DTAPI_MOD_ISDBT: {
                Dtapi::DtDemodParsIsdbt* isdbt = _guts->demod_pars.Isdbt();
                ok = isdbt != nullptr;
                if (ok) {
                    isdbt->m_Bandwidth = intValue<int>(u"isdbt-bandwidth", DTAPI_ISDBT_BW_8MHZ);
                    isdbt->m_SubChannel = intValue<int>(u"isdbt-subchannel", 22); // 0..41, channel 22 is the default
                    isdbt->m_NumberOfSegments = intValue<int>(u"isdbt-segments", DTAPI_ISDBT_SEGM_1);
                }
                break;
            }
            case DTAPI_MOD_QAM16:
            case DTAPI_MOD_QAM32:
            case DTAPI_MOD_QAM64:
            case DTAPI_MOD_QAM128:
            case DTAPI_MOD_QAM256:
            case DTAPI_MOD_QAM_AUTO: {
                Dtapi::DtDemodParsQam* qam = _guts->demod_pars.Qam();
                ok = qam != nullptr;
                if (ok) {
                    qam->m_SymRate = intValue<int>(u"symbol-rate", DTAPI_MOD_SYMRATE_AUTO);
                    qam->m_Annex = intValue<int>(u"j83", DTAPI_MOD_J83_A);
                    qam->m_Interleaving = intValue<int>(u"qam-b", DTAPI_MOD_QAMB_IL_AUTO);
                }
                break;
            }
            default: {
                tsp->error(u"invalid Dektec demodulation type");
                return false;
            }
        }

        // Check if any parameter structure was inaccessible.
        if (!ok) {
            tsp->error(u"internal Dektec library error, no parameter for modulation type");
            return false;
        }

        // Check consistency of demodulation parameters.
        status = _guts->demod_pars.CheckValidity();
        if (status != DTAPI_OK) {
            tsp->error(u"invalid Dektec demodulation parameters: %s", {DektecStrError(status)});
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        _guts->timeout_ms = int(timeout);
    }
    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::start()
{
    if (_guts->is_started) {
        tsp->error(u"already started");
        return false;
    }

    // Locate the device
    if (!_guts->device.getDevice(_guts->dev_index, _guts->chan_index, true, *tsp)) {
        return false;
    }

    // Open the device
    Dtapi::DTAPI_RESULT status = _guts->dtdev.AttachToSerial(_guts->device.desc.m_Serial);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching input Dektec device %d: %s", {_guts->dev_index, DektecStrError(status)});
        return false;
    }

    // Open the input channel
    status = _guts->chan.AttachToPort(&_guts->dtdev, _guts->device.input[_guts->chan_index].m_Port);
    if (status != DTAPI_OK) {
        tsp->error(u"error attaching input channel %d of Dektec device %d: %s", {_guts->chan_index, _guts->dev_index, DektecStrError(status)});
        _guts->dtdev.Detach();
        return false;
    }

    // Reset input channel
    status = _guts->chan.Reset(DTAPI_FULL_RESET);
    if (status != DTAPI_OK) {
        tsp->error(u"input device reset error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    status = _guts->chan.SetRxControl(DTAPI_RXCTRL_IDLE);
    if (status != DTAPI_OK) {
        tsp->error(u"device SetRxControl error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    status = _guts->chan.ClearFifo();            // Clear FIFO (i.e. start with zero load)
    status = _guts->chan.ClearFlags(0xFFFFFFFF); // Clear all flags

    // Apply demodulation settings
    if (_guts->demod_freq > 0) {

        // Configure the LNB for satellite reception.
        if (_guts->lnb_setup && !configureLNB()) {
            return false;
        }

        // Tune to the frequency and demodulation parameters.
        status = _guts->chan.Tune(int64_t(_guts->demod_freq), &_guts->demod_pars);
        if (status != DTAPI_OK) {
            tsp->error(u"error tuning Dektec demodulator: %s", {DektecStrError(status)});
            _guts->chan.Detach(0);
            _guts->dtdev.Detach();
            return false;
        }
    }

    // Set the receiving packet size to 188 bytes (the size of the packets
    // which are returned by the board to the application, dropping extra 16
    // bytes if the transmitted packets are 204-byte).
    status = _guts->chan.SetRxMode(DTAPI_RXMODE_ST188);
    if (status != DTAPI_OK) {
        tsp->error(u"device SetRxMode error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Start the capture on the input device (set receive control to "receive")
    status = _guts->chan.SetRxControl(DTAPI_RXCTRL_RCV);
    if (status != DTAPI_OK) {
        tsp->error(u"device SetRxControl error: %s", {DektecStrError(status)});
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        return false;
    }

    // Consider that the first 5 inputs are "initialization". If a full input
    // fifo is observed here, ignore it. Later, a full fifo indicates potential
    // packet loss.
    _guts->init_cnt = 5;
    _guts->is_started = true;
    return true;
}


//----------------------------------------------------------------------------
// Configure the LNB.
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::configureLNB()
{
    // For satellite reception, control the dish first.
    // See the LinuxTV implementation for more details (file linux/tsTuner.cpp).
    //
    // Modern LNB's switch their polarisation depending of the DC component of
    // their input (13V for vertical polarisation, 18V for horizontal).
    // When they see a 22kHz signal at their input they switch into the high
    // band and use a somewhat higher intermediate frequency to downconvert
    // the signal.
    //
    // When your satellite equipment contains a DiSEqC switch device to switch
    // between different satellites you have to send the according DiSEqC
    // commands, usually command 0x38. Take a look into the DiSEqC spec
    // available at http://www.eutelsat.org/ for the complete list of commands.
    //
    // The burst signal is used in old equipments and by cheap satellite A/B
    // switches.
    //
    // Voltage, burst and 22kHz tone have to be consistent to the values
    // encoded in the DiSEqC commands.

    // Enable the LNB controller.
    Dtapi::DTAPI_RESULT status = _guts->chan.LnbEnable(true);
    if (status != DTAPI_OK) {
        tsp->error(u"error enabling Dektec LNB controller: %s", {DektecStrError(status)});
        return false;
    }

    // Stop 22 kHz continuous tone (was on if previously tuned on high band).
    status = _guts->chan.LnbEnableTone(false);
    if (status != DTAPI_OK) {
        tsp->error(u"error stopping LNB tone: %s", {DektecStrError(status)});
        return false;
    }

    // Setup polarisation voltage: 13V for vertical polarisation, 18V for horizontal
    status = _guts->chan.LnbSetVoltage(_guts->polarity == POL_VERTICAL ? DTAPI_LNB_13V : DTAPI_LNB_18V);
    if (status != DTAPI_OK) {
        tsp->error(u"error setting LNB voltage: %s", {DektecStrError(status)});
        return false;
    }

    // Wait at least 15ms. Not sure it is necessary with Dektec. It is necessary with LinuxTV.
    // Is this required by Linux TV or this is the required LNB setup ?
    SleepThread(15);

    // Send tone burst: A for satellite 0, B for satellite 1.
    // DiSEqC switches may address up to 4 dishes (satellite number 0 to 3)
    // while non-DiSEqC switches can address only 2 (satellite number 0 to 1).
    // This is why the DiSEqC command has space for 2 bits (4 states) while
    // the "send tone burst" command is binary (A or B).
    status = _guts->chan.LnbSendBurst(_guts->sat_number == 0 ? DTAPI_LNB_BURST_A : DTAPI_LNB_BURST_B);
    if (status != DTAPI_OK) {
        tsp->error(u"error sending LNB burst: %s", {DektecStrError(status)});
        return false;
    }

    // Wait 15ms again.
    SleepThread(15);

    // Send DiSEqC commands. See DiSEqC spec ...
    uint8_t cmd[6];
    cmd[0] = 0xE0;  // Command from master, no reply expected, first transmission
    cmd[1] = 0x10;  // Any LNB or switcher (master to all)
    cmd[2] = 0x38;  // Write to port group 0
    cmd[3] = 0xF0 | // Clear all 4 flags first, then set according to next 4 bits
        ((uint8_t(_guts->sat_number) << 2) & 0x0F) |
        (_guts->polarity == POL_VERTICAL ? 0x00 : 0x02) |
        (_guts->high_band ? 0x01 : 0x00);
    cmd[4] = 0x00;  // Unused
    cmd[5] = 0x00;  // Unused

    status = _guts->chan.LnbSendDiseqcMessage(cmd, int(sizeof(cmd)));
    if (status != DTAPI_OK) {
        tsp->error(u"error sending DiSeqC command: %s", {DektecStrError(status)});
        return false;
    }

    // Wait 15ms again.
    SleepThread(15);

    // Start the 22kHz continuous tone when tuning to a transponder in the high band
    status = _guts->chan.LnbEnableTone(_guts->high_band);
    if (status != DTAPI_OK) {
        tsp->error(u"error set LNB tone: %s", {DektecStrError(status)});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::DektecInputPlugin::stop()
{
    if (_guts->is_started) {
        _guts->chan.Detach(0);
        _guts->dtdev.Detach();
        _guts->is_started = false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Get input bitrate
//----------------------------------------------------------------------------

ts::BitRate ts::DektecInputPlugin::getBitrate()
{
    if (!_guts->is_started) {
        return 0;
    }

    int bitrate;
    Dtapi::DTAPI_RESULT status = _guts->chan.GetTsRateBps(bitrate);

    if (status != DTAPI_OK) {
        tsp->error(u"error getting Dektec device input bitrate: %s", {DektecStrError(status)});
        return 0;
    }
    if (_guts->got_bitrate && bitrate != int(_guts->cur_bitrate)) {
        tsp->verbose(u"new input bitrate: %'d b/s", {bitrate});
    }

    _guts->got_bitrate = true;
    return _guts->cur_bitrate = bitrate;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::DektecInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    if (!_guts->is_started) {
        return 0;
    }

    Dtapi::DTAPI_RESULT status = DTAPI_OK;

    // After initialization, we check the receive FIFO load before reading it.
    // If the FIFO is full, we have lost packets.
    if (_guts->init_cnt > 0) {
        _guts->init_cnt--;
    }
    if (_guts->init_cnt == 0) {
        int fifo_load;
        status = _guts->chan.GetFifoLoad(fifo_load);
        if (status != DTAPI_OK) {
            tsp->error(u"error getting input fifo load: %s", {DektecStrError(status)});
        }
        if (fifo_load >= int(DTA_FIFO_SIZE)) {
            // Input overflow.
            tsp->warning(u"input fifo full, possible packet loss");
        }
    }

    // Do not read more than what a DTA device accepts
    size_t size = RoundDown(std::min(max_packets * PKT_SIZE, DTA_MAX_IO_SIZE), PKT_SIZE);

    // Receive packets.
    if (_guts->timeout_ms < 0) {
        // Receive without timeout (wait forever if no input signal).
        status = _guts->chan.Read(reinterpret_cast<char*>(buffer), int(size));
    }
    else {
        // Receive with timeout (can be null, ie. non-blocking).
        status = _guts->chan.Read(reinterpret_cast<char*>(buffer), int(size), _guts->timeout_ms);
    }

    if (status == DTAPI_OK) {
        return size / PKT_SIZE;
    }
    else {
        tsp->error(u"capture error on Dektec device %d: %s", {_guts->dev_index, DektecStrError(status)});
        return 0;
    }
}

#endif // TS_NO_DTAPI
