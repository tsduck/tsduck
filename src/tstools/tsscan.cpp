//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsCOM.h"
#include "tsTuner.h"
#include "tsTunerUtils.h"
#include "tsTunerArgs.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersATSC.h"
#include "tsTSScanner.h"
#include "tsChannelFile.h"
#include "tsNIT.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"
#include "tsTime.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

#define DEFAULT_PSI_TIMEOUT   10000 // ms
#define DEFAULT_MIN_STRENGTH  10
#define DEFAULT_MIN_QUALITY   10
#define DEFAULT_FIRST_OFFSET  (-2)
#define DEFAULT_LAST_OFFSET   (+2)
#define OFFSET_EXTEND         3


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

class ScanOptions: public ts::Args
{
public:
    ScanOptions(int argc, char *argv[]);
    virtual ~ScanOptions();

    ts::TunerArgs   tuner_args;
    bool            uhf_scan;
    bool            nit_scan;
    bool            no_offset;
    bool            use_best_quality;
    bool            use_best_strength;
    int             first_uhf_channel;
    int             last_uhf_channel;
    int             first_uhf_offset;
    int             last_uhf_offset;
    int             min_strength;
    int             min_quality;
    bool            show_modulation;
    bool            list_services;
    bool            global_services;
    ts::MilliSecond psi_timeout;
    ts::UString     channel_file;
    bool            update_channel_file;
    bool            default_channel_file;
};

// Destructor.
ScanOptions::~ScanOptions() {}

