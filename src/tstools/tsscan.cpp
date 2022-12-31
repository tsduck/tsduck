//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//
//  DVB network scanning utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsSignalState.h"
#include "tsModulation.h"
#include "tsHFBand.h"
#include "tsTSScanner.h"
#include "tsChannelFile.h"
#include "tsNIT.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"
#include "tsFileUtils.h"
TS_MAIN(MainCode);

#define DEFAULT_PSI_TIMEOUT   10000 // ms
#define DEFAULT_MIN_STRENGTH  10
#define OFFSET_EXTEND         3


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class ScanOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(ScanOptions);
    public:
        ScanOptions(int argc, char *argv[]);

        ts::DuckContext   duck;
        ts::TunerArgs     tuner_args;
        bool              uhf_scan;
        bool              vhf_scan;
        bool              nit_scan;
        bool              no_offset;
        bool              use_best_strength;
        uint32_t          first_channel;
        uint32_t          last_channel;
        int32_t           first_offset;
        int32_t           last_offset;
        int64_t           min_strength;
        bool              show_modulation;
        bool              list_services;
        bool              global_services;
        ts::MilliSecond   psi_timeout;
        const ts::HFBand* hfband;
        ts::UString       channel_file;
        bool              update_channel_file;
        bool              default_channel_file;
    };
}

