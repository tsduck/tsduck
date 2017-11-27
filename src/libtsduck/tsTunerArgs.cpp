//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Command line arguments for DVB tuners
//
//----------------------------------------------------------------------------

#include "tsTunerArgs.h"
#include "tsTuner.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TunerArgs::TunerArgs(bool info_only, bool allow_short_options) :
    device_name(),
    signal_timeout(Tuner::DEFAULT_SIGNAL_TIMEOUT),
    receive_timeout(0),
#if defined(TS_LINUX)
    demux_buffer_size(Tuner::DEFAULT_DEMUX_BUFFER_SIZE),
#elif defined(TS_WINDOWS)
    demux_queue_size(Tuner::DEFAULT_SINK_QUEUE_SIZE),
#endif
    zap_specification(),
    channel_name(),
    zap_file_name(),
    frequency(),
    polarity(),
    lnb(),
    inversion(),
    symbol_rate(),
    inner_fec(),
    satellite_number(),
    modulation(),
    bandwidth(),
    fec_hp(),
    fec_lp(),
    transmission_mode(),
    guard_interval(),
    hierarchy(),
    delivery_system(),
    pilots(),
    roll_off(),
    _info_only(info_only),
    _allow_short_options(allow_short_options)
{
}


//----------------------------------------------------------------------------
// Reset all values, they become "unset"
//----------------------------------------------------------------------------