// Constructor.
ScanOptions::ScanOptions(int argc, char *argv[]) :
    Args(u"Scan a DVB network", u"[options]"),
    tuner_args(false, true),
    uhf_scan(false),
    nit_scan(false),
    no_offset(false),
    use_best_quality(false),
    use_best_strength(false),
    first_uhf_channel(0),
    last_uhf_channel(0),
    first_uhf_offset(0),
    last_uhf_offset(0),
    min_strength(0),
    min_quality(0),
    show_modulation(false),
    list_services(false),
    global_services(false),
    psi_timeout(0),
    channel_file(),
    update_channel_file(false),
    default_channel_file(false)
{
    // Warning, the following short options are already defined in TunerArgs:
    // 'a', 'c', 'd', 'f', 'm', 's', 'z'
    tuner_args.defineOptions(*this);

    option(u"best-quality");
    help(u"best-quality",
         u"With UHF-band scanning, for each channel, use the offset with the "
         u"best signal quality. By default, use the average of lowest and highest "
         u"offsets with required minimum quality and strength. Note that some tuners "
         u"cannot report a correct signal quality, making this option useless.");

    option(u"best-strength");
    help(u"best-strength",
         u"With UHF-band scanning, for each channel, use the offset with the "
         u"best signal strength. By default, use the average of lowest and highest "
         u"offsets with required minimum quality and strength. Note that some tuners "
         u"cannot report a correct signal strength, making this option useless.");

    option(u"first-uhf-channel", 0, INTEGER, 0, 1, ts::UHF::FIRST_CHANNEL, ts::UHF::LAST_CHANNEL);
    help(u"first-uhf-channel",
         u"For UHF-band scanning, specify the first channel to scan (default: " +
         ts::UString::Decimal(ts::UHF::FIRST_CHANNEL) + u").");

    option(u"first-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"first-offset",
         u"For UHF-band scanning, specify the first offset to scan (default: " +
         ts::UString::Decimal(DEFAULT_FIRST_OFFSET, 0, true, u",", true) + u") "
         u"on each channel.");

    option(u"global-service-list", 'g');
    help(u"global-service-list",
         u"Same as --service-list but display a global list of services at the end "
         u"of scanning instead of per transport stream.");

    option(u"last-uhf-channel", 0, INTEGER, 0, 1, ts::UHF::FIRST_CHANNEL, ts::UHF::LAST_CHANNEL);
    help(u"last-uhf-channel",
         u"For UHF-band scanning, specify the last channel to scan (default: " +
         ts::UString::Decimal(ts::UHF::LAST_CHANNEL) + u").");

    option(u"last-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"last-offset",
         u"For UHF-band scanning, specify the last offset to scan on each channel (default: " +
         ts::UString::Decimal(DEFAULT_LAST_OFFSET, 0, true, u",", true) + u"). "
         u"Note that tsscan may scan higher offsets. As long as some signal is found at a "
         u"specified offset, tsscan continues to check up to 3 higher offsets above the \"last\" one. "
         u"This means that if a signal is found at offset +2, offset +3 will be checked anyway, etc. up to offset +5.");

    option(u"min-quality", 0, INTEGER, 0, 1, 0, 100);
    help(u"min-quality",
         u"Minimum signal quality percentage. Frequencies with lower signal "
         u"quality are ignored (default: " + ts::UString::Decimal(DEFAULT_MIN_QUALITY) + u"%).");

    option(u"min-strength", 0, INTEGER, 0, 1, 0, 100);
    help(u"min-strength",
         u"Minimum signal strength percentage. Frequencies with lower signal "
         u"strength are ignored (default: " + ts::UString::Decimal(DEFAULT_MIN_STRENGTH) + u"%).");

    option(u"no-offset", 'n');
    help(u"no-offset",
         u"For UHF-band scanning, scan only the central frequency of each channel. "
         u"Do not scan frequencies with offsets.");

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

    option(u"uhf-band", 'u');
    help(u"uhf-band",
         u"Perform a complete DVB-T UHF-band scanning. Do not use the NIT.\n\n"
         u"If tuning parameters are present (frequency or channel reference), the NIT is "
         u"read on the specified frequency and a full scan of the corresponding network is "
         u"performed. By default, without specific frequency, an UHF-band scanning is performed.");

    option(u"save-channels", 0, STRING);
    help(u"save-channels", u"filename",
         u"Save the description of all channels in the specified XML file. "
         u"If the file name is \"-\", use the default channel file name: " +
         ts::ChannelFile::DefaultFileName() + "\n"
         u"See also option --update-channels.");

    option(u"update-channels", 0, STRING);
    help(u"update-channels", u"filename",
         u"Update the description of all channels in the specified XML file. "
         u"The content of each scanned transport stream is replaced in the file. "
         u"If the file does not exist, it is created. "
         u"If the file name is \"-\", use the default channel file name: " +
         ts::ChannelFile::DefaultFileName() + "\n"
         u"See also option --save-channels.");

    analyze(argc, argv);
    tuner_args.load(*this);

    uhf_scan          = present(u"uhf-band");
    nit_scan          = tuner_args.hasTuningInfo();
    use_best_quality  = present(u"best-quality");
    use_best_strength = present(u"best-strength");
    first_uhf_channel = intValue(u"first-uhf-channel", ts::UHF::FIRST_CHANNEL);
    last_uhf_channel  = intValue(u"last-uhf-channel", ts::UHF::LAST_CHANNEL);
    show_modulation   = present(u"show-modulation");
    no_offset         = present(u"no-offset");
    first_uhf_offset  = no_offset ? 0 : intValue(u"first-offset", DEFAULT_FIRST_OFFSET);
    last_uhf_offset   = no_offset ? 0 : intValue(u"last-offset", DEFAULT_LAST_OFFSET);
    min_quality       = intValue(u"min-quality", DEFAULT_MIN_QUALITY);
    min_strength      = intValue(u"min-strength", DEFAULT_MIN_STRENGTH);
    list_services     = present(u"service-list");
    global_services   = present(u"global-service-list");
    psi_timeout       = intValue<ts::MilliSecond>(u"psi-timeout", DEFAULT_PSI_TIMEOUT);
    
    const bool save_channel_file = present(u"save-channels");
    update_channel_file = present(u"update-channels");
    channel_file = update_channel_file ? value(u"update-channels") : value(u"save-channels");
    default_channel_file = (save_channel_file || update_channel_file) && (channel_file.empty() || channel_file == u"-");

    if (nit_scan && uhf_scan) {
        error(u"do not specify tuning parameters with --uhf-band");
    }
    if (!uhf_scan && !nit_scan) {
        // Default is UHF scan.
        uhf_scan = true;
    }
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
// UHF-band offset scanner: Scan offsets around a specific UHF channel and
// determine offset with the best signal.
//----------------------------------------------------------------------------

class OffsetScanner
{
public:
    // Constructor: Perform scanning. Keep signal tuned on best offset.
    OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, int channel);

    // Check if signal found and which offset is the best one.
    bool signalFound() const {return _signal_found;}
    int channel() const {return _channel;}
    int bestOffset() const {return _best_offset;}
    ts::TunerParametersPtr tunerParameters() const {return _best_params;}

private:
    ScanOptions& _opt;
    ts::Tuner& _tuner;
    const int _channel;
    bool _signal_found;
    int _best_offset;
    int _lowest_offset;
    int _highest_offset;
    int _best_quality;
    int _best_quality_offset;
    int _best_strength;
    int _best_strength_offset;
    ts::TunerParametersPtr _best_params;

    // Build tuning parameters for a channel.
    ts::TunerParametersPtr tuningParameters(int offset);

    // Tune to specified offset. Return false on error.
    bool tune(int offset, ts::TunerParametersPtr& params);

    // Test the signal at one specific offset. Return true if signal is found.
    bool tryOffset(int offset);
};


//----------------------------------------------------------------------------
// UHF-band offset scanner constructor.
// Perform scanning. Keep signal tuned on best offset
//----------------------------------------------------------------------------

