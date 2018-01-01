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
//
//  DVB network scanning utility
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsCOM.h"
#include "tsTuner.h"
#include "tsTunerUtils.h"
#include "tsTunerArgs.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersATSC.h"
#include "tsTSScanner.h"
#include "tsNIT.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"
#include "tsTime.h"
#include "tsNullReport.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;

#define DEFAULT_PSI_TIMEOUT   10000 // ms
#define DEFAULT_MIN_STRENGTH  10
#define DEFAULT_MIN_QUALITY   10
#define DEFAULT_FIRST_OFFSET  (-2)
#define DEFAULT_LAST_OFFSET   (+2)
#define OFFSET_EXTEND         3


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::TunerArgs   tuner;
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
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"DVB network scanning utility.", u"[options]"),
    tuner(false, true),
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
    psi_timeout(0)
{
    // Warning, the following short options are already defined in TunerArgs:
    // 'a', 'c', 'd', 'f', 'm', 's', 'z'
    option(u"best-quality",         0);
    option(u"best-strength",        0);
    option(u"first-uhf-channel",    0,  INTEGER, 0, 1, ts::UHF::FIRST_CHANNEL, ts::UHF::LAST_CHANNEL);
    option(u"first-offset",         0,  INTEGER, 0, 1, -40, +40);
    option(u"global-service-list", 'g');
    option(u"last-uhf-channel",     0, INTEGER, 0, 1, ts::UHF::FIRST_CHANNEL, ts::UHF::LAST_CHANNEL);
    option(u"last-offset",          0,  INTEGER, 0, 1, -40, +40);
    option(u"min-quality",          0,  INTEGER, 0, 1, 0, 100);
    option(u"min-strength",         0,  INTEGER, 0, 1, 0, 100);
    option(u"no-offset",           'n');
    option(u"psi-timeout",          0,  UNSIGNED);
    option(u"service-list",        'l');
    option(u"show-modulation",      0);
    option(u"uhf-band",            'u');
    tuner.defineOptions(*this);

    setHelp(u"If tuning parameters are present (frequency or channel reference), the NIT is\n"
            u"read on the specified frequency and a full scan of the corresponding network is\n"
            u"performed.\n"
            u"\n"
            u"By default, without specific frequency, an UHF-band scanning is performed.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --best-quality\n"
            u"      With UHF-band scanning, for each channel, use the offset with the\n"
            u"      best signal quality. By default, use the average of lowest and highest\n"
            u"      offsets with required minimum quality and strength.\n"
            u"\n"
            u"  --best-strength\n"
            u"      With UHF-band scanning, for each channel, use the offset with the\n"
            u"      best signal strength. By default, use the average of lowest and highest\n"
            u"      offsets with required minimum quality and strength.\n"
            u"\n"
            u"  --first-uhf-channel value\n"
            u"      For UHF-band scanning, specify the first channel to scan (default: " +
            ts::UString::Decimal(ts::UHF::FIRST_CHANNEL) + u").\n"
            u"\n"
            u"  --first-offset value\n"
            u"      For UHF-band scanning, specify the first offset to scan (default: " +
            ts::UString::Decimal(DEFAULT_FIRST_OFFSET, 0, true, u",", true) + u")\n"
            u"      on each channel.\n"
            u"\n"
            u"  -g\n"
            u"  --global-service-list\n"
            u"      Same as --service-list but display a global list of services at the end\n"
            u"      of scanning instead of per transport stream.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --last-uhf-channel value\n"
            u"      For UHF-band scanning, specify the last channel to scan (default: " +
            ts::UString::Decimal(ts::UHF::LAST_CHANNEL) + u").\n"
            u"\n"
            u"  --last-offset value\n"
            u"      For UHF-band scanning, specify the last offset to scan (default: " +
            ts::UString::Decimal(DEFAULT_LAST_OFFSET, 0, true, u",", true) + u")\n"
            u"      on each channel.\n"
            u"\n"
            u"  --min-quality value\n"
            u"      Minimum signal quality percentage. Frequencies with lower signal\n"
            u"      quality are ignored (default: " + ts::UString::Decimal(DEFAULT_MIN_QUALITY) + u"%).\n"
            u"\n"
            u"  --min-strength value\n"
            u"      Minimum signal strength percentage. Frequencies with lower signal\n"
            u"      strength are ignored (default: " + ts::UString::Decimal(DEFAULT_MIN_STRENGTH) + u"%).\n"
            u"\n"
            u"  -n\n"
            u"  --no-offset\n"
            u"      For UHF-band scanning, scan only the central frequency of each channel.\n"
            u"      Do not scan frequencies with offsets.\n"
            u"\n"
            u"  --psi-timeout milliseconds\n"
            u"      Specifies the timeout, in milli-seconds, for PSI/SI table collection.\n"
            u"      Useful only with --service-list. The default is " +
            ts::UString::Decimal(DEFAULT_PSI_TIMEOUT) + u" milli-seconds.\n"
            u"\n"
            u"  -l\n"
            u"  --service-list\n"
            u"      Read SDT of each channel and display the list of services.\n"
            u"\n"
            u"  --show-modulation\n"
            u"      Display modulation parameters when possible.\n"
            u"\n"
            u"  -u\n"
            u"  --uhf-band\n"
            u"      Perform a complete DVB-T UHF-band scanning. Do not use the NIT.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
    tuner.addHelp(*this);

    analyze(argc, argv);
    tuner.load(*this);

    uhf_scan          = present(u"uhf-band");
    nit_scan          = tuner.hasTuningInfo();
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

    if (nit_scan && uhf_scan) {
        error(u"do not specify tuning parameters with --uhf-band");
    }
    if (!uhf_scan && !nit_scan) {
        // Default is UHF scan.
        uhf_scan = true;
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Analyze and display relevant TS info
//----------------------------------------------------------------------------

namespace {
    void DisplayTS(std::ostream& strm,
                   const ts::UString& margin,
                   Options& opt,
                   ts::Tuner& tuner,
                   ts::TunerParametersPtr tparams,
                   ts::ServiceList& global_services)
    {
        const bool get_services = opt.list_services || opt.global_services;

        // Collect info
        ts::TSScanner info(tuner, opt.psi_timeout, !get_services, opt);

        // Display TS Id
        ts::SafePtr<ts::PAT> pat;
        info.getPAT(pat);
        if (!pat.isNull()) {
            strm << margin
                 << ts::UString::Format(u"Transport stream id: %d, 0x%X", {pat->ts_id, pat->ts_id})
                 << std::endl;
        }

        // Display modulation parameters
        if (opt.show_modulation) {
            if (tparams.isNull()) {
                info.getTunerParameters(tparams);
            }
            if (!tparams.isNull()) {
                tparams->displayParameters(strm, margin);
            }
        }

        // Display services
        if (get_services) {
            ts::ServiceList services;
            if (info.getServices(services)) {
                if (opt.list_services) {
                    // Display services for this TS
                    services.sort(ts::Service::Sort1);
                    strm << std::endl;
                    ts::Service::Display(strm, margin, services);
                    strm << std::endl;
                }
                if (opt.global_services) {
                    // Add collected services in global service list
                    global_services.insert(global_services.end(), services.begin(), services.end());
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
//  UHF-band offset scanner: Scan offsets around a specific UHF channel and
//  determine offset with the best signal.
//----------------------------------------------------------------------------

class OffsetScanner
{
public:
    // Constructor
    // Perform scanning. Keep signal tuned on best offset
    OffsetScanner(Options& opt, ts::Tuner& tuner, int channel);

    // Check if signal found and which offset is the best one.
    bool signalFound() const {return _signal_found;}
    int channel() const {return _channel;}
    int bestOffset() const {return _best_offset;}

private:
    Options&   _opt;
    ts::Tuner& _tuner;
    const int  _channel;
    bool       _signal_found;
    int        _best_offset;
    int        _lowest_offset;
    int        _highest_offset;
    int        _best_quality;
    int        _best_quality_offset;
    int        _best_strength;
    int        _best_strength_offset;

    // Tune to specified offset. Return false on error.
    bool tune(int offset);

    // Test the signal at one specific offset. Return true if signal is found.
    bool tryOffset(int offset);
};


// Constructor. Perform scanning. Keep signal tuned on best offset
OffsetScanner::OffsetScanner(Options& opt, ts::Tuner& tuner, int channel) :
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
    _best_strength_offset(0)
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
        _signal_found = tune(_best_offset);
    }
}


// Tune to specified offset. Return false on error.
bool OffsetScanner::tune(int offset)
{
    // Default tuning parameters
    ts::TunerParametersDVBT tparams;
    tparams.frequency = ts::UHF::Frequency(_channel, offset);
    tparams.inversion = ts::SPINV_AUTO;
#if defined(TS_WINDOWS)
    tparams.bandwidth = ts::BW_8_MHZ; // BW_AUTO not supported
#else
    tparams.bandwidth = ts::BW_AUTO;
#endif
    tparams.fec_hp = ts::FEC_AUTO;
    tparams.fec_lp = ts::FEC_AUTO;
    tparams.modulation = ts::QAM_AUTO;
    tparams.transmission_mode = ts::TM_AUTO;
    tparams.guard_interval = ts::GUARD_AUTO;
    tparams.hierarchy = ts::HIERARCHY_AUTO;
    return _tuner.tune(tparams, _opt);
}


// Test the signal at one specific offset
bool OffsetScanner::tryOffset(int offset)
{
    _opt.debug(u"trying offset %d", {offset});

    // Tune to transponder and start signal acquisition.
    // Signal locking timeout is applied in start().
    if (!tune(offset) || !_tuner.start(_opt)) {
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
        }

        if (quality >= 0 && quality <= _opt.min_quality) {
            // Quality is supported but too low
            ok = false;
        }
        else if (quality > _best_quality) {
            // Best offset so far for signal quality
            _best_quality = quality;
            _best_quality_offset = offset;
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
//  UHF-band scanning
//----------------------------------------------------------------------------

namespace {
    void UHFScan(Options& opt, ts::Tuner& tuner, ts::ServiceList& all_services)
    {
        // UHF means DVB-T
        if (tuner.tunerType() != ts::DVB_T) {
            opt.error(u"UHF scanning needs DVB-T, tuner %s is %s", {tuner.deviceName(), ts::TunerTypeEnum.name(tuner.tunerType())});
            return;
        }

        // Loop on all selected UHF channels
        for (int chan = opt.first_uhf_channel; chan <= opt.last_uhf_channel; ++chan) {

            // Scan all offsets surrounding the channel
            OffsetScanner offscan(opt, tuner, chan);
            if (offscan.signalFound()) {

                // Report channel characteristics
                std::cout << "* UHF "
                          << ts::UHF::Description(chan, offscan.bestOffset(), tuner.signalStrength(opt), tuner.signalQuality(opt))
                          << std::endl;

                // Analyze PSI/SI if required
                DisplayTS(std::cout, u"  ", opt, tuner, ts::TunerParametersPtr(), all_services);
            }
        }
    }
}


//----------------------------------------------------------------------------
//  NIT-based scanning
//----------------------------------------------------------------------------

namespace {
    void NITScan(Options& opt, ts::Tuner& tuner, ts::ServiceList& all_services)
    {
        // Tune to the reference transponder.
        ts::TunerParametersPtr params;
        if (!opt.tuner.tune(tuner, params, opt)) {
            return;
        }

        // Collect info on reference transponder.
        ts::TSScanner info(tuner, opt.psi_timeout, false, opt);

        // Get the collected NIT
        ts::SafePtr<ts::NIT> nit;
        info.getNIT(nit);
        if (nit.isNull()) {
            opt.error(u"cannot scan network, no NIT found on specified transponder");
            return;
        }

        // Process each TS descriptor list in the NIT.
        for (ts::NIT::TransportMap::const_iterator it = nit->transports.begin(); it != nit->transports.end(); ++it) {
            const ts::DescriptorList& dlist(it->second);

            // Loop on all descriptors for the current TS.
            for (size_t i = 0; i < dlist.count(); ++i) {
                // Try to get delivery system information from current descriptor
                ts::TunerParametersPtr tp(ts::DecodeDeliveryDescriptor(*dlist[i]));
                if (!tp.isNull()) {
                    // Got a delivery descriptor, this is the description of one transponder.
                    // Tune to this transponder.
                    opt.debug(u"* tuning to " + tp->toPluginOptions(true));
                    if (tuner.tune(*tp, opt)) {

                        // Report channel characteristics
                        std::cout << "* Frequency: " << tp->shortDescription(tuner.signalStrength(opt), tuner.signalQuality(opt)) << std::endl;

                        // Analyze PSI/SI if required
                        DisplayTS(std::cout, u"  ", opt, tuner, tp, all_services);
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Main code. Isolated from main() to ensure that destructors are invoked
//  before COM uninitialize.
//----------------------------------------------------------------------------

namespace {
    void MainCode(Options& opt)
    {
        ts::ServiceList all_services;

        // Initialize tuner.
        ts::Tuner tuner;
        tuner.setSignalTimeoutSilent(true);
        if (!opt.tuner.configureTuner(tuner, opt)) {
            return;
        }

        if (opt.uhf_scan) {
            UHFScan(opt, tuner, all_services);
        }
        else if (opt.nit_scan) {
            NITScan(opt, tuner, all_services);
        }
        else {
            opt.fatal(u"inconsistent options, internal error");
        }

        // Report global list of services if required
        if (opt.global_services) {
            all_services.sort(ts::Service::Sort1);
            std::cout << std::endl;
            ts::Service::Display(std::cout, u"", all_services);
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    ts::COM com(opt);

    if (com.isInitialized()) {
        MainCode(opt);
    }

    opt.exitOnError();
    return EXIT_SUCCESS;
}
