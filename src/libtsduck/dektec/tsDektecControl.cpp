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

#include "tsDektecControl.h"
#include "tsDektec.h"


//----------------------------------------------------------------------------
// Stubs when DTAPI is not supported
//----------------------------------------------------------------------------

#if defined(TS_NO_DTAPI)

ts::DektecControl::DektecControl(int argc, char *argv[]) :
    Args(u"Control Dektec devices (unimplemented)"),
    _duck(this),
    _guts(nullptr)
{
}

ts::DektecControl::~DektecControl()
{
}

int ts::DektecControl::execute()
{
    error(TS_NO_DTAPI_MESSAGE);
    return EXIT_FAILURE;
}

#else


//----------------------------------------------------------------------------
// Start of real implementation using DTAPI.
//----------------------------------------------------------------------------

#include "tsDuckContext.h"
#include "tsSysUtils.h"
#include "tsDektecUtils.h"
#include "tsDektecDevice.h"
#include "tsDektecVPD.h"
#include "tsjsonOutputArgs.h"
#include "tsjson.h"
#include "tsjsonObject.h"
#include "tsjsonString.h"
#include "tsjsonNumber.h"


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::DektecControl::Guts
{
    TS_NOBUILD_NOCOPY(Guts);
private:
    Report& _report;
public:
    bool    _list_all;       // List all Dektec devices
    bool    _normalized;     // List in "normalized" format
    json::OutputArgs _json;  // List in JSON format
    int     _wait_sec;       // Wait time before exit
    size_t  _devindex;       // Dektec device
    bool    _reset;          // Reset the device
    bool    _set_led;        // Change LED state
    int     _led_state;      // State of the LED (one of DTAPI_LED_*)
    int     _set_input;      // Port number to set as input, for directional ports
    int     _set_output;     // Port number to set as output, for directional ports
    int     _power_mode;     // Power mode to set on DTU-315

    // Constructor
    Guts(Report& report);

    // Apply commands to one device. Return command status.
    int oneDevice(const DektecDevice& device);

    // Displays a list of all Dektec devices. Return command status.
    int listDevices(const DektecDeviceVector& devices);

    // Displays a list of all Dektec devices in normalized format. Return command status.
    int listNormalizedDevices(const DektecDeviceVector& devices);

    // Displays the capability of a hardware function in normalized format.
    void listNormalizedCapabilities(size_t device_index, size_t channel_index, const char* type, const Dtapi::DtHwFuncDesc& hw);

    // Displays a list of all Dektec devices in JSON format. Return command status.
    int listDevicesJSON(const DektecDeviceVector& devices);

    // Displays the capability of a hardware function in JSON format.
    void listCapabilitiesJSON(ts::json::Value&, size_t device_index, size_t channel_index, const Dtapi::DtHwFuncDesc& hw);

    // Display a long line on multiple lines
    void wideDisplay(const UString& line);
};


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::DektecControl::Guts::Guts(Report& report) :
    _report(report),
    _list_all(false),
    _normalized(false),
    _json(),
    _wait_sec(0),
    _devindex(0),
    _reset(false),
    _set_led(false),
    _led_state(0),
    _set_input(0),
    _set_output(0),
    _power_mode(-1)
{
}

