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
        TS_NOBUILD_NOCOPY(DVBInputPlugin);
    public:
        // Implementation of plugin API
        DVBInputPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;
        virtual bool abortInput() override;

        // Larger stack size than default
        virtual size_t stackUsage() const override {return 512 * 1024;} // 512 kB

    private:
        Tuner     _tuner;            // DVB tuner device
        TunerArgs _tuner_args;       // Command-line tuning arguments
        BitRate   _previous_bitrate; // Previous value from getBitrate()
    };
}

TS_REGISTER_INPUT_PLUGIN(u"dvb", ts::DVBInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DVBInputPlugin::DVBInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"DVB receiver device input", u"[options]"),
    _tuner(duck),
    _tuner_args(false),
    _previous_bitrate(0)
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

bool ts::DVBInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
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
    tsp->verbose(u"using %s (%s)", {_tuner.deviceName(), _tuner.deliverySystems().toString()});

    // Tune to the specified frequency.
    if (!_tuner_args.hasModulationArgs()) {
        tsp->verbose(u"no modulation parameter specified, using current transponder in tuner");
    }
    else if (_tuner.tune(_tuner_args)) {
        tsp->verbose(u"tuned to transponder %s", {_tuner_args.toPluginOptions()});
    }
    else {
        stop();
        return false;
    }

    // Compute theoretical TS bitrate from tuning parameters.
    const BitRate bitrate = _tuner_args.theoreticalBitrate();
    if (bitrate > 0) {
        tsp->verbose(u"expected bitrate from tuning parameters: %'d b/s", {bitrate});
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
        ObjectRepository::Instance()->store(u"tsp.dvb.params", ObjectPtr(new ModulationArgs(_tuner_args)));

        // Display new tuning info
        tsp->verbose(u"actual tuning options: %s", {_tuner_args.toPluginOptions()});
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
