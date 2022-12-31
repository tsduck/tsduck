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
//  Output to HiDes modulator devices.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsHiDesDevice.h"
#include "tsModulationArgs.h"
#include "tsLegacyBandWidth.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HiDesOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(HiDesOutputPlugin);
    public:
        // Implementation of plugin API
        HiDesOutputPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;

    private:
        int             _dev_number;  // Device adapter number.
        UString         _dev_name;    // Device name.
        BitRate         _bitrate;     // Nominal output bitrate.
        HiDesDevice     _device;      // HiDes device object.
        HiDesDeviceInfo _dev_info;    // HiDes device information.
    };
}

TS_REGISTER_OUTPUT_PLUGIN(u"hides", ts::HiDesOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HiDesOutputPlugin::HiDesOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a HiDes modulator device", u"[options]"),
    _dev_number(-1),
    _dev_name(),
    _bitrate(0),
    _device(),
    _dev_info()
{
    option(u"adapter", 'a', UNSIGNED);
    help(u"adapter",
         u"Specify the HiDes adapter number to use. By default, the first HiDes "
         u"device is selected. Use the command tshides to list all HiDes devices.");

    DefineLegacyBandWidthArg(*this, u"bandwidth", 'b', 8000000);

    option(u"constellation", 'c', Enumeration({
        {u"QPSK",   QPSK},
        {u"16-QAM", QAM_16},
        {u"64-QAM", QAM_64},
    }));
    help(u"constellation", u"Constellation type. The default is 64-QAM.");

    option(u"dc-compensation", 0, STRING);
    help(u"dc-compensation", u"i-value/q-value",
         u"Specify the DC offset compensation values for I and Q. Each offset value "
         u"shall be in the range " + UString::Decimal(HiDesDevice::IT95X_DC_CAL_MIN) +
         u" to " + UString::Decimal(HiDesDevice::IT95X_DC_CAL_MAX) + u".");

    option(u"device", 'd', STRING);
    help(u"device", u"name",
         u"Specify the HiDes device name to use. By default, the first HiDes device "
         u"is selected. Use the command tshides to list all HiDes devices.");

    option(u"frequency", 'f', POSITIVE);
    help(u"frequency",
         u"Frequency, in Hz, of the output carrier. This parameter is mandatory. There is no default.");

    option(u"gain", 0, INT32);
    help(u"gain",
         u"Adjust the output gain to the specified value in dB. "
         u"The allowed gain range depends on the device, the frequency and the bandwidth.");

    option(u"guard-interval", 'g', Enumeration({
        {u"1/32", GUARD_1_32},
        {u"1/16", GUARD_1_16},
        {u"1/8",  GUARD_1_8},
        {u"1/4",  GUARD_1_4},
    }));
    help(u"guard-interval", u"Guard interval. The default is 1/32.");

    option(u"high-priority-fec", 'h', Enumeration({
        {u"1/2", FEC_1_2},
        {u"2/3", FEC_2_3},
        {u"3/4", FEC_3_4},
        {u"5/6", FEC_5_6},
        {u"7/8", FEC_7_8},
    }));
    help(u"high-priority-fec", u"Error correction for high priority streams. The default is 2/3.");

    option(u"spectral-inversion", 's', SpectralInversionEnum);
    help(u"spectral-inversion", u"Spectral inversion. The default is auto.");

    option(u"transmission-mode", 't', Enumeration({
        {u"2K", TM_2K},
        {u"4K", TM_4K},
        {u"8K", TM_8K},
    }));
    help(u"transmission-mode", u"Transmission mode. The default is 8K.");
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::HiDesOutputPlugin::start()
{
    if (_device.isOpen()) {
        tsp->error(u"already started");
        return false;
    }

    // Get options.
    _dev_number = intValue<int>(u"adapter", -1);
    _dev_name = value(u"device");
    const bool set_gain = present(u"gain");
    const int gain = intValue<int>(u"gain");
    const bool set_dc = present(u"dc-compensation");
    const UString dc_string(value(u"dc-compensation"));
    int dc_i = 0;
    int dc_q = 0;

    ModulationArgs params;
    LoadLegacyBandWidthArg(params.bandwidth, *this, u"bandwidth");
    params.modulation = intValue<Modulation>(u"constellation", QAM_64);
    params.frequency = intValue<uint64_t>(u"frequency", 0);
    params.guard_interval = intValue<GuardInterval>(u"guard-interval", GUARD_1_32);
    params.fec_hp = intValue<InnerFEC>(u"high-priority-fec", FEC_2_3);
    params.inversion = intValue<SpectralInversion>(u"spectral-inversion", SPINV_AUTO);
    params.transmission_mode = intValue<TransmissionMode>(u"transmission-mode", TM_8K);

    // Check option consistency.
    if (_dev_number < 0 && _dev_name.empty()) {
        // Use first device by default.
        _dev_number = 0;
    }
    else if (_dev_number >= 0 && !_dev_name.empty()) {
        tsp->error(u"specify either HiDes adapter number or device name but not both");
        return false;
    }
    if (params.frequency == 0) {
        tsp->error(u"no carrier frequency specified");
        return false;
    }
    if (set_dc && (!dc_string.scan(u"%d/%d", {&dc_i, &dc_q}) ||
                   dc_i < HiDesDevice::IT95X_DC_CAL_MIN ||
                   dc_i > HiDesDevice::IT95X_DC_CAL_MAX ||
                   dc_q < HiDesDevice::IT95X_DC_CAL_MIN ||
                   dc_q > HiDesDevice::IT95X_DC_CAL_MAX))
    {
        tsp->error(u"invalid DC compensation value \"%s\"", {dc_string});
        return false;
    }

    // Nominal output bitrate is computed from the modulation parameters.
    _bitrate = params.theoreticalBitrate();

    // Open the device, either by number or by name.
    if (_dev_number >= 0 && !_device.open(_dev_number, *tsp)) {
        return false;
    }
    if (!_dev_name.empty() && !_device.open(_dev_name, *tsp)) {
        return false;
    }
    if (!_device.getInfo(_dev_info, *tsp)) {
        _device.close(*tsp);
        return false;
    }
    tsp->verbose(u"using device %s with nominal output bitrate of %'d bits/s", {_dev_info.toString(),_bitrate});

    // Tune to frequency.
    if (!_device.tune(params, *tsp)) {
        _device.close(*tsp);
        return false;
    }

    // Adjust output gain if required.
    if (set_gain) {
        int new_gain = gain;
        if (!_device.setGain(new_gain, *tsp)) {
            _device.close(*tsp);
            return false;
        }
        // The value of gain is updated to effective value.
        tsp->verbose(u"adjusted output gain, requested %d dB, set to %d dB", {gain, new_gain});
    }

    // Set DC calibration
    if (set_dc && !_device.setDCCalibration(dc_i, dc_q, *tsp)) {
        _device.close(*tsp);
        return false;
    }

    // Start transmission.
    if (!_device.startTransmission(*tsp)) {
        _device.close(*tsp);
        return false;
    }

    // Now fully ready to transmit.
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::HiDesOutputPlugin::stop()
{
    return _device.stopTransmission(*tsp) && _device.close(*tsp);
}


//----------------------------------------------------------------------------
// Bitrate computation method
//----------------------------------------------------------------------------

ts::BitRate ts::HiDesOutputPlugin::getBitrate()
{
    // Was computed once, during start().
    return _bitrate;
}

ts::BitRateConfidence ts::HiDesOutputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the HiDes device hardware.
    return BitRateConfidence::HARDWARE;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::HiDesOutputPlugin::send(const TSPacket* pkt, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    return _device.send(pkt, packet_count, *tsp, tsp);
}
