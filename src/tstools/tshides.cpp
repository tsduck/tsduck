//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Control HiDes modulator devices.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsHFBand.h"
#include "tsHiDesDevice.h"
#include "tsLegacyBandWidth.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class HiDesOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(HiDesOptions);
    public:
        HiDesOptions(int argc, char *argv[]);

        bool          count = false;       // Only display device count.
        bool          gain_range = false;  // Only display output gain range.
        int           dev_number = -1;     // Device adapter number.
        ts::UString   dev_name {};         // Device name.
        uint64_t      frequency = 0;       // Carrier frequency, in Hz.
        ts::BandWidth bandwidth = 0;       // Bandwidth.
    };
}

HiDesOptions::HiDesOptions(int argc, char *argv[]) :
    ts::Args(u"List HiDes modulator devices", u"[options]")
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

    LoadLegacyBandWidthArg(bandwidth, *this, u"bandwidth", 8000000);
    count = present(u"count");
    gain_range = present(u"gain-range");
    getIntValue(dev_number, u"adapter", -1);
    getValue(dev_name, u"device");
    if (present(u"frequency")) {
        getIntValue(frequency, u"frequency");
    }
    else {
        // Get UHF band description in the default region.
        const ts::HFBand* uhf = ts::HFBand::GetBand(u"", u"UHF", *this);
        frequency = uhf->frequency(uhf->firstChannel());
    }

    if (count && gain_range) {
        error(u"--count and --gain-range are mutually exclusive");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    HiDesOptions opt(argc, argv);
    ts::HiDesDevice dev;
    ts::HiDesDeviceInfo info;
    ts::HiDesDeviceInfoList devices;
    const bool one_device = opt.dev_number >= 0 || !opt.dev_name.empty();
    bool ok = false;

    // Open one device or get all devices.
    if (!opt.gain_range && !one_device) {
        // Get all HiDes devices.
        ok = ts::HiDesDevice::GetAllDevices(devices, opt);
    }
    else if (!opt.dev_name.empty()) {
        // Open one device by name.
        ok = dev.open(opt.dev_name, opt);
    }
    else {
        // One one device by number (default: first device).
        ok = dev.open(std::max<int>(0, opt.dev_number), opt);
    }

    if (!ok) {
        return EXIT_FAILURE;
    }
    else if (opt.count) {
        // Display device count.
        std::cout << devices.size() << std::endl;
    }
    else if (opt.gain_range) {
        // Display gain range.
        int min, max;
        if (dev.getInfo(info, opt) && dev.getGainRange(min, max, opt.frequency, opt.bandwidth, opt)) {
            std::cout << ts::UString::Format(u"Device: %s", info.toString()) << std::endl
                << ts::UString::Format(u"Frequency: %'d Hz", opt.frequency) << std::endl
                << ts::UString::Format(u"Bandwidth: %'d Hz", opt.bandwidth) << std::endl
                << ts::UString::Format(u"Min. gain: %d dB", min) << std::endl
                << ts::UString::Format(u"Max. gain: %d dB", max) << std::endl;
        }
    }
    else if (one_device) {
        // Display one device.
        if (dev.getInfo(info, opt)) {
            std::cout << info.toString(opt.verbose()) << std::endl;
        }
    }
    else if (devices.empty()) {
        std::cout << "No HiDes device found" << std::endl;
    }
    else {
        // Display all devices.
        if (opt.verbose()) {
            std::cout << "Found " << devices.size() << " HiDes device" << (devices.size() > 1 ? "s" : "") << std::endl << std::endl;
        }
        for (const auto& it : devices) {
            std::cout << it.toString(opt.verbose()) << std::endl;
        }
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
