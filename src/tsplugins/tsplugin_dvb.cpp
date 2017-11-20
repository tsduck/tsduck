//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  DVB device input (DVB-S, DVB-C, DVB-T)
//
//----------------------------------------------------------------------------

#include "tsPlatform.h"
#include "tsPlugin.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsTunerParameters.h"
#include "tsSysUtils.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsCOM.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DVBInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        DVBInput(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate();
        virtual size_t receive(TSPacket*, size_t);

        // Larger stack size than default
        virtual size_t stackUsage() const {return 512 * 1024;} // 512 kB

    private:
        COM                _com;              // COM initialization helper
        Tuner              _tuner;            // DVB tuner device
        TunerArgs          _tuner_args;       // Command-line tuning arguments
        TunerParametersPtr _tuner_params;     // Tuning parameters
        BitRate            _previous_bitrate; // Previous value from getBitrate()

        // Inaccessible operations
        DVBInput() = delete;
        DVBInput(const DVBInput&) = delete;
        DVBInput& operator=(const DVBInput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(ts::DVBInput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DVBInput::DVBInput(TSP* tsp_) :
    InputPlugin(tsp_, u"DVB receiver device input.", u"[options]"),
    _com(*tsp_),
    _tuner(),
    _tuner_args(false, true),
    _tuner_params(),
    _previous_bitrate(0)
{
    setHelp(u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    // Define common tuning options
    _tuner_args.defineOptions(*this);
    _tuner_args.addHelp(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DVBInput::start()
{
    // Check that COM was correctly initialized
    if (!_com.isInitialized()) {
        tsp->error ("COM initialization failure");
        return false;
    }

    // Check if tuner is already open.
    if (_tuner.isOpen()) {
        return false;
    }

    // Get common tuning options from command line
    _tuner_args.load(*this);
    if (!Args::valid()) {
        return false;
    }

    // Reinitialize other states
    _previous_bitrate = 0;

    // Open DVB tuner
    if (!_tuner_args.configureTuner(_tuner, *tsp)) {
        return false;
    }
    tsp->verbose("using " + _tuner.deviceName() + " (" + TunerTypeEnum.name(_tuner.tunerType()) + ")");

    // Tune to the specified frequency.
    if (!_tuner_args.tune(_tuner, _tuner_params, *tsp)) {
        stop();
        return false;
    }
    tsp->verbose("tuned to transponder " + _tuner_params->toPluginOptions());

    // Start receiving packets
    tsp->debug("starting tuner reception");
    if (!_tuner.start(*tsp)) {
        stop();
        return false;
    }
    tsp->debug("tuner reception started");

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DVBInput::stop()
{
    _tuner.stop(*tsp);
    _tuner.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Get input bitrate method
//----------------------------------------------------------------------------

ts::BitRate ts::DVBInput::getBitrate()
{
    // The bitrate is entirely based on the transponder characteristics
    // such as symbol rate, number of bits per symbol (modulation),
    // number of used bits vs. transported bits (FEC), etc.

    // Get current tuning information
    if (!_tuner.getCurrentTuning (*_tuner_params, false, *tsp)) {
        return 0; // error
    }

    // Let the TunerParameters subclass compute the bitrate
    BitRate bitrate = _tuner_params->theoreticalBitrate();

    // When bitrate changes, the modulation parameters have changed
    if (bitrate != _previous_bitrate && tsp->verbose()) {
        // Store the new parameters in a global repository (may be used by other plugins)
        TunerParameters* new_params (TunerParameters::Factory (_tuner_params->tunerType()));
        new_params->copy (*_tuner_params);
        Object::StoreInRepository ("tsp.dvb.params", ObjectPtr (new_params));
        // Display new tuning info
        tsp->verbose ("actual tuning options: " + new_params->toPluginOptions());
    }

    return _previous_bitrate = bitrate;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::DVBInput::receive (TSPacket* buffer, size_t max_packets)
{
    return _tuner.receive (buffer, max_packets, tsp, *tsp);
}