ts::DektecControl::DektecControl(int argc, char *argv[]) :
    Args(u"Control Dektec devices", u"[options] [device]"),
    _duck(this),
    _guts(new Guts(*this))
{
    option(u"", 0, UNSIGNED, 0, 1);
    help(u"",
         u"Device index, from 0 to N-1 (with N being the number of Dektec devices "
         u"in the system). The default is 0. Use option --all to have a "
         u"complete list of devices in the system.");

    option(u"all", 'a');
    help(u"all", u"List all Dektec devices available on the system. ");

    option(u"input", 'i', POSITIVE);
    help(u"input", u"port-number",
         u"Set the specified port in input mode. This applies to bidirectional "
         u"ports which can be either set in input or output mode.");

    option(u"led", 'l', Enumeration({
        {u"off",      DTAPI_LED_OFF},
        {u"green",    DTAPI_LED_GREEN},
        {u"red",      DTAPI_LED_RED},
        {u"yellow",   DTAPI_LED_YELLOW},
        {u"blue",     DTAPI_LED_BLUE},
        {u"hardware", DTAPI_LED_HARDWARE},
    }));
    help(u"led", u"state",
         u"Set the state of the LED on the rear panel. Useful to identify a "
         u"Dektec device when more than one is present. See also "
         u"option --wait (the led state is automatically returned to "
         u"\"hardware\" after exit).");

    _guts->_json.defineArgs(*this, true, u"With --all, list the Dektec devices in JSON format (useful for automatic analysis).");

    option(u"normalized", 'n');
    help(u"normalized", u"With --all, list the Dektec devices in a normalized output format (useful for automatic analysis).");

    option(u"output", 'o', POSITIVE);
    help(u"output", u"port-number",
         u"Set the specified port in output mode. This applies to bidirectional "
         u"ports which can be either set in input or output mode.");

    option(u"power-mode", 'p', DektecPowerMode);
    help(u"power-mode", u"On DTU-315 USB modulators, set the power mode to the specified value.");

    option(u"reset", 'r');
    help(u"reset", u"Reset the device.");

    option(u"wait", 'w', UNSIGNED);
    help(u"wait", u"seconds",
         u"Wait the specified number of seconds before exiting. The default "
         u"if 5 seconds if option --led is specified and 0 otherwise.");

    analyze(argc, argv);

    _guts->_devindex   = intValue(u"", 0);
    _guts->_list_all   = present(u"all");
    _guts->_normalized = present(u"normalized");
    _guts->_reset      = present(u"reset");
    _guts->_set_led    = present(u"led");
    _guts->_led_state  = intValue(u"led", DTAPI_LED_OFF);
    _guts->_set_input  = intValue(u"input", -1);
    _guts->_set_output = intValue(u"output", -1);
    _guts->_wait_sec   = intValue(u"wait", _guts->_set_led ? 5 : 0);
    _guts->_power_mode = intValue(u"power-mode", -1);
    _guts->_json.loadArgs(_duck, *this);

    if (_guts->_json.useFile() && _guts->_normalized) {
        error(u"options --json and --normalized are mutually exclusive");
    }

    exitOnError();
}

