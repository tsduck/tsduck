//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHiDesControl.h"
#include "tsHiDesDevice.h"
#include "tsLegacyBandWidth.h"
#include "tsHFBand.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::HiDesControl::HiDesControl(int argc, char *argv[]) :
    Args(u"List HiDes modulator devices", u"[options]")
{
    option(u"adapter", 'a', UNSIGNED);
    help(u"adapter", u"Specify the HiDes adapter number to list. By default, list all HiDes devices.");

    DefineLegacyBandWidthArg(*this, u"bandwidth", 'b', 8000000);

    option(u"count", 'c');
    help(u"count", u"Only display the number of devices.");

    option(u"device", 'd', STRING);
    help(u"device", u"name",
         u"Specify the HiDes device name to list. "
         u"By default, list all HiDes devices.");

    option(u"frequency", 'f', POSITIVE);
    help(u"frequency",
         u"Frequency, in Hz, of the output carrier with --gain-range. "
         u"The default is the first UHF channel.");

    option(u"gain-range", 'g');
    help(u"gain-range",
         u"Display the allowed range of output gain for the specified device, "
         u"using the specified frequency and bandwidth.");

    analyze(argc, argv);

    LoadLegacyBandWidthArg(_bandwidth, *this, u"bandwidth", 8000000);
    _display_device_count = present(u"count");
    _display_gain_range = present(u"gain-range");
    getIntValue(_device_number, u"adapter", -1);
    getValue(_device_name, u"device");
    if (present(u"frequency")) {
        getIntValue(_frequency, u"frequency");
    }
    else {
        // Get UHF band description in the default region.
        const ts::HFBand* uhf = ts::HFBand::GetBand(u"", u"UHF", *this);
        _frequency = uhf->frequency(uhf->firstChannel());
    }

    if (_display_device_count && _display_gain_range) {
        error(u"--count and --gain-range are mutually exclusive");
    }

    exitOnError();
}

ts::HiDesControl::~HiDesControl()
{
}


//----------------------------------------------------------------------------
// Execute the HiDes control command.
//----------------------------------------------------------------------------

int ts::HiDesControl::execute()
{
#if defined(TS_NO_HIDES)

    error(u"This version of TSDuck was compiled without HiDes support");
    return EXIT_FAILURE;

#else

    HiDesDevice dev;
    HiDesDeviceInfo info;
    HiDesDeviceInfoList devices;
    const bool one_device = _device_number >= 0 || !_device_name.empty();
    bool ok = false;

    // Open one device or get all devices.
    if (!_display_gain_range && !one_device) {
        // Get all HiDes devices.
        ok = HiDesDevice::GetAllDevices(devices, *this);
    }
    else if (!_device_name.empty()) {
        // Open one device by name.
        ok = dev.open(_device_name, *this);
    }
    else {
        // One one device by number (default: first device).
        ok = dev.open(std::max<int>(0, _device_number), *this);
    }

    if (!ok) {
        return EXIT_FAILURE;
    }
    else if (_display_device_count) {
        // Display device count.
        std::cout << devices.size() << std::endl;
    }
    else if (_display_gain_range) {
        // Display gain range.
        int min, max;
        if (dev.getInfo(info, *this) && dev.getGainRange(min, max, _frequency, _bandwidth, *this)) {
            std::cout << ts::UString::Format(u"Device: %s", info.toString()) << std::endl
                      << ts::UString::Format(u"Frequency: %'d Hz", _frequency) << std::endl
                      << ts::UString::Format(u"Bandwidth: %'d Hz", _bandwidth) << std::endl
                      << ts::UString::Format(u"Min. gain: %d dB", min) << std::endl
                      << ts::UString::Format(u"Max. gain: %d dB", max) << std::endl;
        }
    }
    else if (one_device) {
        // Display one device.
        if (dev.getInfo(info, *this)) {
            std::cout << info.toString(verbose()) << std::endl;
        }
    }
    else if (devices.empty()) {
        std::cout << "No HiDes device found" << std::endl;
    }
    else {
        // Display all devices.
        if (verbose()) {
            std::cout << "Found " << devices.size() << " HiDes device" << (devices.size() > 1 ? "s" : "") << std::endl << std::endl;
        }
        for (const auto& it : devices) {
            std::cout << it.toString(verbose()) << std::endl;
        }
    }

    return valid() ? EXIT_SUCCESS : EXIT_FAILURE;


#endif // TS_NO_HIDES
}
