//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_PLUGIN_CONSTRUCTORS(HiDesOutputPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
        virtual bool isRealTime() override {return true;}
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;

    private:
        int             _dev_number = -1;  // Device adapter number.
        UString         _dev_name {};      // Device name.
        BitRate         _bitrate = 0;      // Nominal output bitrate.
        HiDesDevice     _device {};        // HiDes device object.
        HiDesDeviceInfo _dev_info {};      // HiDes device information.
    };
}

TS_REGISTER_OUTPUT_PLUGIN(u"hides", ts::HiDesOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HiDesOutputPlugin::HiDesOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a HiDes modulator device", u"[options]")
{
    option(u"adapter", 'a', UNSIGNED);
    help(u"adapter",
         u"Specify the HiDes adapter number to use. By default, the first HiDes "
         u"device is selected. Use the command tshides to list all HiDes devices.");

    DefineLegacyBandWidthArg(*this, u"bandwidth", 'b', 8'000'000);

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

    option(u"spectral-inversion", 's', *SpectralInversionEnum);
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
        error(u"already started");
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
        error(u"specify either HiDes adapter number or device name but not both");
        return false;
    }
    if (*params.frequency == 0) {
        error(u"no carrier frequency specified");
        return false;
    }
    if (set_dc && (!dc_string.scan(u"%d/%d", &dc_i, &dc_q) ||
                   dc_i < HiDesDevice::IT95X_DC_CAL_MIN ||
                   dc_i > HiDesDevice::IT95X_DC_CAL_MAX ||
                   dc_q < HiDesDevice::IT95X_DC_CAL_MIN ||
                   dc_q > HiDesDevice::IT95X_DC_CAL_MAX))
    {
        error(u"invalid DC compensation value \"%s\"", dc_string);
        return false;
    }

    // Nominal output bitrate is computed from the modulation parameters.
    _bitrate = params.theoreticalBitrate();

    // Open the device, either by number or by name.
    if (_dev_number >= 0 && !_device.open(_dev_number, *this)) {
        return false;
    }
    if (!_dev_name.empty() && !_device.open(_dev_name, *this)) {
        return false;
    }
    if (!_device.getInfo(_dev_info, *this)) {
        _device.close(*this);
        return false;
    }
    verbose(u"using device %s with nominal output bitrate of %'d bits/s", _dev_info.toString(),_bitrate);

    // Tune to frequency.
    if (!_device.tune(params, *this)) {
        _device.close(*this);
        return false;
    }

    // Adjust output gain if required.
    if (set_gain) {
        int new_gain = gain;
        if (!_device.setGain(new_gain, *this)) {
            _device.close(*this);
            return false;
        }
        // The value of gain is updated to effective value.
        verbose(u"adjusted output gain, requested %d dB, set to %d dB", gain, new_gain);
    }

    // Set DC calibration
    if (set_dc && !_device.setDCCalibration(dc_i, dc_q, *this)) {
        _device.close(*this);
        return false;
    }

    // Start transmission.
    if (!_device.startTransmission(*this)) {
        _device.close(*this);
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
    return _device.stopTransmission(*this) && _device.close(*this);
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
    return _device.send(pkt, packet_count, *this, tsp);
}