ts::DektecControl::~DektecControl()
{
    if (_guts != nullptr) {
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Execute the dektec control command.
//----------------------------------------------------------------------------

int ts::DektecControl::execute()
{
    DektecDeviceVector devices;
    DektecDevice::GetAllDevices(devices);

    if (_guts->_list_all) {
        // List all devices
        if (_guts->_json.useJSON()) {
            return _guts->listDevicesJSON(devices);
        }
        else if (_guts->_normalized) {
            return _guts->listNormalizedDevices(devices);
        }
        else {
            return _guts->listDevices(devices);
        }
    }
    else {
        // List only one device
        if (_guts->_devindex >= devices.size()) {
            // Invalid device index specified
            error(u"invalid device index: %d", {_guts->_devindex});
            return EXIT_FAILURE;
        }
        else {
            return _guts->oneDevice(devices[_guts->_devindex]);
        }
    }
}


//----------------------------------------------------------------------------
// Display a long line on multiple lines
//----------------------------------------------------------------------------

void ts::DektecControl::Guts::wideDisplay(const UString& line)
{
    UStringVector lines;
    line.splitLines(lines, 80, u".,;:)", u"      ");
    for (size_t i = 0; i < lines.size(); ++i) {
        std::cout << lines[i] << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display a list of all Dektec devices. Return main() status.
//----------------------------------------------------------------------------

int ts::DektecControl::Guts::listDevices(const DektecDeviceVector& devices)
{
    // Display DTAPI and device drivers versions
    if (_report.verbose()) {
        std::cout << std::endl
                  << GetDektecVersions() << std::endl
                  << std::endl;
    }

    // Display device list
    for (size_t index = 0; index < devices.size(); index++) {

        const DektecDevice& device(devices[index]);
        DektecVPD vpd(device.desc);
        Dtapi::DtDevice dtdev;

        // Print short info
        std::cout << (_report.verbose() ? "* Device " : "") << index << ": " << device.model;
        if (vpd.vpdid[0] != 0) {
            std::cout << " (" << vpd.vpdid << ")";
        }
        std::cout << std::endl;

        // Print verbose info
        if (_report.verbose()) {
            std::cout << "  Physical ports: " << device.desc.m_NumPorts << std::endl
                      << "  Channels: input: " << device.input.size()
                      << ", output: " << device.output.size() << std::endl;
            for (size_t i = 0; i < device.input.size(); i++) {
                wideDisplay(UString::Format(u"  Input %d: %s", {i, DektecDevice::GetPortDescription(device.input[i])}));
            }
            for (size_t i = 0; i < device.output.size(); i++) {
                wideDisplay(UString::Format(u"  Output %d: %s", {i, DektecDevice::GetPortDescription(device.output[i])}));
            }
            std::cout << UString::Format(u"  Subsystem id: 0x%04X", {device.desc.m_SubsystemId})
                      << " (" << device.model << ")" << std::endl
                      << UString::Format(u"  Subsystem vendor id: 0x%04X", {device.desc.m_SubVendorId}) << std::endl
                      << UString::Format(u"  Device id: 0x%04X", {device.desc.m_DeviceId}) << std::endl
                      << UString::Format(u"  Vendor id: 0x%04X", {device.desc.m_VendorId}) << std::endl
                      << UString::Format(u"  Serial number: %016X", {device.desc.m_Serial}) << std::endl
                      << UString::Format(u"  Firmware version: %d (0x%08X)", {device.desc.m_FirmwareVersion, device.desc.m_FirmwareVersion}) << std::endl
                      << UString::Format(u"  Firmware variant: %d (0x%08X)", {device.desc.m_FirmwareVariant, device.desc.m_FirmwareVariant}) << std::endl;

            switch (device.desc.m_Category) {
                case DTAPI_CAT_PCI:
                    std::cout << "  PCI bus: " << device.desc.m_PciBusNumber
                              << ", slot: " << device.desc.m_SlotNumber << std::endl;
                    break;
                case DTAPI_CAT_USB:
                    std::cout << "  USB address: " << device.desc.m_UsbAddress << std::endl;
                    break;
                default:
                    break;
            }

            if (vpd.cl[0] != 0) {
                std::cout << "  Customer id: " << vpd.cl << std::endl;
            }
            if (vpd.ec[0] != 0) {
                std::cout << "  Engineering change level: " << vpd.ec << std::endl;
            }
            if (vpd.mn[0] != 0) {
                std::cout << "  Manufacture id: " << vpd.mn << std::endl;
            }
            if (vpd.pd[0] != 0) {
                std::cout << "  Production date: " << vpd.pd << std::endl;
            }
            if (vpd.pn[0] != 0) {
                std::cout << "  Part number: " << vpd.pn << std::endl;
            }
            if (vpd.sn[0] != 0) {
                std::cout << "  Serial number: " << vpd.sn << std::endl;
            }
            if (vpd.xt[0] != 0) {
                std::cout << "  Crystal stability: " << vpd.xt << std::endl;
            }
            if (vpd.bo[0] != 0) {
                std::cout << "  Bitrate offset: " << vpd.bo << std::endl;
            }
            std::cout << std::endl;
        }
    }

    return EXIT_SUCCESS;
}


//----------------------------------------------------------------------------
// Display the capability of a hardware function in normalized format.
//----------------------------------------------------------------------------

void ts::DektecControl::Guts::listNormalizedCapabilities(size_t device_index, size_t channel_index, const char* type, const Dtapi::DtHwFuncDesc& hw)
{
    std::cout << "channel:" << type << ":"
        << "device=" << device_index << ":"
        << "channel=" << channel_index << ":"
        << "port=" << hw.m_Port << ":"
        << ((hw.m_Flags & DTAPI_CAP_ASI) != 0 ? "asi:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPI) != 0 ? "spi:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SDI) != 0 ? "sdi:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPISDI) != 0 ? "spi-sdi:" : "")
        << ((hw.m_Flags & DTAPI_CAP_MOD) != 0 ? "modulator:" : "")
        << ((hw.m_Flags & DTAPI_CAP_VIRTUAL) != 0 ? "virtual-stream:" : "")
        << ((hw.m_Flags & DTAPI_CAP_DBLBUF) != 0 ? "double-buffer:" : "")
        << ((hw.m_Flags & DTAPI_CAP_IP) != 0 ? "ts-over-ip:" : "")
        << ((hw.m_Flags & DTAPI_CAP_FAILSAFE) != 0 ? "failsafe:" : "")
        << ((hw.m_Flags & DTAPI_CAP_LOOPTHR) != 0 ? "loop-through:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TRPMODE) != 0 ? "transparent:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SDITIME) != 0 ? "sdi-time-stamp:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TIMESTAMP64) != 0 ? "sdi-time-stamp-64:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TXONTIME) != 0 ? "transmit-on-time-stamp:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_ATSC) != 0 ? "atsc:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_CMMB) != 0 ? "cmmb:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DTMB) != 0 ? "dtmb:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DVBC2) != 0 ? "dvb-c2:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DVBS) != 0 ? "dvb-s:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DVBS2) != 0 ? "dvb-s2:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DVBT) != 0 ? "dvb-t:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_DVBT2) != 0 ? "dvb-t2:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_IQ) != 0 ? "iq-samples:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_ISDBS) != 0 ? "isdb-s:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_ISDBT) != 0 ? "isdb-t:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_QAMA) != 0 ? "qam:qam-a:dvb-c:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_QAMB) != 0 ? "qam:qam-b:" : "")
        << ((hw.m_Flags & DTAPI_CAP_TX_QAMC) != 0 ? "qam:qam-c:" : "")
        << ((hw.m_Flags & DTAPI_CAP_VHF) != 0 ? "vhf:" : "")
        << ((hw.m_Flags & DTAPI_CAP_UHF) != 0 ? "uhf:" : "")
        << ((hw.m_Flags & DTAPI_CAP_LBAND) != 0 ? "lband:" : "")
        << ((hw.m_Flags & DTAPI_CAP_IF) != 0 ? "if-output:" : "")
        << ((hw.m_Flags & DTAPI_CAP_DIGIQ) != 0 ? "iq-output:" : "")
        << ((hw.m_Flags & DTAPI_CAP_ADJLVL) != 0 ? "adjust-level:" : "")
        << ((hw.m_Flags & DTAPI_CAP_IFADC) != 0 ? "access-downconverted:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SHAREDANT) != 0 ? "shared-input:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SNR) != 0 ? "snr-setting:" : "")
        << ((hw.m_Flags & DTAPI_CAP_CM) != 0 ? "channel-modelling:" : "")
        << ((hw.m_Flags & DTAPI_CAP_RAWASI) != 0 ? "asi-raw-10bit:" : "")
        << ((hw.m_Flags & DTAPI_CAP_LOCK2INP) != 0 ? "lock-io-rate:" : "")
        << ((hw.m_Flags & DTAPI_CAP_EXTTSRATE) != 0 ? "dedicated-clock-input:" : "")
        << ((hw.m_Flags & DTAPI_CAP_EXTRATIO) != 0 ? "dedicated-clock-input-ratio:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPICLKEXT) != 0 ? "spi-external-clock:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPILVDS1) != 0 ? "lvds1:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPILVDS2) != 0 ? "lvds2:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPILVTTL) != 0 ? "lvttl:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPICLKINT) != 0 ? "spi-fixed-clock:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPISER10B) != 0 ? "spi-serial-10-bit:" : "")
        << ((hw.m_Flags & DTAPI_CAP_SPISER8B) != 0 ? "spi-serial-8-bit:" : "");

    if ((hw.m_Flags & DTAPI_CAP_IP) != 0) {
        std::cout << UString::Format(u"ip=%d.%d.%d.%d:mac=%02X-%02X-%02X-%02X-%02X-%02X:",
                                     {hw.m_Ip[0] & 0xFF, hw.m_Ip[1] & 0xFF,
                                      hw.m_Ip[2] & 0xFF, hw.m_Ip[3] & 0xFF,
                                      hw.m_MacAddr[0] & 0xFF, hw.m_MacAddr[1] & 0xFF,
                                      hw.m_MacAddr[2] & 0xFF, hw.m_MacAddr[3] & 0xFF,
                                      hw.m_MacAddr[4] & 0xFF, hw.m_MacAddr[5] & 0xFF});
    }

    std::cout << std::endl;
}


