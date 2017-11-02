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
#include "tsDecimal.h"
#include "tsFormat.h"
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
    if (args.present("adapter") && args.present("device-name")) {
        args.error("choose either --adapter or --device-name but not both");
    }
    if (args.present("device-name")) {
        device_name = args.value("device-name");
    }
    else if (args.present("adapter")) {
        const int adapter = args.intValue("adapter", 0);
#if defined(TS_LINUX)
        device_name = Format("/dev/dvb/adapter%d", adapter);
#elif defined(TS_WINDOWS)
        device_name = Format(":%d", adapter);
#else
        // Does not mean anything, just for error messages.
        device_name = Format("DVB adapter %d", adapter);
#endif
    }

    // Tuning options.
    if (!_info_only) {
        bool got_one = false;

        // Reception parameters.
        signal_timeout = args.intValue<MilliSecond>("signal-timeout", Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) * 1000;
        receive_timeout = args.intValue<MilliSecond>("receive-timeout", 0);
#if defined(TS_LINUX)
        demux_buffer_size = args.intValue<size_t>("demux-buffer-size", Tuner::DEFAULT_DEMUX_BUFFER_SIZE);
#elif defined(TS_WINDOWS)
        demux_queue_size = args.intValue<size_t>("demux-queue-size", Tuner::DEFAULT_SINK_QUEUE_SIZE);
#endif

        // Carrier frequency
        if (args.present("frequency") + args.present("uhf-channel") + args.present("vhf-channel") > 1) {
            args.error("options --frequency, --uhf-channel and --vhf-channel are mutually exclusive");
        }
        else if (args.present("frequency")) {
            got_one = true;
            frequency = args.intValue<uint64_t>("frequency");
        }
        else if (args.present("uhf-channel")) {
            got_one = true;
            frequency = UHF::Frequency(args.intValue<int>("uhf-channel", 0), args.intValue<int>("offset-count", 0));
        }
        else if (args.present("vhf-channel")) {
            got_one = true;
            frequency = VHF::Frequency(args.intValue<int>("vhf-channel", 0), args.intValue<int>("offset-count", 0));
        }

        // Other individual tuning options
        if (args.present("symbol-rate")) {
            got_one = true;
            symbol_rate = args.intValue<uint32_t>("symbol-rate");
        }
        if (args.present("polarity")) {
            got_one = true;
            polarity = args.enumValue<Polarization>("polarity");
        }
        if (args.present("spectral-inversion")) {
            got_one = true;
            inversion = args.enumValue<SpectralInversion>("spectral-inversion");
        }
        if (args.present("fec-inner")) {
            got_one = true;
            inner_fec = args.enumValue<InnerFEC>("fec-inner");
        }
        if (args.present("modulation")) {
            got_one = true;
            modulation = args.enumValue<Modulation>("modulation");
        }
        if (args.present("bandwidth")) {
            got_one = true;
            bandwidth = args.enumValue<BandWidth>("bandwidth");
        }
        if (args.present("high-priority-fec")) {
            got_one = true;
            fec_hp = args.enumValue<InnerFEC>("high-priority-fec");
        }
        if (args.present("low-priority-fec")) {
            got_one = true;
            fec_lp = args.enumValue<InnerFEC>("low-priority-fec");
        }
        if (args.present("transmission-mode")) {
            got_one = true;
            transmission_mode = args.enumValue<TransmissionMode>("transmission-mode");
        }
        if (args.present("guard-interval")) {
            got_one = true;
            guard_interval = args.enumValue<GuardInterval>("guard-interval");
        }
        if (args.present("hierarchy")) {
            got_one = true;
            hierarchy = args.enumValue<Hierarchy>("hierarchy");
        }
        if (args.present("delivery-system")) {
            got_one = true;
            delivery_system = args.enumValue<DeliverySystem>("delivery-system");
        }
        if (args.present("pilots")) {
            got_one = true;
            pilots = args.enumValue<Pilot>("pilots");
        }
        if (args.present("roll-off")) {
            got_one = true;
            roll_off = args.enumValue<RollOff>("roll-off");
        }

        // Local options (not related to transponder)
        if (args.present("lnb")) {
            std::string s(args.value("lnb"));
            LNB l(s);
            if (!l.isValid()) {
                args.error("invalid LNB description " + s);
            }
            else {
                lnb = l;
            }
        }
        if (args.present("satellite-number")) {
            satellite_number = args.intValue<size_t>("satellite-number");
        }

        // Locating the transponder by channel
        if (args.present("channel-transponder")) {
            channel_name = args.value("channel-transponder");
        }
        if (args.present("zap-config-file")) {
            zap_file_name = args.value("zap-config-file");
        }

        // Zap format tune specification
        if (args.present("tune")) {
            zap_specification = args.value("tune");
        }

        // Mutually exclusive methods of locating the channels
        if (got_one + channel_name.set() + zap_specification.set() > 1) {
            args.error("--tune, --channel-transponder and individual tuning options are incompatible");
        }
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TunerArgs::defineOptions(Args& args) const
{
    // Tuner identification.
    args.option("adapter", _allow_short_options ? 'a' : 0, Args::UNSIGNED);
    args.option("device-name", _allow_short_options ? 'd' : 0, Args::STRING);

    if (!_info_only) {

        // Reception parameters.
        args.option("receive-timeout", 0, Args::UNSIGNED);
        args.option("signal-timeout", 0, Args::UNSIGNED);
#if defined(TS_LINUX)
        args.option("demux-buffer-size", 0, Args::UNSIGNED);
#elif defined(TS_WINDOWS)
        args.option("demux-queue-size", 0, Args::UNSIGNED);
#endif

        // Tuning options.
        args.option("bandwidth", 0, BandWidthEnum);
        args.option("channel-transponder", _allow_short_options ? 'c' : 0, Args::STRING);
        args.option("delivery-system", 0, DeliverySystemEnum);
        args.option("fec-inner", 0, InnerFECEnum);
        args.option("frequency", _allow_short_options ? 'f' : 0, Args::UNSIGNED);
        args.option("guard-interval", 0, GuardIntervalEnum);
        args.option("hierarchy", 0, HierarchyEnum);
        args.option("high-priority-fec", 0, InnerFECEnum);
        args.option("lnb", 0, Args::STRING);
        args.option("low-priority-fec", 0, InnerFECEnum);
        args.option("modulation", _allow_short_options ? 'm' : 0, ModulationEnum);
        args.option("offset-count", 0, Args::INTEGER, 0, 1, -3, 3);
        args.option("pilots", 0, PilotEnum);
        args.option("polarity", 0, PolarizationEnum);
        args.option("roll-off", 0, RollOffEnum);
        args.option("satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
        args.option("spectral-inversion", 0, SpectralInversionEnum);
        args.option("symbol-rate", _allow_short_options ? 's' : 0, Args::UNSIGNED);
        args.option("transmission-mode", 0, TransmissionModeEnum);
        args.option("tune", 0, Args::STRING);
        args.option("uhf-channel", 0, Args::INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
        args.option("vhf-channel", 0, Args::INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);
        args.option("zap-config-file", _allow_short_options ? 'z' : 0, Args::STRING);
    }
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TunerArgs::addHelp(Args& args) const
{
    std::string help =
        "\n"
        "Tuner identification:\n"
        "\n" +
        std::string(_allow_short_options ? "  -a N\n" : "") +
        "  --adapter N\n"
#if defined(TS_LINUX)
        "      Specifies the Linux DVB adapter N (/dev/dvb/adapterN).\n"
#elif defined(TS_WINDOWS)
        "      Specifies the Nth DVB adapter in the system.\n"
#endif
        "      This option can be used instead of device name.\n"
        "\n" +
        std::string(_allow_short_options ? "  -d \"name\"\n" : "") +
        "  --device-name \"name\"\n"
#if defined (TS_LINUX)
        "      Specify the DVB receiver device name, /dev/dvb/adapterA[:F[:M[:V]]]\n"
        "      where A = adapter number, F = frontend number (default: 0), M = demux\n"
        "      number (default: 0), V = dvr number (default: 0).\n"
#elif defined (TS_WINDOWS)
        "      Specify the DVB receiver device name. This is a DirectShow/BDA tuner\n"
        "      filter name (not case sensitive, blanks are ignored).\n"
#endif
        "      By default, the first DVB receiver device is used.\n"
        "      Use the tslsdvb utility to list all DVB devices.\n";

    if (!_info_only) {
        help +=
            "\n"
            "Tuner reception parameters:\n"
#if defined (TS_LINUX)
            "\n"
            "  --demux-buffer-size value\n"
            "      Default buffer size, in bytes, of the demux device.\n"
            "      The default is 1 MB.\n"
#elif defined (TS_WINDOWS)
            "\n"
            "  --demux-queue-size value\n"
            "      Specify the maximum number of media samples in the queue between the\n"
            "      DirectShow capture thread and the input plugin thread. The default is\n"
            "      " + Decimal(Tuner::DEFAULT_SINK_QUEUE_SIZE) + " media samples.\n"
#endif
            "\n"
            "  --receive-timeout milliseconds\n"
            "      Specifies the timeout, in milliseconds, for each receive operation.\n"
            "      To disable the timeout and wait indefinitely for packets, specify zero.\n"
            "      This is the default.\n"
            "\n"
            "  --signal-timeout seconds\n"
            "      Specifies the timeout, in seconds, for DVB signal locking. If no signal\n"
            "      is detected after this timeout, the command aborts. To disable the\n"
            "      timeout and wait indefinitely for the signal, specify zero. The default\n"
            "      is " + Decimal(Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) + " seconds.\n"
            "\n"
            "Tuning:\n"
            "\n"
            "  By default, no tuning is performed on the DVB frontend. The transponder\n"
            "  on which the frontend is currently tuned is used. There are three ways to\n"
            "  specify a new transponder: specifying individual tuning options, a global\n"
            "  tuning information string using the Linux DVB \"zap\" format, the name of\n"
            "  a channel contained in the transponder (with appropriate channel\n"
            "  configuration files).\n"
            "\n"
            "Individual tuning options:\n"
            "\n"
            "  --bandwidth value\n"
            "      Used for DVB-T tuners only.\n"
            "      Must be one of \"auto\", \"8-MHz\", \"7-MHz\", \"6-MHz\", \"5-MHz\".\n"
            "      The default is \"8-MHz\".\n"
            "\n"
            "  --delivery-system value\n"
            "      Used for DVB-S and DVB-S2 tuners only.\n"
            "      Which delivery system to use. Must be one of \"DVB-S\", \"DVB-S2\".\n"
            "      The default is \"DVB-S\".\n"
            "\n"
            "  --fec-inner value\n"
            "      Used for DVB-S, DVB-S2 and DVB-C tuners only.\n"
            "      Inner Forward Error Correction. Must be one of \"none\", \"auto\", \"1/2\",\n"
            "      \"1/3\", \"1/4\", \"2/3\", \"2/5\", \"3/4\", \"3/5\", \"4/5\", \"5/6\", \"5/11\",\n"
            "      \"6/7\", \"7/8\", \"8/9\", \"9/10\". The default is \"auto\".\n"
            "\n" +
            std::string(_allow_short_options ? "  -f value\n" : "") +
            "  --frequency value\n"
            "      Carrier frequency in Hz (all tuners).\n"
            "\n"
            "  --guard-interval value\n"
            "      Used for DVB-T tuners only.\n"
            "      Must be one of \"auto\", \"1/32\", \"1/16\", \"1/8\", \"1/4\".\n"
            "      The default is \"1/32\".\n"
            "\n"
            "  --hierarchy value\n"
            "      Used for DVB-T tuners only.\n"
            "      Must be one of \"auto\", \"none\", \"1\", \"2\", \"4\".\n"
            "      The default is \"none\".\n"
            "\n"
            "  --high-priority-fec value\n"
            "      Used for DVB-T tuners only.\n"
            "      Error correction for high priority streams.  See option --fec-inner\n"
            "      for the list of possible values. The default is \"auto\".\n"
            "\n"
            "  --lnb string\n"
            "      Used for DVB-S and DVB-S2 tuners only.\n"
            "      Description of the LNB, if not a universal LNB. The format of the\n"
            "      string is \"low_freq[,high_freq[,switch_freq]]\" where all frequencies\n"
            "      are in MHz. The characteristics of the default universal LNB are\n"
            "      low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.\n"
            "\n"
            "  --low-priority-fec value\n"
            "      Used for DVB-T tuners only.\n"
            "      Error correction for low priority streams. See option --fec-inner\n"
            "      for the list of possible values. The default is \"auto\".\n"
            "\n" +
            std::string(_allow_short_options ? "  -m value\n" : "") +
            "  --modulation value\n"
            "      Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners.\n"
            "      Modulation type. Must be one of \"QPSK\", \"8-PSK\", \"QAM\" (auto QAM),\n"
            "      \"16-QAM\", \"32-QAM\", \"64-QAM\", \"128-QAM\", \"256-QAM\", \"8-VSB\", \"16-VSB\".\n"
            "      The default is \"64-QAM\" for DVB-T and DVB-C, \"QPSK\" for DVB-S2,\n"
            "      \"8-VSB\" for ATSC.\n"
            "\n"
            "  --offset-count value\n"
            "      Used for DVB-T tuners only.\n"
            "      Specify the number of offsets from the UHF or VHF channel. The default\n"
            "      is zero. See options --uhf-channel or --vhf-channel.\n"
            "\n"
            "  --pilots value\n"
            "      Used for DVB-S2 tuners only.\n"
            "      Presence of pilots frames. Must be one of \"auto\", \"on\" or \"off\".\n"
            "      The default is \"off\".\n"
            "\n"
            "  --polarity value\n"
            "      Used for DVB-S and DVB-S2 tuners only.\n"
            "      Polarity. Must be one of \"horizontal\", \"vertical\", \"left\" or \"right\".\n"
            "      For satellite reception use only \"horizontal\" or \"vertical\" (the default\n"
            "      is \"vertical\").\n"
            "\n"
            "  --roll-off value\n"
            "      Used for DVB-S2 tuners only.\n"
            "      Roll-off factor. Must be one of \"auto\", \"0.35\", \"0.25\", \"0.20\".\n"
            "      The default is \"0.35\" (implied for DVB-S, default for DVB-S2).\n"
            "\n"
            "  --satellite-number value\n"
            "      Used for DVB-S and DVB-S2 tuners only.\n"
            "      Satellite/dish number. Must be 0 to 3 with DiSEqC switches and 0 to 1 for\n"
            "      non-DiSEqC switches. The default is 0.\n"
            "\n"
            "  --spectral-inversion value\n"
            "      Spectral inversion. Must be one of \"on\", \"off\" or \"auto\". The default\n"
            "      is \"auto\".\n"
            "\n" +
            std::string(_allow_short_options ? "  -s value\n" : "") +
            "  --symbol-rate value\n"
            "      Used for DVB-S, DVB-S2 and DVB-C tuners only.\n"
            "      Symbol rate in symbols/second. The default is\n"
            "      " + Decimal(TunerParametersDVBS::DEFAULT_SYMBOL_RATE) + " sym/s for satellite and " +
            Decimal(TunerParametersDVBC::DEFAULT_SYMBOL_RATE) + " sym/s for cable.\n"
            "\n"
            "  --transmission-mode value\n"
            "      Used for DVB-T tuners only.\n"
            "      Must be one of \"auto\", \"2K\", \"4K\", \"8K\". The default is \"8K\".\n"
            "\n"
            "  --uhf-channel value\n"
            "      Used for DVB-T tuners only.\n"
            "      Specify the UHF channel number of the carrier. Can be used in\n"
            "      replacement to --frequency. Can be combined with an --offset-count\n"
            "      option. The resulting frequency is\n"
            "      306 MHz + (uhf-channel * 8 MHz) + (offset-count * 166.6 kHz).\n"
            "\n"
            "  --vhf-channel value\n"
            "      Used for DVB-T tuners only.\n"
            "      Specify the VHF channel number of the carrier. Can be used in\n"
            "      replacement to --frequency. Can be combined with an --offset-count\n"
            "      option. The resulting frequency is\n"
            "      142.5 MHz + (vhf-channel * 7 MHz) + (offset-count * 166.6 kHz).\n"
            "\n"
            "Tuning options using Linux DVB \"zap\" format:\n"
            "\n"
            "  --tune string\n"
            "      Specifies all tuning information for the transponder in one string.\n"
            "      As such, this option is incompatible with the individual tuning options,\n"
            "      except \"local\" (non-transponder) options --lnb and --satellite-number.\n"
            "      The format of the parameter string depends on the DVB frontend type.\n"
            "      It is the same format as used in the Linux DVB szap/czap/tzap\n"
            "      configuration files:\n"
            "\n"
            "      Satellite (QPSK): \"freq:pol:satnum:symrate\"\n"
            "        With freq = frequency in MHz, pol = polarity (either v or h), satnum =\n"
            "        satellite number, symrate = symbol rate in ksym/s.\n"
            "\n"
            "      Cable (QAM): \"freq:inv:symrate:conv:mod\"\n"
            "        With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,\n"
            "        INVERSION_ON, INVERSION_AUTO), symrate = symbol rate in sym/s, conv =\n"
            "        convolutional rate (one of FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4,\n"
            "        FEC_4_5, FEC_5_6, FEC_6_7, FEC_7_8, FEC_8_9, FEC_AUTO),\n"
            "        mod = modulation (one of QPSK, QAM_16, QAM_32, QAM_64, QAM_128,\n"
            "        QAM_256, QAM_AUTO).\n"
            "\n"
            "      Terrestrial (OFDM): \"freq:inv:bw:convhp:convlp:modu:mode:guard:hier\"\n"
            "        With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,\n"
            "        INVERSION_ON, INVERSION_AUTO), bw = bandwidth (one of BANDWIDTH_8_MHZ,\n"
            "        BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_AUTO), convhp and convlp =\n"
            "        convolutional rate for high and low priority (see values in cable),\n"
            "        modu = modulation (see values in cable), mode = transmission mode\n"
            "        (one of TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K, \n"
            "        TRANSMISSION_MODE_AUTO), guard = guard interval (one of\n"
            "        GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8,\n"
            "        GUARD_INTERVAL_1_4, GUARD_INTERVAL_AUTO), hier = hierarchy (one of\n"
            "        HIERARCHY_NONE, HIERARCHY_1, HIERARCHY_2, HIERARCHY_4, HIERARCHY_AUTO).\n"
            "\n"
            "Locating the transponder by channel name:\n"
            "\n" +
            std::string(_allow_short_options ? "  -c name\n" : "") +
            "  --channel-transponder name\n"
            "      Tune to the transponder containing the specified channel. The channel\n"
            "      name is not case-sensitive and blanks are ignored. The channel is searched\n"
            "      in a \"zap configuration file\" and the corresponding tuning information\n"
            "      in this file is used.\n"
            "\n" +
            std::string(_allow_short_options ? "  -z path\n" : "") +
            "  --zap-config-file path\n"
            "      Zap configuration file to use for option -c or --channel-transponder.\n"
            "      The format of these text files is specified by the Linux DVB szap, czap\n"
            "      and tzap utilities. Zap config files can be created using the scandvb\n"
            "      tool (szap, czap, tzap and scandvb are part of the dvb-apps package).\n"
            "      The location of the default zap configuration file depends on the system.\n"
            "      - On Linux, the default file is $HOME/.Xzap/channels.conf, where X is\n"
            "        either 's' (satellite), 'c' (cable) or 't' (terrestrial), depending\n"
            "        on the frontend type.\n"
            "      - On Windows, the default file is %APPDATA%\\tsduck\\Xzap\\channels.conf,\n"
            "        where X is either 's', 'c' or 't'.\n";
    }

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Open a tuner and configure it according to the parameters in this object.
//----------------------------------------------------------------------------

bool ts::TunerArgs::configureTuner(Tuner& tuner, ReportInterface& report) const
{
    if (tuner.isOpen()) {
        report.error("DVB tuner is already open");
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

bool ts::TunerArgs::tune(Tuner& tuner, TunerParametersPtr& params, ReportInterface& report) const
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

std::string ts::TunerArgs::DefaultZapFile(TunerType tuner_type)
{
    const char* file;

#if defined(TS_WINDOWS)
    const char* root_env = "APPDATA";
    switch (tuner_type) {
        case DVB_S: file = "\\tsduck\\szap\\channels.conf"; break;
        case DVB_C: file = "\\tsduck\\czap\\channels.conf"; break;
        case DVB_T: file = "\\tsduck\\tzap\\channels.conf"; break;
        default: return "";
    }
#else
    const char* root_env = "HOME";
    switch (tuner_type) {
        case DVB_S: file = "/.szap/channels.conf"; break;
        case DVB_C: file = "/.czap/channels.conf"; break;
        case DVB_T: file = "/.tzap/channels.conf"; break;
        default: return "";
    }
#endif

    std::string root_path(GetEnvironment(root_env));
    return root_path.empty() ? "" : root_path + file;
}