OffsetScanner::OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, int channel) :
    _opt(opt),
    _tuner(tuner),
    _channel(channel),
    _signal_found(false),
    _best_offset(0),
    _lowest_offset(0),
    _highest_offset(0),
    _best_quality(0),
    _best_quality_offset(0),
    _best_strength(0),
    _best_strength_offset(0),
    _best_params()
{
    _opt.verbose(u"scanning channel %'d, %'d Hz", {_channel, ts::UHF::Frequency(_channel)});

    if (_opt.no_offset) {
        // Only try the central frequency
        tryOffset(0);
    }
    else {
        // Scan lower offsets in descending order, starting at central frequency
        if (_opt.first_uhf_offset <= 0) {
            bool last_ok = false;
            int offset = _opt.last_uhf_offset > 0 ? 0 : _opt.last_uhf_offset;
            while (offset >= _opt.first_uhf_offset - (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset);
                --offset;
            }
        }

        // Scan higher offsets in ascending order, starting after central frequency
        if (_opt.last_uhf_offset > 0) {
            bool last_ok = false;
            int offset = _opt.first_uhf_offset <= 0 ? 1 : _opt.first_uhf_offset;
            while (offset <= _opt.last_uhf_offset + (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset);
                ++offset;
            }
        }
    }

    // If signal was found, select best offset
    if (_signal_found) {
        if (_opt.use_best_quality && _best_quality > 0) {
            // Signal quality indicator is valid, use offset with best signal quality
            _best_offset = _best_quality_offset;
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
        _signal_found = tune(_best_offset, _best_params) && _tuner.getCurrentTuning(*_best_params, false, _opt);
    }
}


//----------------------------------------------------------------------------
// Build tuning parameters for a channel.
//----------------------------------------------------------------------------

ts::TunerParametersPtr OffsetScanner::tuningParameters(int offset)
{
    // Force frequency in tuning parameters.
    // Other tuning parameters from command line (or default values).
    _opt.tuner_args.frequency = ts::UHF::Frequency(_channel, offset);
    ts::TunerParametersPtr params(new ts::TunerParametersDVBT);
    return params->fromTunerArgs(_opt.tuner_args, _opt) ? params : ts::TunerParametersPtr();
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Tune to specified offset. Return false on error.
//----------------------------------------------------------------------------

bool OffsetScanner::tune(int offset, ts::TunerParametersPtr& params)
{
    params = tuningParameters(offset);
    return !params.isNull() && _tuner.tune(*params, _opt);
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Test the signal at one specific offset.
//----------------------------------------------------------------------------

bool OffsetScanner::tryOffset(int offset)
{
    _opt.debug(u"trying offset %d", {offset});

    // Tune to transponder and start signal acquisition.
    // Signal locking timeout is applied in start().
    ts::TunerParametersPtr params;
    if (!tune(offset, params) || !_tuner.start(_opt)) {
        return false;
    }

    // Previously, we double-checked that the signal was locked.
    // However, looking for signal locked fails on Windows, even if the
    // signal was actually locked. So, we skip this test and we rely on
    // the fact that the signal timeout is always non-zero with tsscan,
    // so since _tuner.start() has succeeded we can be sure that at least
    // one packet was successfully read and there is some signal.
    bool ok =
#if defined(TS_LINUX)
        _tuner.signalLocked(_opt);
#else
        true;
#endif

    if (ok) {
        // Get signal quality & strength
        const int strength = _tuner.signalStrength(_opt);
        const int quality = _tuner.signalQuality(_opt);
        _opt.verbose(ts::UHF::Description(_channel, offset, strength, quality));

        if (strength >= 0 && strength <= _opt.min_strength) {
            // Strength is supported but too low
            ok = false;
        }
        else if (strength > _best_strength) {
            // Best offset so far for signal strength
            _best_strength = strength;
            _best_strength_offset = offset;
            _tuner.getCurrentTuning(*params, false, _opt);
        }

        if (quality >= 0 && quality <= _opt.min_quality) {
            // Quality is supported but too low
            ok = false;
        }
        else if (quality > _best_quality) {
            // Best offset so far for signal quality
            _best_quality = quality;
            _best_quality_offset = offset;
            _tuner.getCurrentTuning(*params, false, _opt);
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
    _tuner.stop(_opt);

    return ok;
}


//----------------------------------------------------------------------------
// Scanning context.
//----------------------------------------------------------------------------

class ScanContext
{
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
    void scanTS(std::ostream& strm, const ts::UString& margin, ts::TunerParametersPtr tparams);

    // UHF-band scanning
    void uhfScan();

    // NIT-based scanning
    void nitScan();

    // Inaccessible methods.
    ScanContext() = delete;
    ScanContext(const ScanContext&) = delete;
    ScanContext& operator=(const ScanContext&) = delete;
};

// Contructor.
ScanContext::ScanContext(ScanOptions& opt) :
    _opt(opt),
    _tuner(),
    _services(),
    _channels()
{
}


//----------------------------------------------------------------------------
// Analyze a TS and generate relevant info.
//----------------------------------------------------------------------------

void ScanContext::scanTS(std::ostream& strm, const ts::UString& margin, ts::TunerParametersPtr tparams)
{
    const bool get_services = _opt.list_services || _opt.global_services;

    // Collect info from the TS.
    // Use "PAT only" when we do not need the services or channels file.
    ts::TSScanner info(_tuner, _opt.psi_timeout, !get_services && _opt.channel_file.empty(), _opt);

    if (tparams.isNull()) {
        info.getTunerParameters(tparams);
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
        ts::ChannelFile::NetworkPtr net_info(_channels.networkGetOrCreate(net_id, _tuner.tunerType()));
        ts_info = net_info->tsGetOrCreate(ts_id);
        ts_info->clear(); // reset all services in TS.
        ts_info->onid = sdt.isNull() ? 0 : sdt->onetw_id;
        ts_info->tune = tparams;
    }

    // Display modulation parameters
    if (_opt.show_modulation && !tparams.isNull()) {
        tparams->displayParameters(strm, margin);
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
// UHF-band scanning
//----------------------------------------------------------------------------

void ScanContext::uhfScan()
{
    // UHF means DVB-T
    if (_tuner.tunerType() != ts::DVB_T) {
        _opt.error(u"UHF scanning needs DVB-T, tuner %s is %s", {_tuner.deviceName(), ts::TunerTypeEnum.name(_tuner.tunerType())});
        return;
    }

    // Loop on all selected UHF channels
    for (int chan = _opt.first_uhf_channel; chan <= _opt.last_uhf_channel; ++chan) {

        // Scan all offsets surrounding the channel
        OffsetScanner offscan(_opt, _tuner, chan);
        if (offscan.signalFound()) {

            // Report channel characteristics
            std::cout << "* UHF "
                        << ts::UHF::Description(chan, offscan.bestOffset(), _tuner.signalStrength(_opt), _tuner.signalQuality(_opt))
                        << std::endl;

            // Analyze PSI/SI if required
            scanTS(std::cout, u"  ", offscan.tunerParameters());
        }
    }
}


//----------------------------------------------------------------------------
// NIT-based scanning
//----------------------------------------------------------------------------

void ScanContext::nitScan()
{
    // Tune to the reference transponder.
    ts::TunerParametersPtr params;
    if (!_opt.tuner_args.tune(_tuner, params, _opt)) {
        return;
    }

    // Collect info on reference transponder.
    ts::TSScanner info(_tuner, _opt.psi_timeout, false, _opt);

    // Get the collected NIT
    ts::SafePtr<ts::NIT> nit;
    info.getNIT(nit);
    if (nit.isNull()) {
        _opt.error(u"cannot scan network, no NIT found on specified transponder");
        return;
    }

    // Process each TS descriptor list in the NIT.
    for (ts::NIT::TransportMap::const_iterator it = nit->transports.begin(); it != nit->transports.end(); ++it) {
        const ts::DescriptorList& dlist(it->second.descs);

        // Loop on all descriptors for the current TS.
        for (size_t i = 0; i < dlist.count(); ++i) {
            // Try to get delivery system information from current descriptor
            ts::TunerParametersPtr tp(ts::DecodeDeliveryDescriptor(*dlist[i]));
            if (!tp.isNull()) {
                // Got a delivery descriptor, this is the description of one transponder.
                // Tune to this transponder.
                _opt.debug(u"* tuning to " + tp->toPluginOptions(true));
                if (_tuner.tune(*tp, _opt)) {

                    // Report channel characteristics
                    std::cout << "* Frequency: " << tp->shortDescription(_tuner.signalStrength(_opt), _tuner.signalQuality(_opt)) << std::endl;

                    // Analyze PSI/SI if required
                    scanTS(std::cout, u"  ", tp);
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
    if (!_opt.tuner_args.configureTuner(_tuner, _opt)) {
        return;
    }

    // Pre-load the existing channel file.
    if (_opt.update_channel_file && !_opt.channel_file.empty() && ts::FileExists(_opt.channel_file) && !_channels.load(_opt.channel_file, _opt)) {
        return;
    }

    // Main processing depends on scanning method.
    if (_opt.uhf_scan) {
        uhfScan();
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
    ts::COM com(opt);

    if (com.isInitialized()) {
        ScanContext ctx(opt);
        ctx.main();
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}

TS_MAIN(MainCode)