//----------------------------------------------------------------------------
// List all Dektec devices in normalized format. Return main() status.
//----------------------------------------------------------------------------

int ts::DektecControl::Guts::listNormalizedDevices(const DektecDeviceVector& devices)
{
    Dtapi::DTAPI_RESULT status;

    // Display DTAPI and device drivers versions
    int maj, min, bugfix, build;
    Dtapi::DtapiGetVersion(maj, min, bugfix, build);
    std::cout << "dtapi:version=" << maj << "." << min << "." << bugfix << "." << build << ":" << std::endl;

    status = Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_PCI, maj, min, bugfix, build);
    if (status == DTAPI_OK) {
        std::cout << "driver:pci:version=" << maj << "." << min << "." << bugfix << "." << build << ":" << std::endl;
    }

    status = Dtapi::DtapiGetDeviceDriverVersion(DTAPI_CAT_USB, maj, min, bugfix, build);
    if (status == DTAPI_OK) {
        std::cout << "driver:usb:version=" << maj << "." << min << "." << bugfix << "." << build << ":" << std::endl;
    }

    // Display device list
    for (size_t index = 0; index < devices.size(); index++) {

        const DektecDevice& device(devices[index]);
        DektecVPD vpd(device.desc);
        Dtapi::DtDevice dtdev;

        std::cout << "device:device=" << index << ":model=" << device.model << ":";
        switch (device.desc.m_Category) {
            case DTAPI_CAT_PCI:
                std::cout << "pci:"
                    << "bus=" << device.desc.m_PciBusNumber << ":"
                    << "slot=" << device.desc.m_SlotNumber << ":";
                break;
            case DTAPI_CAT_USB:
                std::cout << "usb:address=" << device.desc.m_UsbAddress << ":";
                break;
            default:
                break;
        }
        std::cout << "nb-port=" << device.desc.m_NumPorts << ":"
            << "nb-input=" << device.input.size() << ":"
            << "nb-output=" << device.output.size() << ":"
            << "subsys-id=" << device.desc.m_SubsystemId << ":"
            << "subsys-vendor-id=" << device.desc.m_SubVendorId << ":"
            << "device-id=" << device.desc.m_DeviceId << ":"
            << "vendor-id=" << device.desc.m_VendorId << ":"
            << "serial=" << uint64_t(device.desc.m_Serial) << ":"
            << "fw-version=" << device.desc.m_FirmwareVersion << ":"
            << "fw-variant=" << device.desc.m_FirmwareVariant << ":";
        if (vpd.vpdid[0] != 0) {
            std::cout << "vpd-id=" << vpd.vpdid << ":";
        }
        if (vpd.cl[0] != 0) {
            std::cout << "vpd-cl=" << vpd.cl << ":";
        }
        if (vpd.ec[0] != 0) {
            std::cout << "vpd-ec=" << vpd.ec << ":";
        }
        if (vpd.mn[0] != 0) {
            std::cout << "vpd-mn=" << vpd.mn << ":";
        }
        if (vpd.pd[0] != 0) {
            std::cout << "vpd-pd=" << vpd.pd << ":";
        }
        if (vpd.pn[0] != 0) {
            std::cout << "vpd-pn=" << vpd.pn << ":";
        }
        if (vpd.xt[0] != 0) {
            std::cout << "vpd-xt=" << vpd.xt << ":";
        }
        if (vpd.bo[0] != 0) {
            std::cout << "vpd-bo=" << vpd.bo << ":";
        }
        std::cout << std::endl;

        for (size_t i = 0; i < device.input.size(); i++) {
            listNormalizedCapabilities(index, i, "input", device.input[i]);
        }
        for (size_t i = 0; i < device.output.size(); i++) {
            listNormalizedCapabilities(index, i, "output", device.output[i]);
        }
    }

    return EXIT_SUCCESS;
}


