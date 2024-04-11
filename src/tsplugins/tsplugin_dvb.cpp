//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;
        virtual bool abortInput() override;

        // Larger stack size than default
        virtual size_t stackUsage() const override {return 512 * 1024;} // 512 kB

    private:
        Tuner     _tuner {duck};          // DVB tuner device
        TunerArgs _tuner_args {false};    // Command-line tuning arguments
        BitRate   _previous_bitrate = 0;  // Previous value from getBitrate()
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
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::DVBInputPlugin::getOptions()
{
    // Get common tuning options from command line
    duck.loadArgs(*this);
    _tuner_args.loadArgs(duck, *this);
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
    tsp->verbose(u"using %s (%s)", _tuner.deviceName(), _tuner.deliverySystems().toString());

    // Tune to the specified frequency.
    if (!_tuner_args.hasModulationArgs()) {
        tsp->verbose(u"no modulation parameter specified, using current transponder in tuner");
    }
    else if (_tuner.tune(_tuner_args)) {
        tsp->verbose(u"tuned to transponder %s", _tuner_args.toPluginOptions());
    }
    else {
        stop();
        return false;
    }

    // Compute theoretical TS bitrate from tuning parameters.
    const BitRate bitrate = _tuner_args.theoreticalBitrate();
    if (bitrate > 0) {
        tsp->verbose(u"expected bitrate from tuning parameters: %'d b/s", bitrate);
    }

    // Start receiving packets
    tsp->debug(u"starting tuner reception");
    if (!_tuner.start()) {
        stop();
        return false;
    }
    tsp->debug(u"tuner reception started");

    // Display signal state in verbose mode.
    SignalState state;
    if (_tuner.getSignalState(state)) {
        tsp->verbose(state.toString());
    }

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
        // Store the new parameters in a global repository (may be used by other plugins)
        ObjectRepository::Instance().store(u"tsp.dvb.params", std::make_shared<ModulationArgs>(_tuner_args));

        // Display new tuning info
        tsp->verbose(u"actual tuning options: %s", _tuner_args.toPluginOptions());
    }

    return _previous_bitrate = bitrate;
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
    return _tuner.receive(buffer, max_packets, tsp);
}
