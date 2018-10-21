//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2018, Thierry Lelegard
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
    plp(),
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
    plp.reset();
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
        args.error(u"choose either --adapter or --device-name but not both");
    }
    if (args.present(u"device-name")) {
        device_name = args.value(u"device-name");
    }
    else if (args.present(u"adapter")) {
        const int adapter = args.intValue(u"adapter", 0);
#if defined(TS_LINUX)
        device_name.format(u"/dev/dvb/adapter%d", {adapter});
#elif defined(TS_WINDOWS)
        device_name.format(u":%d", {adapter});
#else
        // Does not mean anything, just for error messages.
        device_name.format(u"DVB adapter %d", {adapter});
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
            args.error(u"options --frequency, --uhf-channel and --vhf-channel are mutually exclusive");
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
        if (args.present(u"plp")) {
            got_one = true;
            plp = args.enumValue<PLP>(u"plp");
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
    args.help(u"adapter", u"N",
#if defined(TS_LINUX)
              u"Specifies the Linux DVB adapter N (/dev/dvb/adapterN). "
#elif defined(TS_WINDOWS)
              u"Specifies the Nth DVB adapter in the system. "
#endif
              u"This option can be used instead of device name.");

    args.option(u"device-name", _allow_short_options ? u'd' : 0, Args::STRING);
    args.help(u"device-name", u"name",
#if defined (TS_LINUX)
              u"Specify the DVB receiver device name, /dev/dvb/adapterA[:F[:M[:V]]] "
              u"where A = adapter number, F = frontend number (default: 0), M = demux "
              u"number (default: 0), V = dvr number (default: 0). "
#elif defined (TS_WINDOWS)
              u"Specify the DVB receiver device name. This is a DirectShow/BDA tuner "
              u"filter name (not case sensitive, blanks are ignored). "
#endif
              u"By default, the first DVB receiver device is used. "
              u"Use the tslsdvb utility to list all DVB devices. ");

    // All other parameters are used to control the tuner.
    if (!_info_only) {

        // Reception parameters.
        args.option(u"receive-timeout", 0, Args::UNSIGNED);
        args.help(u"receive-timeout", u"milliseconds",
                  u"Specifies the timeout, in milliseconds, for each receive operation. "
                  u"To disable the timeout and wait indefinitely for packets, specify zero. "
                  u"This is the default.");

        args.option(u"signal-timeout", 0, Args::UNSIGNED);
        args.help(u"signal-timeout", u"seconds",
                  u"Specifies the timeout, in seconds, for DVB signal locking. If no signal "
                  u"is detected after this timeout, the command aborts. To disable the "
                  u"timeout and wait indefinitely for the signal, specify zero. The default "
                  u"is " + UString::Decimal(Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) + u" seconds.");

#if defined(TS_LINUX)

        args.option(u"demux-buffer-size", 0, Args::UNSIGNED);
        args.help(u"demux-buffer-size",
                  u"Default buffer size, in bytes, of the demux device. "
                  u"The default is 1 MB.");

#elif defined(TS_WINDOWS)

        args.option(u"demux-queue-size", 0, Args::UNSIGNED);
        args.help(u"demux-queue-size",
                  u"Specify the maximum number of media samples in the queue between the "
                  u"DirectShow capture thread and the input plugin thread. The default is " +
                  UString::Decimal(Tuner::DEFAULT_SINK_QUEUE_SIZE) + u" media samples.");

#endif

        // Tuning options.
        args.option(u"bandwidth", 0, BandWidthEnum);
        args.help(u"bandwidth",
                  u"Used for DVB-T/T2 tuners only. The default is \"8-MHz\".");

        args.option(u"channel-transponder", _allow_short_options ? 'c' : 0, Args::STRING);
        args.help(u"channel-transponder", u"name",
                  u"Tune to the transponder containing the specified channel. The channel "
                  u"name is not case-sensitive and blanks are ignored. The channel is searched "
                  u"in a \"zap configuration file\" and the corresponding tuning information "
                  u"in this file is used.");

        args.option(u"delivery-system", 0, DeliverySystemEnum);
        args.help(u"delivery-system",
                  u"Used for DVB-S and DVB-S2 tuners only. Which delivery system to use. "
                  u"The default is \"DVB-S\".");

        args.option(u"fec-inner", 0, InnerFECEnum);
        args.help(u"fec-inner",
                  u"Used for DVB-S/S2 and DVB-C tuners only. Inner Forward Error Correction. "
                  u"The default is \"auto\".");

        args.option(u"frequency", _allow_short_options ? 'f' : 0, Args::UNSIGNED);
        args.help(u"frequency", u"Carrier frequency in Hz (all tuners). There is no default.");

        args.option(u"guard-interval", 0, GuardIntervalEnum);
        args.help(u"guard-interval", u"Used for DVB-T/T2 tuners only. The default is \"1/32\".");

        args.option(u"hierarchy", 0, HierarchyEnum);
        args.help(u"hierarchy", u"Used for DVB-T/T2 tuners only. The default is \"none\".");

        args.option(u"high-priority-fec", 0, InnerFECEnum);
        args.help(u"high-priority-fec",
                  u"Used for DVB-T/T2 tuners only. "
                  u"Error correction for high priority streams. "
                  u"The default is \"auto\".");

        args.option(u"lnb", 0, Args::STRING);
        args.help(u"lnb", u"low_freq[,high_freq,switch_freq]",
                  u"Used for DVB-S and DVB-S2 tuners only. "
                  u"Description of the LNB.  All frequencies are in MHz. "
                  u"low_freq and high_freq are the frequencies of the local oscillators. "
                  u"switch_freq is the limit between the low and high band. "
                  u"high_freq and switch_freq are used for dual-band LNB's only. "
                  u"The default is a universal LNB: low_freq = 9750 MHz, high_freq = 10600 MHz, switch_freq = 11700 MHz.");

        args.option(u"low-priority-fec", 0, InnerFECEnum);
        args.help(u"low-priority-fec",
                  u"Used for DVB-T/T2 tuners only. "
                  u"Error correction for low priority streams. "
                  u"The default is \"auto\".");

        args.option(u"modulation", _allow_short_options ? 'm' : 0, ModulationEnum);
        args.help(u"modulation",
                  u"Used for DVB-C, DVB-T, DVB-S2 and ATSC tuners. "
                  u"Modulation type. "
                  u"The default is \"64-QAM\" for DVB-T/T2 and DVB-C, \"QPSK\" for DVB-S2, \"8-VSB\" for ATSC.");

        args.option(u"offset-count", 0, Args::INTEGER, 0, 1, -3, 3);
        args.help(u"offset-count",
                  u"Used for DVB-T tuners only. "
                  u"Specify the number of offsets from the UHF or VHF channel. The default "
                  u"is zero. See options --uhf-channel or --vhf-channel.");

        args.option(u"pilots", 0, PilotEnum);
        args.help(u"pilots",
                  u"Used for DVB-S2 tuners only. Presence of pilots frames. "
                  u"The default is \"off\". ");

        args.option(u"plp", 0, Args::UINT8);
        args.help(u"plp",
                  u"Used for DVB-T2 tuners only. "
                  u"Physical Layer Pipe (PLP) number to select, from 0 to 255. "
                  u"The default is to keep the entire stream, without PLP selection. "
                  u"Warning: this option is supported on Linux only.");

        args.option(u"polarity", 0, PolarizationEnum);
        args.help(u"polarity",
                  u"Used for DVB-S and DVB-S2 tuners only. "
                  u"Polarity. The default is \"vertical\".");

        args.option(u"roll-off", 0, RollOffEnum);
        args.help(u"roll-off",
                  u"Used for DVB-S2 tuners only. Roll-off factor. "
                  u"The default is \"0.35\" (implied for DVB-S, default for DVB-S2).");

        args.option(u"satellite-number", 0, Args::INTEGER, 0, 1, 0, 3);
        args.help(u"satellite-number",
                  u"Used for DVB-S and DVB-S2 tuners only. "
                  u"Satellite/dish number. Must be 0 to 3 with DiSEqC switches and 0 to 1 for "
                  u"non-DiSEqC switches. The default is 0. ");

        args.option(u"spectral-inversion", 0, SpectralInversionEnum);
        args.help(u"spectral-inversion",
                  u"Spectral inversion. The default is \"auto\".");

        args.option(u"symbol-rate", _allow_short_options ? 's' : 0, Args::UNSIGNED);
        args.help(u"symbol-rate",
                  u"Used for DVB-S, DVB-S2 and DVB-C tuners only. "
                  u"Symbol rate in symbols/second. The default is " +
                  UString::Decimal(TunerParametersDVBS::DEFAULT_SYMBOL_RATE) +
                  u" sym/s for satellite and " +
                  UString::Decimal(TunerParametersDVBC::DEFAULT_SYMBOL_RATE) +
                  u" sym/s for cable. ");

        args.option(u"transmission-mode", 0, TransmissionModeEnum);
        args.help(u"transmission-mode",
                  u"Used for DVB-T tuners only. Transmission mode. The default is \"8K\".");

        args.option(u"tune", 0, Args::STRING);
        args.help(u"tune", u"string",
                  u"Tuning options using Linux DVB \"zap\" format.\n\n"
                  u"By default, no tuning is performed on the DVB frontend. The transponder "
                  u"on which the frontend is currently tuned is used. There are three ways to "
                  u"specify a new transponder: specifying individual tuning options, a global "
                  u"tuning information string using the Linux DVB \"zap\" format, the name of "
                  u"a channel contained in the transponder (with appropriate channel "
                  u"configuration files).\n\n"
                  u"The --tune string specifies all tuning information for the transponder in one string. "
                  u"As such, this option is incompatible with the individual tuning options, "
                  u"except \"local\" (non-transponder) options --lnb and --satellite-number. "
                  u"The format of the parameter string depends on the DVB frontend type. "
                  u"It is the same format as used in the Linux DVB szap/czap/tzap "
                  u"configuration files:\n\n"
                  u"Satellite (QPSK): \"freq:pol:satnum:symrate\". "
                  u"With freq = frequency in MHz, pol = polarity (either v or h), satnum = "
                  u"satellite number, symrate = symbol rate in ksym/s.\n\n"
                  u"Cable (QAM): \"freq:inv:symrate:conv:mod\". "
                  u"With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF, "
                  u"INVERSION_ON, INVERSION_AUTO), symrate = symbol rate in sym/s, conv = "
                  u"convolutional rate (one of FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4, "
                  u"FEC_4_5, FEC_5_6, FEC_6_7, FEC_7_8, FEC_8_9, FEC_AUTO), "
                  u"mod = modulation (one of QPSK, QAM_16, QAM_32, QAM_64, QAM_128, "
                  u"QAM_256, QAM_AUTO).\n\n"
                  u"Terrestrial (OFDM): \"freq:inv:bw:convhp:convlp:modu:mode:guard:hier\". "
                  u"With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF, "
                  u"INVERSION_ON, INVERSION_AUTO), bw = bandwidth (one of BANDWIDTH_8_MHZ, "
                  u"BANDWIDTH_7_MHZ, BANDWIDTH_6_MHZ, BANDWIDTH_AUTO), convhp and convlp = "
                  u"convolutional rate for high and low priority (see values in cable), "
                  u"modu = modulation (see values in cable), mode = transmission mode "
                  u"(one of TRANSMISSION_MODE_2K, TRANSMISSION_MODE_8K,  "
                  u"TRANSMISSION_MODE_AUTO), guard = guard interval (one of "
                  u"GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_8, "
                  u"GUARD_INTERVAL_1_4, GUARD_INTERVAL_AUTO), hier = hierarchy (one of "
                  u"HIERARCHY_NONE, HIERARCHY_1, HIERARCHY_2, HIERARCHY_4, HIERARCHY_AUTO).");

        args.option(u"uhf-channel", 0, Args::INTEGER, 0, 1, UHF::FIRST_CHANNEL, UHF::LAST_CHANNEL);
        args.help(u"uhf-channel",
                  u"Used for DVB-T tuners only. "
                  u"Specify the UHF channel number of the carrier. Can be used in "
                  u"replacement to --frequency. Can be combined with an --offset-count "
                  u"option. The resulting frequency is "
                  u"306 MHz + (uhf-channel * 8 MHz) + (offset-count * 166.6 kHz).");

        args.option(u"vhf-channel", 0, Args::INTEGER, 0, 1, VHF::FIRST_CHANNEL, VHF::LAST_CHANNEL);
        args.help(u"vhf-channel",
                  u"Used for DVB-T tuners only. "
                  u"Specify the VHF channel number of the carrier. Can be used in "
                  u"replacement to --frequency. Can be combined with an --offset-count "
                  u"option. The resulting frequency is "
                  u"142.5 MHz + (vhf-channel * 7 MHz) + (offset-count * 166.6 kHz).");

        args.option(u"zap-config-file", _allow_short_options ? 'z' : 0, Args::STRING);
        args.help(u"zap-config-file",
                  u"Zap configuration file to use for option -c or --channel-transponder. "
                  u"The format of these text files is specified by the Linux DVB szap, czap "
                  u"and tzap utilities. Zap config files can be created using the scandvb "
                  u"tool (szap, czap, tzap and scandvb are part of the dvb-apps package). "
                  u"The location of the default zap configuration file depends on the system."
#if defined(TS_LINUX)
                  u" On Linux, the default file is $HOME/.Xzap/channels.conf, where X is either 's' "
                  u"(satellite), 'c' (cable) or 't' (terrestrial), depending on the frontend type."
#elif defined(TS_WINDOWS)
                  u" On Windows, the default file is %APPDATA%\\tsduck\\Xzap\\channels.conf, "
                  u"where X is either 's', 'c' or 't'."
#endif
                  );
    }
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
        report.debug(u"tuning to transponder " + params->toPluginOptions());

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
    const UChar* file = nullptr;

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