ScanOptions::ScanOptions(int argc, char *argv[]) :
    Args(u"Scan a DTV network frequencies and services", u"[options]"),
    duck(this),
    tuner_args(false),
    uhf_scan(false),
    vhf_scan(false),
    nit_scan(false),
    no_offset(false),
    use_best_strength(false),
    first_channel(0),
    last_channel(0),
    first_offset(0),
    last_offset(0),
    min_strength(0),
    show_modulation(false),
    list_services(false),
    global_services(false),
    psi_timeout(0),
    hfband(),
    channel_file(),
    update_channel_file(false),
    default_channel_file(false)
{
    duck.defineArgsForCharset(*this);
    duck.defineArgsForHFBand(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    tuner_args.defineArgs(*this, true);

    setIntro(u"There are three mutually exclusive types of network scanning. "
             u"Exactly one of the following options shall be specified: "
             u"--nit-scan, --uhf-band, --vhf-band.");

    option(u"nit-scan", 'n');
    help(u"nit-scan",
         u"Tuning parameters for a reference transport stream must be present (frequency or channel reference). "
         u"The NIT is read on the specified frequency and a full scan of the corresponding network is performed.");

    option(u"uhf-band", 'u');
    help(u"uhf-band",
         u"Perform a complete UHF-band scanning (DVB-T, ISDB-T or ATSC). "
         u"Use the predefined UHF frequency layout of the specified region (see option --hf-band-region). "
         u"By default, scan the center frequency of each channel only. "
         u"Use option --use-offsets to scan all predefined offsets in each channel.");

    option(u"vhf-band", 'v');
    help(u"vhf-band", u"Perform a complete VHF-band scanning. See also option --uhf-band.");

    option(u"best-quality");
    help(u"best-quality", u"Obsolete option, do not use.");

    option(u"best-strength");
    help(u"best-strength",
         u"With UHF/VHF-band scanning, for each channel, use the offset with the best signal strength. "
         u"By default, use the average of lowest and highest offsets with required minimum strength. "
         u"Note that some tuners cannot report a correct signal strength, making this option useless.");

    option(u"first-channel", 0, POSITIVE);
    help(u"first-channel",
         u"For UHF/VHF-band scanning, specify the first channel to scan (default: lowest channel in band).");

    option(u"first-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"first-offset",
         u"For UHF/VHF-band scanning, specify the first offset to scan on each channel.");

    option(u"global-service-list", 'g');
    help(u"global-service-list",
         u"Same as --service-list but display a global list of services at the end "
         u"of scanning instead of per transport stream.");

    option(u"last-channel", 0, POSITIVE);
    help(u"last-channel",
         u"For UHF/VHF-band scanning, specify the last channel to scan (default: highest channel in band).");

    option(u"last-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"last-offset",
         u"For UHF/VHF-band scanning, specify the last offset to scan on each channel. "
         u"Note that tsscan may scan higher offsets. As long as some signal is found at a "
         u"specified offset, tsscan continues to check up to 3 higher offsets above the \"last\" one. "
         u"This means that if a signal is found at offset +2, offset +3 will be checked anyway, etc. up to offset +5.");

    option(u"min-quality", 0, INT64);
    help(u"min-quality", u"Obsolete option, do not use.");

    option(u"min-strength", 0, INT64);
    help(u"min-strength",
         u"Minimum signal strength. Frequencies with lower signal strength are ignored. "
         u"The value can be in milli-dB or percentage. It depends on the tuner and its driver. "
         u"Check the displayed unit. "
         u"The default is " + ts::UString::Decimal(DEFAULT_MIN_STRENGTH) + u", whatever unit it is.");

    option(u"no-offset");
    help(u"no-offset",
         u"For UHF/VHF-band scanning, scan only the central frequency of each channel. "
         u"This is now the default. Specify option --use-offsets to scan all offsets.");

    option(u"use-offsets");
    help(u"use-offsets",
         u"For UHF/VHF-band scanning, do not scan only the central frequency of each channel. "
         u"Also scan frequencies with offsets. As an example, if a signal is transmitted at offset +1, "
         u"the reception may be successful at offsets -1 to +3 (but not -2 and +4). "
         u"With this option, tsscan checks all offsets and reports that the signal is at offset +1. "
         u"By default, tsscan reports that the signal is found at the central frequency of the channel (offset zero).");

    option(u"psi-timeout", 0, UNSIGNED);
    help(u"psi-timeout", u"milliseconds",
         u"Specifies the timeout, in milli-seconds, for PSI/SI table collection. "
         u"Useful only with --service-list. The default is " +
         ts::UString::Decimal(DEFAULT_PSI_TIMEOUT) + u" milli-seconds.");

    option(u"service-list", 'l');
    help(u"service-list", u"Read SDT of each channel and display the list of services.");

    option(u"show-modulation", 0);
    help(u"show-modulation",
         u"Display modulation parameters when possible. Note that some tuners "
         u"cannot report correct modulation parameters, making this option useless.");

    option(u"save-channels", 0, FILENAME);
    help(u"save-channels", u"filename",
         u"Save the description of all channels in the specified XML file. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"See also option --update-channels.");

    option(u"update-channels", 0, FILENAME);
    help(u"update-channels", u"filename",
         u"Update the description of all channels in the specified XML file. "
         u"The content of each scanned transport stream is replaced in the file. "
         u"If the file does not exist, it is created. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"The location of the default tuning configuration file depends on the system. "
#if defined(TS_LINUX)
         u"On Linux, the default file is $HOME/.tsduck.channels.xml. "
#elif defined(TS_WINDOWS)
         u"On Windows, the default file is %APPDATA%\\tsduck\\channels.xml. "
#endif
         u"See also option --save-channels.");

    analyze(argc, argv);
    duck.loadArgs(*this);
    tuner_args.loadArgs(duck, *this);

    // Type of scanning
    uhf_scan = present(u"uhf-band");
    vhf_scan = present(u"vhf-band");
    nit_scan = present(u"nit-scan");

    if (nit_scan + uhf_scan + vhf_scan != 1) {
        error(u"specify exactly one of --nit-scan, --uhf-band or --vhf-band");
    }
    if (nit_scan && !tuner_args.hasModulationArgs()) {
        error(u"specify the characteristics of the reference TS with --nit-scan");
    }

    // Type of HF band to use.
    hfband = vhf_scan ? duck.vhfBand() : duck.uhfBand();

    use_best_strength = present(u"best-strength");
    first_channel     = intValue(u"first-channel", hfband->firstChannel());
    last_channel      = intValue(u"last-channel", hfband->lastChannel());
    show_modulation   = present(u"show-modulation");
    no_offset         = !present(u"use-offsets");
    first_offset      = no_offset ? 0 : intValue(u"first-offset", hfband->firstOffset(first_channel));
    last_offset       = no_offset ? 0 : intValue(u"last-offset", hfband->lastOffset(first_channel));
    min_strength      = intValue(u"min-strength", DEFAULT_MIN_STRENGTH);
    list_services     = present(u"service-list");
    global_services   = present(u"global-service-list");
    psi_timeout       = intValue<ts::MilliSecond>(u"psi-timeout", DEFAULT_PSI_TIMEOUT);

    const bool save_channel_file = present(u"save-channels");
    update_channel_file = present(u"update-channels");
    channel_file = update_channel_file ? value(u"update-channels") : value(u"save-channels");
    default_channel_file = (save_channel_file || update_channel_file) && (channel_file.empty() || channel_file == u"-");

    if (save_channel_file && update_channel_file) {
        error(u"--save-channels and --update-channels are mutually exclusive");
    }
    else if (default_channel_file) {
        // Use default channel file.
        channel_file = ts::ChannelFile::DefaultFileName();
    }

    exitOnError();
}


//----------------------------------------------------------------------------
// UHF/VHF-band offset scanner: Scan offsets around a specific channel and
// determine offset with the best signal.
//----------------------------------------------------------------------------

class OffsetScanner
{
    TS_NOBUILD_NOCOPY(OffsetScanner);
public:
    // Constructor: Perform scanning. Keep signal tuned on best offset.
    OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, uint32_t channel);

    // Check if signal found and which offset is the best one.
    bool signalFound() const { return _signal_found; }
    uint32_t channel() const { return _channel; }
    int32_t bestOffset() const { return _best_offset; }
    void getTunerParameters(ts::ModulationArgs& params) const { params = _best_params; }

private:
    ScanOptions&       _opt;
    ts::Tuner&         _tuner;
    const uint32_t     _channel;
    bool               _signal_found;
    int32_t            _best_offset;
    int32_t            _lowest_offset;
    int32_t            _highest_offset;
    int64_t            _best_strength;
    int32_t            _best_strength_offset;
    ts::ModulationArgs _best_params;

    // Build tuning parameters for a channel.
    void buildTuningParameters(ts::ModulationArgs& params, int32_t offset);

    // Tune to specified offset. Return false on error.
    bool tune(int32_t offset, ts::ModulationArgs& params);

    // Test the signal at one specific offset. Return true if signal is found.
    bool tryOffset(int32_t offset);
};


//----------------------------------------------------------------------------
// UHF-band offset scanner constructor.
// Perform scanning. Keep signal tuned on best offset
//----------------------------------------------------------------------------

OffsetScanner::OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, uint32_t channel) :
    _opt(opt),
    _tuner(tuner),
    _channel(channel),
    _signal_found(false),
    _best_offset(0),
    _lowest_offset(0),
    _highest_offset(0),
    _best_strength(0),
    _best_strength_offset(0),
    _best_params()
{
    _opt.verbose(u"scanning channel %'d, %'d Hz", {_channel, _opt.hfband->frequency(_channel)});

    if (_opt.no_offset) {
        // Only try the central frequency
        tryOffset(0);
    }
    else {
        // Scan lower offsets in descending order, starting at central frequency
        if (_opt.first_offset <= 0) {
            bool last_ok = false;
            int32_t offset = _opt.last_offset > 0 ? 0 : _opt.last_offset;
            while (offset >= _opt.first_offset - (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset);
                --offset;
            }
        }

        // Scan higher offsets in ascending order, starting after central frequency
        if (_opt.last_offset > 0) {
            bool last_ok = false;
            int32_t offset = _opt.first_offset <= 0 ? 1 : _opt.first_offset;
            while (offset <= _opt.last_offset + (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset);
                ++offset;
            }
        }
    }

    // If signal was found, select best offset
    if (_signal_found) {
        if (_opt.no_offset) {
            // No offset search, the best and only offset is zero.
            _best_offset = 0;
        }
        else if (_opt.use_best_strength && _best_strength > 0) {
            // Signal strength indicator is valid, use offset with best signal strength
            _best_offset = _best_strength_offset;
        }
        else {
            // Default: use average between lowest and highest offsets
            _best_offset = (_lowest_offset + _highest_offset) / 2;
        }

        // Finally, tune back to best offset
        _signal_found = tune(_best_offset, _best_params) && _tuner.getCurrentTuning(_best_params, false);
    }
}


