//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Tuner device input (was DVB only initially, any tuner now)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsObjectRepository.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsSignalState.h"
#include "tsModulationArgs.h"
#include "tsjsonOutputArgs.h"
#include "tsjsonObject.h"
#include "tsxmlAttribute.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DVBInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DVBInputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;
        virtual bool abortInput() override;

        // Larger stack size than default
        virtual size_t stackUsage() const override {return 512 * 1024;} // 512 kB

    private:
        Tuner            _tuner {duck};          // DVB tuner device.
        TunerArgs        _tuner_args {false};    // Command-line tuning arguments.
        json::OutputArgs _json_args {};          // JSON status reporting.
        cn::seconds      _json_interval {};      // Interval between JSON status reports.
        BitRate          _previous_bitrate = 0;  // Previous value from getBitrate().
        Time             _next_json_report {};   // UTC time of next JSON report.

        static constexpr cn::seconds DEFAULT_JSON_INTERVAL = cn::seconds(60);

        // Produce a JSON status report if necessary.
        void jsonReport();

        // Store the tuning parameters in a global repository (may be used by other plugins).
        void storeTunerArgs();
    };
}

TS_REGISTER_INPUT_PLUGIN(u"dvb", ts::DVBInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DVBInputPlugin::DVBInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"DVB receiver device input", u"[options]")
{
    // Define common tuning options
    duck.defineArgsForHFBand(*this);
    _tuner_args.defineArgs(*this, true);

    // Define options for periodic status reporting.
    _json_args.defineArgs(*this, true, u"Produce a status report in JSON format at regular intervals.", false);

    option<cn::seconds>(u"json-interval");
    help(u"json-interval",
         u"With --json-line, --json-tcp, --json-udp, specify the interval between two status reports. "
         u"The default is " + UString::Chrono(DEFAULT_JSON_INTERVAL) + u".");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::getOptions()
{
    // Get common tuning options from command line
    duck.loadArgs(*this);
    _tuner_args.loadArgs(duck, *this);
    _json_args.loadArgs(*this);
    getChronoValue(_json_interval, u"json-interval", DEFAULT_JSON_INTERVAL);
    return Args::valid();
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    if (timeout.count() > 0) {
        _tuner_args.receive_timeout = timeout;
    }
    return true;
}


//----------------------------------------------------------------------------
// Abort input method
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::abortInput()
{
    _tuner.abort(true);
    return true;
}


//----------------------------------------------------------------------------
// Store the tuning parameters in a global repository (may be used by other plugins).
//----------------------------------------------------------------------------

void ts::DVBInputPlugin::storeTunerArgs()
{
    ObjectRepository::Instance().store(u"tsp.dvb.params", std::make_shared<ModulationArgs>(_tuner_args));
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::start()
{
    // Check if tuner is already open.
    if (_tuner.isOpen()) {
        return false;
    }

    // Reinitialize other states
    _previous_bitrate = 0;

    // Open DVB tuner
    if (!_tuner_args.configureTuner(_tuner)) {
        return false;
    }
    verbose(u"using %s (%s)", _tuner.deviceName(), _tuner.deliverySystems().toString());

    // Tune to the specified frequency.
    if (!_tuner_args.hasModulationArgs()) {
        verbose(u"no modulation parameter specified, using current transponder in tuner");
    }
    else if (_tuner.tune(_tuner_args)) {
        verbose(u"tuned to transponder %s", _tuner_args.toPluginOptions());
    }
    else {
        stop();
        return false;
    }
    storeTunerArgs();

    // Compute theoretical TS bitrate from tuning parameters.
    const BitRate bitrate = _tuner_args.theoreticalBitrate();
    if (bitrate > 0) {
        verbose(u"expected bitrate from tuning parameters: %'d b/s", bitrate);
    }

    // Start receiving packets
    debug(u"starting tuner reception");
    if (!_tuner.start()) {
        stop();
        return false;
    }
    debug(u"tuner reception started");

    // Display signal state in verbose mode.
    SignalState state;
    if (_tuner.getSignalState(state)) {
        verbose(state.toString());
    }

    // Initialize periodic JSON reporting. Produce an initial report if necessary.
    _next_json_report = Time::CurrentUTC();
    jsonReport();

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::stop()
{
    _tuner.stop();
    _tuner.close();
    return true;
}


//----------------------------------------------------------------------------
// Get input bitrate method
//----------------------------------------------------------------------------

ts::BitRate ts::DVBInputPlugin::getBitrate()
{
    // The bitrate is entirely based on the transponder characteristics
    // such as symbol rate, number of bits per symbol (modulation),
    // number of used bits vs. transported bits (FEC), etc.

    // Get current tuning information
    if (!_tuner.getCurrentTuning(_tuner_args, false)) {
        return 0; // error
    }

    // Let the TunerParameters subclass compute the bitrate
    BitRate bitrate = _tuner_args.theoreticalBitrate();

    // When bitrate changes, the modulation parameters have changed
    if (bitrate != _previous_bitrate) {
        storeTunerArgs();
        verbose(u"actual tuning options: %s", _tuner_args.toPluginOptions());
    }

    return _previous_bitrate = bitrate;
}


//----------------------------------------------------------------------------
// This is a hardware-based real-time plugin.
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::isRealTime()
{
    return true;
}

ts::BitRateConfidence ts::DVBInputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the demodulator hardware.
    return BitRateConfidence::HARDWARE;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::DVBInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    const size_t count = _tuner.receive(buffer, max_packets, tsp);
    jsonReport();
    return count;
}


//----------------------------------------------------------------------------
// Produce a JSON status report if necessary.
//----------------------------------------------------------------------------

void ts::DVBInputPlugin::jsonReport()
{
    if (_json_args.useJSON() && Time::CurrentUTC() >= _next_json_report) {

        // Schedule next report.
        _next_json_report += _json_interval;

        // Build current report.
        json::Object obj;
        obj.add(u"#name", u"dvbstatus");
        obj.add(u"time", xml::Attribute::DateTimeToString(Time::CurrentLocalTime()));
        obj.add(u"packet-index", int64_t(tsp->pluginPackets()));
        if (_previous_bitrate > 0) {
            obj.add(u"bitrate", _previous_bitrate.toString());
        }
        _tuner_args.toJSON(obj);
        SignalState state;
        if (_tuner.getSignalState(state)) {
            state.toJSON(obj);
        }

        // Send the report to whatever was specified in the command line options.
        _json_args.report(obj, *this);
    }
}