//----------------------------------------------------------------------------
// Display the capability of a hardware function in JSON format.
//----------------------------------------------------------------------------

void ts::DektecControl::Guts::listCapabilitiesJSON(ts::json::Value& jv, size_t device_index, size_t channel_index, const Dtapi::DtHwFuncDesc &hw)
{
    jv.add(u"device", device_index);
    jv.add(u"channel", channel_index);
    jv.add(u"port", hw.m_Port);
    jv.add(u"asi", json::Bool((hw.m_Flags & DTAPI_CAP_ASI) != 0));
    jv.add(u"spi", json::Bool((hw.m_Flags & DTAPI_CAP_SPI) != 0));
    jv.add(u"sdi", json::Bool((hw.m_Flags & DTAPI_CAP_SDI) != 0));
    jv.add(u"spi-sdi", json::Bool((hw.m_Flags & DTAPI_CAP_SPISDI) != 0));
    jv.add(u"modulator", json::Bool((hw.m_Flags & DTAPI_CAP_MOD) != 0));
    jv.add(u"virtual-stream", json::Bool((hw.m_Flags & DTAPI_CAP_VIRTUAL) != 0));
    jv.add(u"double-buffer", json::Bool((hw.m_Flags & DTAPI_CAP_DBLBUF) != 0));
    jv.add(u"ts-over-ip", json::Bool((hw.m_Flags & DTAPI_CAP_IP) != 0));
    jv.add(u"failsafe", json::Bool((hw.m_Flags & DTAPI_CAP_FAILSAFE) != 0));
    jv.add(u"loop-through", json::Bool((hw.m_Flags & DTAPI_CAP_LOOPTHR) != 0));
    jv.add(u"transparent", json::Bool((hw.m_Flags & DTAPI_CAP_TRPMODE) != 0));
    jv.add(u"sdi-time-stamp", json::Bool((hw.m_Flags & DTAPI_CAP_SDITIME) != 0));
    jv.add(u"sdi-time-stamp-64", json::Bool((hw.m_Flags & DTAPI_CAP_TIMESTAMP64) != 0));
    jv.add(u"transmit-on-time-stamp", json::Bool((hw.m_Flags & DTAPI_CAP_TXONTIME) != 0));
    jv.add(u"atsc", json::Bool((hw.m_Flags & DTAPI_CAP_TX_ATSC) != 0));
    jv.add(u"cmmb", json::Bool((hw.m_Flags & DTAPI_CAP_TX_CMMB) != 0));
    jv.add(u"dtmb", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DTMB) != 0));
    jv.add(u"dvb-c2", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DVBC2) != 0));
    jv.add(u"dvb-s", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DVBS) != 0));
    jv.add(u"dvb-s2", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DVBS2) != 0));
    jv.add(u"dvb-t", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DVBT) != 0));
    jv.add(u"dvb-t2", json::Bool((hw.m_Flags & DTAPI_CAP_TX_DVBT2) != 0));
    jv.add(u"iq-samples", json::Bool((hw.m_Flags & DTAPI_CAP_TX_IQ) != 0));
    jv.add(u"isdb-s", json::Bool((hw.m_Flags & DTAPI_CAP_TX_ISDBS) != 0));
    jv.add(u"isdb-t", json::Bool((hw.m_Flags & DTAPI_CAP_TX_ISDBT) != 0));
    jv.add(u"qam:qam-a:dvb-c", json::Bool((hw.m_Flags & DTAPI_CAP_TX_QAMA) != 0));
    jv.add(u"qam:qam-b", json::Bool((hw.m_Flags & DTAPI_CAP_TX_QAMB) != 0));
    jv.add(u"qam:qam-c", json::Bool((hw.m_Flags & DTAPI_CAP_TX_QAMC) != 0));
    jv.add(u"vhf", json::Bool((hw.m_Flags & DTAPI_CAP_VHF) != 0));
    jv.add(u"uhf", json::Bool((hw.m_Flags & DTAPI_CAP_UHF) != 0));
    jv.add(u"lband", json::Bool((hw.m_Flags & DTAPI_CAP_LBAND) != 0));
    jv.add(u"if-output", json::Bool((hw.m_Flags & DTAPI_CAP_IF) != 0));
    jv.add(u"iq-output", json::Bool((hw.m_Flags & DTAPI_CAP_DIGIQ) != 0));
    jv.add(u"adjust-level", json::Bool((hw.m_Flags & DTAPI_CAP_ADJLVL) != 0));
    jv.add(u"access-down-converted", json::Bool((hw.m_Flags & DTAPI_CAP_IFADC) != 0));
    jv.add(u"shared-input", json::Bool((hw.m_Flags & DTAPI_CAP_SHAREDANT) != 0));
    jv.add(u"snr-setting", json::Bool((hw.m_Flags & DTAPI_CAP_SNR) != 0));
    jv.add(u"channel-modelling", json::Bool((hw.m_Flags & DTAPI_CAP_CM) != 0));
    jv.add(u"asi-raw-10bit", json::Bool((hw.m_Flags & DTAPI_CAP_RAWASI) != 0));
    jv.add(u"lock-io-rate", json::Bool((hw.m_Flags & DTAPI_CAP_LOCK2INP) != 0));
    jv.add(u"dedicated-clock-input", json::Bool((hw.m_Flags & DTAPI_CAP_EXTTSRATE) != 0));
    jv.add(u"dedicated-clock-input-ratio", json::Bool((hw.m_Flags & DTAPI_CAP_EXTRATIO) != 0));
    jv.add(u"spi-external-clock", json::Bool((hw.m_Flags & DTAPI_CAP_SPICLKEXT) != 0));
    jv.add(u"lvds1", json::Bool((hw.m_Flags & DTAPI_CAP_SPILVDS1) != 0));
    jv.add(u"lvds2", json::Bool((hw.m_Flags & DTAPI_CAP_SPILVDS2) != 0));
    jv.add(u"lvttl", json::Bool((hw.m_Flags & DTAPI_CAP_SPILVTTL) != 0));
    jv.add(u"spi-fixed-clock", json::Bool((hw.m_Flags & DTAPI_CAP_SPICLKINT) != 0));
    jv.add(u"spi-serial-10-bit", json::Bool((hw.m_Flags & DTAPI_CAP_SPISER10B) != 0));
    jv.add(u"spi-serial-8-bit", json::Bool((hw.m_Flags & DTAPI_CAP_SPISER8B) != 0));

    if ((hw.m_Flags & DTAPI_CAP_IP) != 0) {
        jv.add(u"ip", UString::Format(u"%d.%d.%d.%d", {hw.m_Ip[0] & 0xFF, hw.m_Ip[1] & 0xFF, hw.m_Ip[2] & 0xFF, hw.m_Ip[3] & 0xFF}));
        jv.add(u"mac", UString::Format(u"%02X-%02X-%02X-%02X-%02X-%02X:",
                                       {hw.m_MacAddr[0] & 0xFF, hw.m_MacAddr[1] & 0xFF, hw.m_MacAddr[2] & 0xFF,
                                        hw.m_MacAddr[3] & 0xFF, hw.m_MacAddr[4] & 0xFF, hw.m_MacAddr[5] & 0xFF}));
    }
}