//----------------------------------------------------------------------------
// Build tuning parameters for a channel.
//----------------------------------------------------------------------------

void OffsetScanner::buildTuningParameters(ts::ModulationArgs& params, int32_t offset)
{
    // Force frequency in tuning parameters.
    // Other tuning parameters from command line (or default values).
    params = _opt.tuner_args;
    params.resolveDeliverySystem(_tuner.deliverySystems(), _opt);
    params.frequency = _opt.hfband->frequency(_channel, offset);
    params.setDefaultValues();
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Tune to specified offset. Return false on error.
//----------------------------------------------------------------------------

bool OffsetScanner::tune(int32_t offset, ts::ModulationArgs& params)
{
    buildTuningParameters(params, offset);
    return _tuner.tune(params);
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Test the signal at one specific offset.
//----------------------------------------------------------------------------

bool OffsetScanner::tryOffset(int32_t offset)
{
    _opt.debug(u"trying offset %d", {offset});

    // Tune to transponder and start signal acquisition.
    // Signal locking timeout is applied in start().
    ts::ModulationArgs params;
    if (!tune(offset, params) || !_tuner.start()) {
        return false;
    }

    // Get signal characteristics.
    ts::SignalState state;
    bool ok = _tuner.getSignalState(state) && state.signal_locked;

    // If we get a signal and we wee need to scan offsets, check signal strength.
    // If we don't scan offsets, there is no need to consider signal strength, just use the central offset.
    if (ok && !_opt.no_offset) {

        _opt.verbose(u"%s, %s", {_opt.hfband->description(_channel, offset), state});

        if (state.signal_strength.set()) {
            const int64_t strength = state.signal_strength.value().value;
            if (strength <= _opt.min_strength) {
                // Strength is supported but too low
                ok = false;
            }
            else if (strength > _best_strength) {
                // Best offset so far for signal strength
                _best_strength = strength;
                _best_strength_offset = offset;
                _tuner.getCurrentTuning(params, false);
            }
        }
    }

    if (ok) {
        if (!_signal_found) {
            // First offset with signal on this channel
            _signal_found = true;
            _lowest_offset = _highest_offset = offset;
        }
        else if (offset < _lowest_offset) {
            _lowest_offset = offset;
        }
        else if (offset > _highest_offset) {
            _highest_offset = offset;
        }
    }

    // Stop signal acquisition
    _tuner.stop();

    return ok;
}


//----------------------------------------------------------------------------
// Scanning context.
//----------------------------------------------------------------------------

class ScanContext
{
    TS_NOBUILD_NOCOPY(ScanContext);
public:
    // Contructor.
    ScanContext(ScanOptions&);

    // tsscan main code.
    void main();

private:
    ScanOptions&    _opt;
    ts::Tuner       _tuner;
    ts::ServiceList _services;
    ts::ChannelFile _channels;

    // Analyze a TS and generate relevant info.
    void scanTS(std::ostream& strm, const ts::UString& margin, ts::ModulationArgs& tparams);

    // UHF/VHF-band scanning
    void hfBandScan();

    // NIT-based scanning
    void nitScan();
};

// Contructor.
ScanContext::ScanContext(ScanOptions& opt) :
    _opt(opt),
    _tuner(_opt.duck),
    _services(),
    _channels()
{
}


//----------------------------------------------------------------------------
// Analyze a TS and generate relevant info.
//----------------------------------------------------------------------------

void ScanContext::scanTS(std::ostream& strm, const ts::UString& margin, ts::ModulationArgs& tparams)
{
    const bool get_services = _opt.list_services || _opt.global_services;

    // Collect info from the TS.
    // Use "PAT only" when we do not need the services or channels file.
    ts::TSScanner info(_opt.duck, _tuner, _opt.psi_timeout, !get_services && _opt.channel_file.empty());

    // Get tuning parameters again, as TSScanner waits for a lock.
    // Also keep the original frequency and polarity since satellite tuners can only report the intermediate frequency.
    const ts::Variable<uint64_t> saved_frequency(tparams.frequency);
    const ts::Variable<ts::Polarization> saved_polarity(tparams.polarity);
    info.getTunerParameters(tparams);
    if (!tparams.frequency.set() || tparams.frequency.value() == 0) {
        tparams.frequency = saved_frequency;
    }
    if (!tparams.polarity.set()) {
        tparams.polarity = saved_polarity;
    }

    ts::SafePtr<ts::PAT> pat;
    ts::SafePtr<ts::SDT> sdt;
    ts::SafePtr<ts::NIT> nit;

    info.getPAT(pat);
    info.getSDT(sdt);
    info.getNIT(nit);

    // Get network and TS Id.
    uint16_t ts_id = 0;
    uint16_t net_id = 0;
    if (!pat.isNull()) {
        ts_id = pat->ts_id;
        strm << margin << ts::UString::Format(u"Transport stream id: %d, 0x%X", {ts_id, ts_id}) << std::endl;
    }
    if (!nit.isNull()) {
        net_id = nit->network_id;
    }

    // Reset TS description in channels file.
    ts::ChannelFile::TransportStreamPtr ts_info;
    if (!_opt.channel_file.empty()) {
        ts::ChannelFile::NetworkPtr net_info(_channels.networkGetOrCreate(net_id, ts::TunerTypeOf(tparams.delivery_system.value(ts::DS_UNDEFINED))));
        ts_info = net_info->tsGetOrCreate(ts_id);
        ts_info->clear(); // reset all services in TS.
        ts_info->onid = sdt.isNull() ? 0 : sdt->onetw_id;
        ts_info->tune = tparams;
    }

    // Display modulation parameters
    if (_opt.show_modulation) {
        tparams.display(strm, margin, _opt.maxSeverity());
    }

    // Display or collect services
    if (get_services || !ts_info.isNull()) {
        ts::ServiceList srvlist;
        if (info.getServices(srvlist)) {
            if (!ts_info.isNull()) {
                // Add all services in the channels info.
                ts_info->addServices(srvlist);
            }
            if (_opt.list_services) {
                // Display services for this TS
                srvlist.sort(ts::Service::Sort1);
                strm << std::endl;
                ts::Service::Display(strm, margin, srvlist);
                strm << std::endl;
            }
            if (_opt.global_services) {
                // Add collected services in global service list
                _services.insert(_services.end(), srvlist.begin(), srvlist.end());
            }
        }
    }
}


//----------------------------------------------------------------------------
// UHF/VHF-band scanning
//----------------------------------------------------------------------------

void ScanContext::hfBandScan()
{
    // Loop on all selected UHF channels
    for (uint32_t chan = _opt.first_channel; chan <= _opt.last_channel; ++chan) {

        // Scan all offsets surrounding the channel.
        OffsetScanner offscan(_opt, _tuner, chan);
        if (offscan.signalFound()) {

            // A channel was found, report its characteristics.
            ts::SignalState state;
            _tuner.getSignalState(state);
            std::cout << "* " << _opt.hfband->description(chan, offscan.bestOffset()) << ", " << state.toString() << std::endl;

            // Analyze PSI/SI if required.
            ts::ModulationArgs tparams;
            offscan.getTunerParameters(tparams);
            scanTS(std::cout, u"  ", tparams);
        }
    }
}


//----------------------------------------------------------------------------
// NIT-based scanning
//----------------------------------------------------------------------------

void ScanContext::nitScan()
{
    // Tune to the reference transponder.
    if (!_tuner.tune(_opt.tuner_args)) {
        return;
    }

    // Collect info on reference transponder.
    ts::TSScanner info(_opt.duck, _tuner, _opt.psi_timeout, false);

    // Get the collected NIT
    ts::SafePtr<ts::NIT> nit;
    info.getNIT(nit);
    if (nit.isNull()) {
        _opt.error(u"cannot scan network, no NIT found on specified transponder");
        return;
    }

    // Process each TS descriptor list in the NIT.
    for (const auto& it : nit->transports) {

        const ts::TransportStreamId& tsid(it.first);
        const ts::DescriptorList& dlist(it.second.descs);

        for (size_t i = 0; i < dlist.count(); ++i) {
            // Try to get delivery system information from current descriptor
            ts::ModulationArgs params;
            if (params.fromDeliveryDescriptor(_opt.duck, *dlist[i], tsid.transport_stream_id)) {
                // Got a delivery descriptor, this is the description of one transponder.
                // Copy the local reception parameters (LNB, etc.) from the command line options
                // (we use the same reception equipment).
                params.copyLocalReceptionParameters(_opt.tuner_args);
                // Tune to this transponder.
                _opt.debug(u"* tuning to " + params.toPluginOptions(true));
                if (_tuner.tune(params)) {
                    // Report channel characteristics
                    ts::SignalState state;
                    _tuner.getSignalState(state);
                    std::cout << "* Frequency: " << params.shortDescription(_opt.duck) << ", " << state.toString() << std::endl;
                    // Analyze PSI/SI if required
                    scanTS(std::cout, u"  ", params);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Main code from scan context.
//----------------------------------------------------------------------------

void ScanContext::main()
{
    // Initialize tuner.
    _tuner.setSignalTimeoutSilent(true);
    if (!_opt.tuner_args.configureTuner(_tuner)) {
        return;
    }

    // Pre-load the existing channel file.
    if (_opt.update_channel_file && !_opt.channel_file.empty() && ts::FileExists(_opt.channel_file) && !_channels.load(_opt.channel_file, _opt)) {
        return;
    }

    // Main processing depends on scanning method.
    if (_opt.uhf_scan || _opt.vhf_scan) {
        hfBandScan();
    }
    else if (_opt.nit_scan) {
        nitScan();
    }
    else {
        _opt.fatal(u"inconsistent options, internal error");
    }

    // Report global list of services if required
    if (_opt.global_services) {
        _services.sort(ts::Service::Sort1);
        std::cout << std::endl;
        ts::Service::Display(std::cout, u"", _services);
    }

    // Save channel file. Create intermediate directories when it is the default file.
    if (!_opt.channel_file.empty()) {
        _opt.verbose(u"saving %s", {_opt.channel_file});
        _channels.save(_opt.channel_file, _opt.default_channel_file, _opt);
    }
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ScanOptions opt(argc, argv);
    ScanContext ctx(opt);
    ctx.main();
    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
