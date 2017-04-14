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



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DVBInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        DVBInput (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate();
        virtual size_t receive (TSPacket*, size_t);

        // Larger stack size than default
        virtual size_t stackUsage () const {return 512 * 1024;} // 512 kB

    private:
        COM                _com;              // COM initialization helper
        std::string        _device_name;      // Name of tuner device (empty means first one)
        Tuner              _tuner;            // DVB tuner device
        TunerArgs          _tuner_args;       // Command-line tuning arguments
        TunerParametersPtr _tuner_params;     // Tuning parameters
        BitRate            _previous_bitrate; // Previous value from getBitrate()
    };
}

TSPLUGIN_DECLARE_VERSION;
TSPLUGIN_DECLARE_INPUT (ts::DVBInput);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DVBInput::DVBInput (TSP* tsp_) :
    InputPlugin (tsp_, "DVB receiver device input.", "[options]"),
    _com (*tsp_),
    _previous_bitrate (0)
{
    // Warning, the following short options are already defined in TunerArgs:
    // 'c', 'f', 'l', 'm', 'o', 's', 't', 'u', 'v', 'z'
    option ("adapter",        'a', UNSIGNED);
    option ("device-name",    'd', STRING);
    option ("timeout",         0,  UNSIGNED);
    option ("receive-timeout", 0,  UNSIGNED);
#if defined (__linux)
    option ("buffer-size",    'b', UNSIGNED);
    option ("demux",           0,  UNSIGNED);
    option ("dvr",             0,  UNSIGNED);
    option ("frontend",        0,  UNSIGNED);
    option ("s2api",          '2');
#elif defined (__windows)
    option ("queue-size",      0, UNSIGNED);
#endif

    setHelp ("Options:\n"
             "\n"
             "  -a N\n"
             "  --adapter N\n"
#if defined (__linux)
             "      Specifies the Linux DVB adapter N (/dev/dvb/adapterN).\n"
#elif defined (__windows)
             "      Specifies the Nth DVB adapter in the system.\n"
#endif
             "      This option can be used instead of device name.\n"
             "      Use the tslsdvb utility to list all DVB devices.\n"
             "\n"
#if defined (__linux)
             "  -b value\n"
             "  --buffer-size value\n"
             "      Default buffer size, in bytes, of the demux device.\n"
             "      The default is 1 MB.\n"
             "\n"
             "  --demux N\n"
             "      Specifies the demux N in the adapter (/dev/dvb/adapter?/demuxN).\n"
             "      This option can be used instead of device name.\n"
             "\n"
#endif
             "  -d \"name\"\n"
             "  --device-name \"name\"\n"
#if defined (__linux)
             "      Specify the DVB receiver device name, /dev/dvb/adapterA[:F[:M[:V]]]\n"
             "      where A = adapter number, F = frontend number (default: 0), M = demux\n"
             "      number (default: 0), V = dvr number (default: 0). The options --adapter,\n"
             "      --frontend, --demux and --dvr can also be used instead of device name.\n"
#elif defined (__windows)
             "      Specify the DVB receiver device name. This is a DirectShow/BDA tuner\n"
             "      filter name (not case sensitive, blanks are ignored).\n"
#endif
             "      By default, the first DVB receiver device is used.\n"
             "      Use the tslsdvb utility to list all devices.\n"
             "\n"
#if defined (__linux)
             "  --dvr N\n"
             "      Specifies the DVR N in the adapter (/dev/dvb/adapter?/dvrN).\n"
             "      This is a legacy option which can be used instead of device name.\n"
             "\n"
             "  --frontend N\n"
             "      Specifies the frontend N in the adapter (/dev/dvb/adapter?/frontendN).\n"
             "      This is a legacy option which can be used instead of device name.\n"
             "\n"
#endif
             "  --help\n"
             "      Display this help text.\n"
             "\n"
#if defined (__windows)
             "  --queue-size value\n"
             "      Specify the maximum number of media samples in the queue between the\n"
             "      DirectShow capture thread and the input plugin thread. The default is\n"
             "      " + Decimal (Tuner::DEFAULT_SINK_QUEUE_SIZE) + " media samples.\n"
             "\n"
#endif
             "  --receive-timeout milliseconds\n"
             "      Specifies the timeout, in milliseconds, for each receive operation.\n"
             "      To disable the timeout and wait indefinitely for packets, specify zero.\n"
             "      This is the default.\n"
             "\n"
#if defined (__linux)
             "  -2\n"
             "  --s2api\n"
             "      On Linux kernel 2.6.28 and higher, this option forces the usage of the\n"
             "      S2API for communication with the DVB drivers. By default, for DVB-C and\n"
             "      DVB-T, the legacy Linux DVB API V3 is still used. The DVB-S and DVB-S2\n"
             "      tuners always use the S2API.\n"
             "\n"
#endif
             "  --timeout seconds\n"
             "      Specifies the timeout, in seconds, for DVB signal locking. If no signal\n"
             "      is detected after this timeout, the command aborts. To disable the\n"
             "      timeout and wait indefinitely for the signal, specify zero. The default\n"
             "      is " + Decimal (Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) + " seconds.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    // Define common tuning options
    TunerArgs::DefineOptions (*this, true);
    TunerArgs::AddHelp (*this, true);
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

    // Check if already open
    if (_tuner.isOpen()) {
        return false;
    }

    // Get command line options
    getValue (_device_name, "device-name");
    _tuner.setSignalTimeout (intValue ("timeout", Tuner::DEFAULT_SIGNAL_TIMEOUT / 1000) * 1000);
    if (!_tuner.setReceiveTimeout (intValue<MilliSecond> ("receive-timeout", 0), *tsp)) {
        return false;
    }

#if defined (__linux)
    _tuner.setForceS2API (present ("s2api"));
    _tuner.setSignalPoll (Tuner::DEFAULT_SIGNAL_POLL);
    _tuner.setDemuxBufferSize (intValue ("buffer-size", Tuner::DEFAULT_DEMUX_BUFFER_SIZE));
    if (present ("adapter") || present ("frontend") || present ("demux") || present ("dvr")) {
        if (_device_name.empty()) {
            _device_name = Format ("/dev/dvb/adapter%d:%d:%d:%d",
                                   intValue ("adapter", 0), intValue ("frontend", 0),
                                   intValue ("demux", 0), intValue ("dvr", 0));
        }
        else {
            error ("--adapter, --frontend, --demux or --dvr cannot be used with --device-name");
            return false;
        }
    }
#elif defined (__windows)
    _tuner.setSinkQueueSize (intValue ("queue-size", Tuner::DEFAULT_SINK_QUEUE_SIZE));
    if (present ("adapter")) {
        if (_device_name.empty()) {
            _device_name = Format (":%d", intValue ("adapter", 0));
        }
        else {
            error ("--adapter cannot be used with --device-name");
            return false;
        }
    }
#endif

    // Get common tuning options from command line
    _tuner_args.load (*this);
    if (!Args::valid()) {
        return false;
    }

    // Reinitialize other states
    _previous_bitrate = 0;

    // Open DVB adapter frontend
    if (!_tuner.open (_device_name, false, *tsp)) {
        return false;
    }
    tsp->verbose ("using " + _tuner.deviceName() + " (" + TunerTypeEnum.name (_tuner.tunerType()) + ")");

    // Allocate tuning parameters of the appropriate type
    _tuner_params = TunerParameters::Factory (_tuner.tunerType());
    assert (!_tuner_params.isNull());

    // Tune to transponder if tune options specified
    if (_tuner_args.hasTuningInfo()) {

        // Map command line options to actual tuning parameters
        if (!_tuner_params->fromTunerArgs (_tuner_args, *tsp)) {
            stop();
            return false;
        }
        tsp->verbose ("tuning to transponder " + _tuner_params->toPluginOptions());

        // Tune to transponder
        tsp->debug ("starting tuning");
        if (!_tuner.tune (*_tuner_params, *tsp)) {
            stop();
            return false;
        }
        tsp->debug ("tuning started");
    }

    // Start receiving packets
    tsp->debug ("starting tuner reception");
    if (!_tuner.start (*tsp)) {
        stop();
        return false;
    }
    tsp->debug ("tuner reception started");

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DVBInput::stop()
{
    _tuner.stop (*tsp);
    _tuner.close (*tsp);
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