void ts::TunerArgs::reset()
{
    device_name.clear();
    signal_timeout = Tuner::DEFAULT_SIGNAL_TIMEOUT;
    receive_timeout = 0;
#if defined(TS_LINUX)
    demux_buffer_size = Tuner::DEFAULT_DEMUX_BUFFER_SIZE;
#elif defined(TS_WINDOWS)
    demux_queue_size = Tuner::DEFAULT_SINK_QUEUE_SIZE;
#endif
    zap_specification.reset();
    channel_name.reset();
    zap_file_name.reset();
    frequency.reset();
    polarity.reset();
    lnb.reset();
    inversion.reset();
    symbol_rate.reset();
    inner_fec.reset();
    satellite_number.reset();
    modulation.reset();
    bandwidth.reset();
    fec_hp.reset();
    fec_lp.reset();
    transmission_mode.reset();
    guard_interval.reset();
    hierarchy.reset();
    delivery_system.reset();
    pilots.reset();
    roll_off.reset();
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::TunerArgs::load(Args& args)
{
    reset();

    // Tuner identification.
    if (args.present(u"adapter") && args.present(u"device-name")) {
        args.error("choose either --adapter or --device-name but not both");
    }
    if (args.present(u"device-name")) {
        device_name = args.value(u"device-name");
    }
    else if (args.present(u"adapter")) {
        const int adapter = args.intValue(u"adapter", 0);
#if defined(TS_LINUX)
        device_name = UString::Format(u"/dev/dvb/adapter%d", {adapter});
#elif defined(TS_WINDOWS)
        device_name = UString::Format(u":%d", {adapter});
#else
        // Does not mean anything, just for error messages.
        device_name = Format("DVB adapter %d", adapter);
#endif
    }

    // Tuning options.
    if (!_info_only) {
        bool got_one = false;

        // Reception parameters.
        signal_timeout = args.intValue<MilliSecond>(u"signal-timeout", Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) * 1000;
        receive_timeout = args.intValue<MilliSecond>(u"receive-timeout", 0);
#if defined(TS_LINUX)
        demux_buffer_size = args.intValue<size_t>(u"demux-buffer-size", Tuner::DEFAULT_DEMUX_BUFFER_SIZE);
#elif defined(TS_WINDOWS)
        demux_queue_size = args.intValue<size_t>(u"demux-queue-size", Tuner::DEFAULT_SINK_QUEUE_SIZE);
#endif

        // Carrier frequency
        if (args.present(u"frequency") + args.present(u"uhf-channel") + args.present(u"vhf-channel") > 1) {
            args.error("options --frequency, --uhf-channel and --vhf-channel are mutually exclusive");
        }
        else if (args.present(u"frequency")) {
            got_one = true;
            frequency = args.intValue<uint64_t>(u"frequency");
        }
        else if (args.present(u"uhf-channel")) {
            got_one = true;
            frequency = UHF::Frequency(args.intValue<int>(u"uhf-channel", 0), args.intValue<int>(u"offset-count", 0));
        }
        else if (args.present(u"vhf-channel")) {
            got_one = true;
            frequency = VHF::Frequency(args.intValue<int>(u"vhf-channel", 0), args.intValue<int>(u"offset-count", 0));
        }

        // Other individual tuning options
        if (args.present(u"symbol-rate")) {
            got_one = true;
            symbol_rate = args.intValue<uint32_t>(u"symbol-rate");
        }
        if (args.present(u"polarity")) {
            got_one = true;
            polarity = args.enumValue<Polarization>(u"polarity");
        }
        if (args.present(u"spectral-inversion")) {
            got_one = true;
            inversion = args.enumValue<SpectralInversion>(u"spectral-inversion");
        }
        if (args.present(u"fec-inner")) {
            got_one = true;
            inner_fec = args.enumValue<InnerFEC>(u"fec-inner");
        }
        if (args.present(u"modulation")) {
            got_one = true;
            modulation = args.enumValue<Modulation>(u"modulation");
        }
        if (args.present(u"bandwidth")) {
            got_one = true;
            bandwidth = args.enumValue<BandWidth>(u"bandwidth");
        }
        if (args.present(u"high-priority-fec")) {
            got_one = true;
            fec_hp = args.enumValue<InnerFEC>(u"high-priority-fec");
        }
        if (args.present(u"low-priority-fec")) {
            got_one = true;
            fec_lp = args.enumValue<InnerFEC>(u"low-priority-fec");
        }
        if (args.present(u"transmission-mode")) {
            got_one = true;
            transmission_mode = args.enumValue<TransmissionMode>(u"transmission-mode");
        }
        if (args.present(u"guard-interval")) {
            got_one = true;
            guard_interval = args.enumValue<GuardInterval>(u"guard-interval");
        }
        if (args.present(u"hierarchy")) {
            got_one = true;
            hierarchy = args.enumValue<Hierarchy>(u"hierarchy");
        }
        if (args.present(u"delivery-system")) {
            got_one = true;
            delivery_system = args.enumValue<DeliverySystem>(u"delivery-system");
        }
        if (args.present(u"pilots")) {
            got_one = true;
            pilots = args.enumValue<Pilot>(u"pilots");
        }
        if (args.present(u"roll-off")) {
            got_one = true;
            roll_off = args.enumValue<RollOff>(u"roll-off");
        }

        // Local options (not related to transponder)
        if (args.present(u"lnb")) {
            UString s(args.value(u"lnb"));
            LNB l(s);
            if (!l.isValid()) {
                args.error(u"invalid LNB description " + s);
            }
            else {
                lnb = l;
            }
        }
        if (args.present(u"satellite-number")) {
            satellite_number = args.intValue<size_t>(u"satellite-number");
        }

        // Locating the transponder by channel
        if (args.present(u"channel-transponder")) {
            channel_name = args.value(u"channel-transponder");
        }
        if (args.present(u"zap-config-file")) {
            zap_file_name = args.value(u"zap-config-file");
        }

        // Zap format tune specification
        if (args.present(u"tune")) {
            zap_specification = args.value(u"tune");
        }

        // Mutually exclusive methods of locating the channels
        if (got_one + channel_name.set() + zap_specification.set() > 1) {
            args.error(u"--tune, --channel-transponder and individual tuning options are incompatible");
        }
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TunerArgs::defineOptions(Args& args) const
{
    // Tuner identification.
    args.option(u"adapter", _allow_short_options ? u'a' : 0, Args::UNSIGNED);
    args.option(u"device-name", _allow_short_options ? u'd' : 0, Args::STRING);

    if (!_info_only) {

        // Reception parameters.
        args.option(u"receive-timeout", 0, Args::UNSIGNED);
        args.option(u"signal-timeout", 0, Args::UNSIGNED);
#if defined(TS_LINUX)
        args.option(u"demux-buffer-size", 0, Args::UNSIGNED);
#elif defined(TS_WINDOWS)
        args.option(u"demux-queue-size", 0, Args::UNSIGNED);
#endif

        // Tuning options.
        args.option(u"bandwidth", 0, BandWidthEnum);
        args.option(u"channel-transponder", _allow_short_options ? 'c' : 0, Args::STRING);
        args.option(u"delivery-system", 0, DeliverySystemEnum);
        args.option(u"fec-inner", 0, InnerFECEnum);
        args.option(u"frequency", _allow_short_options ? 'f' : 0, Args::UNSIGNED);
        args.option(u"guard-interval", 0, GuardIntervalEnum);
        args.option(u"hierarchy", 0, HierarchyEnum);
        args.option(u"high-priority-fec", 0, InnerFECEnum);
        args.option(u"lnb", 0, Args::STRING);
        args.option(u"low-priority-fec", 0, InnerFECEnum);
        args.option(u"modulation", _allow_short_options ? 'm' : 0, ModulationEnum);
        args.option(u"offset-count", 0, Args::INTEGER, 0, 1, -3, 3);
        args.option(u"pilots", 0, PilotEnum);
        args.option(u"polarity", 0, PolarizationEnum);
        args.option(u"roll-off", 0, RollOffEnum);
        args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
        args.option(u"spectral-inversion", 0, SpectralInversionEnum);
        args.option(u"symbol-rate", _allow_short_options ? 's' : 0, Args::UNSIGNED);
        args.option(u"transmission-mode", 0, TransmissionModeEnum);
        args.option(u"tune", 0, Args::STRING);
        args.option(u"uhf-channel", 0, Args::INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
        args.option(u"vhf-channel", 0, Args::INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);
        args.option(u"zap-config-file", _allow_short_options ? 'z' : 0, Args::STRING);
    }
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TunerArgs::addHelp(Args& args) const
{
    UString help =
        u"\n"
        u"Tuner identification:\n"
        u"\n" +
        UString(_allow_short_options ? u"  -a N\n" : u"") +
        u"  --adapter N\n"
#if defined(TS_LINUX)
        u"      Specifies the Linux DVB adapter N (/dev/dvb/adapterN).\n"
#elif defined(TS_WINDOWS)
        u"      Specifies the Nth DVB adapter in the system.\n"
#endif
        u"      This option can be used instead of device name.\n"
        u"\n" +
        UString(_allow_short_options ? u"  -d \"name\"\n" : u"") +
        u"  --device-name \"name\"\n"
#if defined (TS_LINUX)
        u"      Specify the DVB receiver device name, /dev/dvb/adapterA[:F[:M[:V]]]\n"
        u"      where A = adapter number, F = frontend number (default: 0), M = demux\n"
        u"      number (default: 0), V = dvr number (default: 0).\n"
#elif defined (TS_WINDOWS)
        u"      Specify the DVB receiver device name. This is a DirectShow/BDA tuner\n"
        u"      filter name (not case sensitive, blanks are ignored).\n"
#endif
        u"      By default, the first DVB receiver device is used.\n"
        u"      Use the tslsdvb utility to list all DVB devices.\n";

    if (!_info_only) {
        help +=
            u"\n"
            u"Tuner reception parameters:\n"
#if defined (TS_LINUX)
            u"\n"
            u"  --demux-buffer-size value\n"
            u"      Default buffer size, in bytes, of the demux device.\n"
            u"      The default is 1 MB.\n"
#elif defined (TS_WINDOWS)
            u"\n"
            u"  --demux-queue-size value\n"
            u"      Specify the maximum number of media samples in the queue between the\n"
            u"      DirectShow capture thread and the input plugin thread. The default is\n"
            u"      " + UString::Decimal(Tuner::DEFAULT_SINK_QUEUE_SIZE) + u" media samples.\n"
#endif
            u"\n"
            u"  --receive-timeout milliseconds\n"
            u"      Specifies the timeout, in milliseconds, for each receive operation.\n"
            u"      To disable the timeout and wait indefinitely for packets, specify zero.\n"
            u"      This is the default.\n"
            u"\n"
            u"  --signal-timeout seconds\n"
            u"      Specifies the timeout, in seconds, for DVB signal locking. If no signal\n"
            u"      is detected after this timeout, the command aborts. To disable the\n"
            u"      timeout and wait indefinitely for the signal, specify zero. The default\n"
            u"      is " + UString::Decimal(Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) + u" seconds.\n"
            u"\n"
            u"Tuning:\n"
            u"\n"
            u"  By default, no tuning is performed on the DVB frontend. The transponder\n"
            u"  on which the frontend is currently tuned is used. There are three ways to\n"
            u"  specify a new transponder: specifying individual tuning options, a global\n"
            u"  tuning information string using the Linux DVB \"zap\" format, the name of\n"
            u"  a channel contained in the transponder (with appropriate channel\n"
            u"  configuration files).\n"
            u"\n"
            u"Individual tuning options:\n"
            u"\n"
            u"  --bandwidth value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Must be one of \"auto\", \"8-MHz\", \"7-MHz\", \"6-MHz\", \"5-MHz\".\n"
            u"      The default is \"8-MHz\".\n"
            u"\n"
            u"  --delivery-system value\n"
            u"      Used for DVB-S and DVB-S2 tuners only.\n"
            u"      Which delivery system to use. Must be one of \"DVB-S\", \"DVB-S2\".\n"
            u"      The default is \"DVB-S\".\n"
            u"\n"
            u"  --fec-inner value\n"
            u"      Used for DVB-S, DVB-S2 and DVB-C tuners only.\n"
            u"      Inner Forward Error Correction. Must be one of \"none\", \"auto\", \"1/2\",\n"
            u"      \"1/3\", \"1/4\", \"2/3\", \"2/5\", \"3/4\", \"3/5\", \"4/5\", \"5/6\", \"5/11\",\n"
            u"      \"6/7\", \"7/8\", \"8/9\", \"9/10\". The default is \"auto\".\n"
            u"\n" +
            UString(_allow_short_options ? u"  -f value\n" : u"") +
            u"  --frequency value\n"
            u"      Carrier frequency in Hz (all tuners).\n"
            u"\n"
            u"  --guard-interval value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Must be one of \"auto\", \"1/32\", \"1/16\", \"1/8\", \"1/4\".\n"
            u"      The default is \"1/32\".\n"
            u"\n"
            u"  --hierarchy value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Must be one of \"auto\", \"none\", \"1\", \"2\", \"4\".\n"
            u"      The default is \"none\".\n"
            u"\n"
            u"  --high-priority-fec value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Error correction for high priority streams.  See option --fec-inner\n"
            u"      for the list of possible values. The default is \"auto\".\n"
            u"\n"
            u"  --lnb string\n"
            u"      Used for DVB-S and DVB-S2 tuners only.\n"
            u"      Description of the LNB, if not a universal LNB. The format of the\n"
            u"      string is \"low_freq[,high_freq[,switch_freq]]\" where all frequencies\n"
            u"      are in MHz. The characteristics of the default universal LNB are\n"
            u"      low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.\n"
            u"\n"
            u"  --low-priority-fec value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Error correction for low priority streams. See option --fec-inner\n"
            u"      for the list of possible values. The default is \"auto\".\n"
            u"\n" +
            UString(_allow_short_options ? u"  -m value\n" : u"") +
            u"  --modulation value\n"
            u"      Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners.\n"
            u"      Modulation type. Must be one of \"QPSK\", \"8-PSK\", \"QAM\" (auto QAM),\n"
            u"      \"16-QAM\", \"32-QAM\", \"64-QAM\", \"128-QAM\", \"256-QAM\", \"8-VSB\", \"16-VSB\".\n"
            u"      The default is \"64-QAM\" for DVB-T and DVB-C, \"QPSK\" for DVB-S2,\n"
            u"      \"8-VSB\" for ATSC.\n"
            u"\n"
            u"  --offset-count value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Specify the number of offsets from the UHF or VHF channel. The default\n"
            u"      is zero. See options --uhf-channel or --vhf-channel.\n"
            u"\n"
            u"  --pilots value\n"
            u"      Used for DVB-S2 tuners only.\n"
            u"      Presence of pilots frames. Must be one of \"auto\", \"on\" or \"off\".\n"
            u"      The default is \"off\".\n"
            u"\n"
            u"  --polarity value\n"
            u"      Used for DVB-S and DVB-S2 tuners only.\n"
            u"      Polarity. Must be one of \"horizontal\", \"vertical\", \"left\" or \"right\".\n"
            u"      For satellite reception use only \"horizontal\" or \"vertical\" (the default\n"
            u"      is \"vertical\").\n"
            u"\n"
            u"  --roll-off value\n"
            u"      Used for DVB-S2 tuners only.\n"
            u"      Roll-off factor. Must be one of \"auto\", \"0.35\", \"0.25\", \"0.20\".\n"
            u"      The default is \"0.35\" (implied for DVB-S, default for DVB-S2).\n"
            u"\n"
            u"  --satellite-number value\n"
            u"      Used for DVB-S and DVB-S2 tuners only.\n"
            u"      Satellite/dish number. Must be 0 to 3 with DiSEqC switches and 0 to 1 for\n"
            u"      non-DiSEqC switches. The default is 0.\n"
            u"\n"
            u"  --spectral-inversion value\n"
            u"      Spectral inversion. Must be one of \"on\", \"off\" or \"auto\". The default\n"
            u"      is \"auto\".\n"
            u"\n" +
            UString(_allow_short_options ? u"  -s value\n" : u"") +
            u"  --symbol-rate value\n"
            u"      Used for DVB-S, DVB-S2 and DVB-C tuners only.\n"
            u"      Symbol rate in symbols/second. The default is\n"
            u"      " + UString::Decimal(TunerParametersDVBS::DEFAULT_SYMBOL_RATE) + u" sym/s for satellite and " +
            UString::Decimal(TunerParametersDVBC::DEFAULT_SYMBOL_RATE) + u" sym/s for cable.\n"
            u"\n"
            u"  --transmission-mode value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Must be one of \"auto\", \"2K\", \"4K\", \"8K\". The default is \"8K\".\n"
            u"\n"
            u"  --uhf-channel value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Specify the UHF channel number of the carrier. Can be used in\n"
            u"      replacement to --frequency. Can be combined with an --offset-count\n"
            u"      option. The resulting frequency is\n"
            u"      306 MHz + (uhf-channel * 8 MHz) + (offset-count * 166.6 kHz).\n"
            u"\n"
            u"  --vhf-channel value\n"
            u"      Used for DVB-T tuners only.\n"
            u"      Specify the VHF channel number of the carrier. Can be used in\n"
            u"      replacement to --frequency. Can be combined with an --offset-count\n"
            u"      option. The resulting frequency is\n"
            u"      142.5 MHz + (vhf-channel * 7 MHz) + (offset-count * 166.6 kHz).\n"
            u"\n"
            u"Tuning options using Linux DVB \"zap\" format:\n"
            u"\n"
            u"  --tune string\n"
            u"      Specifies all tuning information for the transponder in one string.\n"
            u"      As such, this option is incompatible with the individual tuning options,\n"
            u"      except \"local\" (non-transponder) options --lnb and --satellite-number.\n"
            u"      The format of the parameter string depends on the DVB frontend type.\n"
            u"      It is the same format as used in the Linux DVB szap/czap/tzap\n"
            u"      configuration files:\n"
            u"\n"
            u"      Satellite (QPSK): \"freq:pol:satnum:symrate\"\n"
            u"        With freq = frequency in MHz, pol = polarity (either v or h), satnum =\n"
            u"        satellite number, symrate = symbol rate in ksym/s.\n"
            u"\n"
            u"      Cable (QAM): \"freq:inv:symrate:conv:mod\"\n"
            u"        With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,\n"
            u"        INVERSION_ON, INVERSION_AUTO), symrate = symbol rate in sym/s, conv =\n"
            u"        convolutional rate (one of FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4,\n"
            u"        FEC_4_5, FEC_5_6, FEC_6_7, FEC_7_8, FEC_8_9, FEC_AUTO),\n"
            u"        mod = modulation (one of QPSK, QAM_16, QAM_32, QAM_64, QAM_128,\n"
            u"        QAM_256, QAM_AUTO).\n"
            u"\n"
            u"      Terrestrial (OFDM): \"freq:inv:bw:convhp:convlp:modu:mode:guard:hier\"\n"
            u"        With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,\n"
            u"        INVERSION_ON, INVERSION_AUTO), bw = bandwidth (one of BANDWIDTH_8_MHZ,\n"
            u"        BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_AUTO), convhp and convlp =\n"
            u"        convolutional rate for high and low priority (see values in cable),\n"
            u"        modu = modulation (see values in cable), mode = transmission mode\n"
            u"        (one of TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K, \n"
            u"        TRANSMISSION_MODE_AUTO), guard = guard interval (one of\n"
            u"        GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8,\n"
            u"        GUARD_INTERVAL_1_4, GUARD_INTERVAL_AUTO), hier = hierarchy (one of\n"
            u"        HIERARCHY_NONE, HIERARCHY_1, HIERARCHY_2, HIERARCHY_4, HIERARCHY_AUTO).\n"
            u"\n"
            u"Locating the transponder by channel name:\n"
            u"\n" +
            UString(_allow_short_options ? u"  -c name\n" : u"") +
            u"  --channel-transponder name\n"
            u"      Tune to the transponder containing the specified channel. The channel\n"
            u"      name is not case-sensitive and blanks are ignored. The channel is searched\n"
            u"      in a \"zap configuration file\" and the corresponding tuning information\n"
            u"      in this file is used.\n"
            u"\n" +
            UString(_allow_short_options ? u"  -z path\n" : u"") +
            u"  --zap-config-file path\n"
            u"      Zap configuration file to use for option -c or --channel-transponder.\n"
            u"      The format of these text files is specified by the Linux DVB szap, czap\n"
            u"      and tzap utilities. Zap config files can be created using the scandvb\n"
            u"      tool (szap, czap, tzap and scandvb are part of the dvb-apps package).\n"
            u"      The location of the default zap configuration file depends on the system.\n"
            u"      - On Linux, the default file is $HOME/.Xzap/channels.conf, where X is\n"
            u"        either 's' (satellite), 'c' (cable) or 't' (terrestrial), depending\n"
            u"        on the frontend type.\n"
            u"      - On Windows, the default file is %APPDATA%\\tsduck\\Xzap\\channels.conf,\n"
            u"        where X is either 's', 'c' or 't'.\n";
    }

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Open a tuner and configure it according to the parameters in this object.
//----------------------------------------------------------------------------

bool ts::TunerArgs::configureTuner(Tuner& tuner, Report& report) const
{
    if (tuner.isOpen()) {
        report.error(u"DVB tuner is already open");
        return false;
    }

    // Open DVB adapter frontend. Use first device by default.
    if (!tuner.open(device_name, _info_only, report)) {
        return false;
    }

    // Set configuration parameters.
    tuner.setSignalTimeout(signal_timeout);
    if (!tuner.setReceiveTimeout(receive_timeout, report)) {
        tuner.close(NULLREP);
        return false;
    }

#if defined(TS_LINUX)
    tuner.setSignalPoll(Tuner::DEFAULT_SIGNAL_POLL);
    tuner.setDemuxBufferSize(demux_buffer_size);
#elif defined(TS_WINDOWS)
    tuner.setSinkQueueSize(demux_queue_size);
#endif

    return true;
}


//----------------------------------------------------------------------------
// Tune to the specified parameters.
//----------------------------------------------------------------------------

bool ts::TunerArgs::tune(Tuner& tuner, TunerParametersPtr& params, Report& report) const
{
    // Allocate tuning parameters of the appropriate type
    params = TunerParameters::Factory(tuner.tunerType());
    assert(!params.isNull());

    // Tune to transponder if some tune options were specified.
    if (hasTuningInfo()) {

        // Map command line options to actual tuning parameters
        if (!params->fromTunerArgs(*this, report)) {
            return false;
        }
        report.debug("tuning to transponder " + params->toPluginOptions());

        // Tune to transponder
        if (!tuner.tune(*params, report)) {
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Default zap file name for a given tuner type
//----------------------------------------------------------------------------

ts::UString ts::TunerArgs::DefaultZapFile(TunerType tuner_type)
{
    const UChar* file = 0;

#if defined(TS_WINDOWS)
    const UChar* root_env = u"APPDATA";
    switch (tuner_type) {
        case DVB_S: file = u"\\tsduck\\szap\\channels.conf"; break;
        case DVB_C: file = u"\\tsduck\\czap\\channels.conf"; break;
        case DVB_T: file = u"\\tsduck\\tzap\\channels.conf"; break;
        default: return UString();
    }
#else
    const UChar* root_env = u"HOME";
    switch (tuner_type) {
        case DVB_S: file = u"/.szap/channels.conf"; break;
        case DVB_C: file = u"/.czap/channels.conf"; break;
        case DVB_T: file = u"/.tzap/channels.conf"; break;
        default: return UString();
    }
#endif

    const UString root_path(GetEnvironment(root_env));
    return root_path.empty() ? UString() : UString(root_path) + UString(file);
}