//----------------------------------------------------------------------------
// List all Dektec devices in JSON format. Return main() status.
//----------------------------------------------------------------------------

int ts::DektecControl::Guts::listDevicesJSON(const DektecDeviceVector& devices)
{
    json::Object root;

    // Display DTAPI and device drivers versions
    std::map<UString,UString> versions;
    GetDektecVersions(versions);
    for (const auto& it : versions) {
        root.query(u"versions", true).add(it.first, it.second);
    }

    // Display device list
    for (size_t index = 0; index < devices.size(); index++) {

        const DektecDevice& device(devices[index]);
        DektecVPD vpd(device.desc);
        Dtapi::DtDevice dtdev;
        json::Value& jdev(root.query(u"devices[]", true));

        jdev.add(u"index", index);
        jdev.add(u"model", device.model);

        switch (device.desc.m_Category) {
            case DTAPI_CAT_PCI:
                jdev.query(u"pci", true).add(u"bus", device.desc.m_PciBusNumber);
                jdev.query(u"pci", true).add(u"slot", device.desc.m_SlotNumber);
                break;
            case DTAPI_CAT_USB:
                jdev.query(u"usb", true).add(u"address", device.desc.m_UsbAddress);
                break;
            default:
                break;
        }

        jdev.add(u"nb-port", device.desc.m_NumPorts);
        jdev.add(u"nb-input", device.input.size());
        jdev.add(u"nb-output", device.output.size());
        jdev.add(u"subsys-id", device.desc.m_SubsystemId);
        jdev.add(u"subsys-vendor-id", device.desc.m_SubVendorId);
        jdev.add(u"device-id", device.desc.m_DeviceId);
        jdev.add(u"vendor-id", device.desc.m_VendorId);
        jdev.add(u"serial", int64_t(device.desc.m_Serial));
        jdev.add(u"fw-version", device.desc.m_FirmwareVersion);
        jdev.add(u"fw-variant", device.desc.m_FirmwareVariant);
        if (vpd.vpdid[0] != 0) {
            jdev.add(u"vpd-id", UString::FromUTF8(vpd.vpdid));
        }
        if (vpd.cl[0] != 0) {
            jdev.add(u"vpd-cl", UString::FromUTF8(vpd.cl));
        }
        if (vpd.ec[0] != 0) {
            jdev.add(u"vpd-ec", UString::FromUTF8(vpd.ec));
        }
        if (vpd.mn[0] != 0) {
            jdev.add(u"vpd-mn", UString::FromUTF8(vpd.mn));
        }
        if (vpd.pd[0] != 0) {
            jdev.add(u"vpd-pd", UString::FromUTF8(vpd.pd));
        }
        if (vpd.pn[0] != 0) {
            jdev.add(u"vpd-pn", UString::FromUTF8(vpd.pn));
        }
        if (vpd.xt[0] != 0) {
            jdev.add(u"vpd-xt", UString::FromUTF8(vpd.xt));
        }
        if (vpd.bo[0] != 0) {
            jdev.add(u"vpd-bo", UString::FromUTF8(vpd.bo));
        }

        for (size_t i = 0; i < device.input.size(); i++) {
            listCapabilitiesJSON(jdev.query(u"inputs[]", true), index, i, device.input[i]);
        }
        for (size_t i = 0; i < device.output.size(); i++) {
            listCapabilitiesJSON(jdev.query(u"outputs[]", true), index, i, device.output[i]);
        }
    }

    // JSON output.
    return _json.report(root, std::cout, _report) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//----------------------------------------------------------------------------
// Apply commands to one device. Return main() status.
//----------------------------------------------------------------------------

int ts::DektecControl::Guts::oneDevice(const DektecDevice& device)
{
    Dtapi::DtDevice dtdev;
    Dtapi::DTAPI_RESULT status;

    status = dtdev.AttachToSerial (device.desc.m_Serial);

    if (status != DTAPI_OK) {
        std::cerr << "* Error attaching device: " << DektecStrError(status) << std::endl;
        return EXIT_FAILURE;
    }

    if (_reset) {
        // Reset input channels
        for (size_t ci = 0; ci < device.input.size(); ci++) {
            Dtapi::DtInpChannel chan;
            status = chan.AttachToPort(&dtdev, device.input[ci].m_Port);
            if (status != DTAPI_OK) {
                std::cerr << "* Error attaching input channel " << ci
                          << ": " << DektecStrError(status) << std::endl;
            }
            else {
                if (_report.verbose()) {
                    std::cout << "Resetting input channel " << ci << std::endl;
                }
                status = chan.Reset (DTAPI_FULL_RESET);
                if (status != DTAPI_OK) {
                    std::cerr << "* Error resetting input channel " << ci
                              << ": " << DektecStrError(status) << std::endl;
                }
                chan.Detach(0);
            }
        }
        // Reset output channels
        for (size_t ci = 0; ci < device.output.size(); ci++) {
            Dtapi::DtOutpChannel chan;
            status = chan.AttachToPort(&dtdev, device.output[ci].m_Port);
            if (status != DTAPI_OK) {
                std::cerr << "* Error attaching output channel " << ci
                          << ": " << DektecStrError(status) << std::endl;
            }
            else {
                if (_report.verbose()) {
                    std::cout << "Resetting output channel " << ci << std::endl;
                }
                status = chan.Reset(DTAPI_FULL_RESET);
                if (status != DTAPI_OK) {
                    std::cerr << "* Error resetting output channel " << ci
                              << ": " << DektecStrError(status) << std::endl;
                }
                chan.Detach (0);
            }
        }
    }

    if (_set_led) {
        status = dtdev.LedControl(_led_state);
        if (status != DTAPI_OK) {
            std::cerr << "* Error setting LED: " << DektecStrError(status) << std::endl;
            dtdev.Detach();
            return EXIT_FAILURE;
        }
    }

    if (_power_mode >= 0) {
        // This is expected to work on DTA-315 modulator for which there is only one port.
        // Loop on all output ports, just in case.
        for (size_t ci = 0; ci < device.output.size(); ci++) {
            const int port = device.output[ci].m_Port;
            status = dtdev.SetIoConfig(port, DTAPI_IOCONFIG_PWRMODE, _power_mode);
            if (status != DTAPI_OK) {
                std::cerr << "* Error setting power mode on port " << port << ": " << DektecStrError(status) << std::endl;
                dtdev.Detach();
                return EXIT_FAILURE;
            }
        }
    }

    if (_set_input >= 1) {
        // Important: according to file CapList.xlsx (coming with DTAPI), the SetIoConfig IODIR with value INPUT
        // shall also specify INPUT as SubCap (aka SubValue). This is new with devices which are not only
        // bidirectional but which can also redirect input and output (internal loopback, antenna, etc).
        // In that case, SubCap INPUT means "input port coming from physical socket".
        status = dtdev.SetIoConfig(_set_input, DTAPI_IOCONFIG_IODIR, DTAPI_IOCONFIG_INPUT, DTAPI_IOCONFIG_INPUT);
        if (status != DTAPI_OK) {
            std::cerr << "* Error setting port " << _set_input << " to input mode: " << DektecStrError(status) << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (_set_output >= 1) {
        // Important: same comment as INPUT above, specify OUTPUT as SubCap to output physical output port.
        status = dtdev.SetIoConfig(_set_output, DTAPI_IOCONFIG_IODIR, DTAPI_IOCONFIG_OUTPUT, DTAPI_IOCONFIG_OUTPUT);
        if (status != DTAPI_OK) {
            std::cerr << "* Error setting port " << _set_output << " to output mode: " << DektecStrError(status) << std::endl;
            return EXIT_FAILURE;
        }
    }

    SleepThread(MilliSecPerSec * _wait_sec);
    dtdev.Detach();

    return EXIT_SUCCESS;
}

#endif // TS_NO_DTAPI
