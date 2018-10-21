//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//-----------------------------------------------------------------------------

#include "tsDektecDevice.h"
#include "tsDektecUtils.h"
TSDUCK_SOURCE;

#if defined(TS_NO_DTAPI)
bool tsDektecDeviceIsEmpty = true; // Avoid warning about empty module.
#else


//-----------------------------------------------------------------------------
// Return the error message corresponding to a DTAPI error code
//-----------------------------------------------------------------------------

ts::UString ts::DektecStrError(Dtapi::DTAPI_RESULT status)
{
    return UString::FromUTF8(Dtapi::DtapiResult2Str(status)) + UString::Format(u" (DTAPI status %d)", {status});
}


//-----------------------------------------------------------------------------
// Get the list of all Dektec ports in the system.
//-----------------------------------------------------------------------------

bool ts::DektecDevice::GetAllPorts(DektecPortDescVector& ports, bool is_input, bool is_output, bool is_bidirectional, Report& report)
{
    // According to some old version of DTAPI doc, the number of hardware functions is limited.
    // First, try with this limit.
    ports.resize(DTA_MAX_HW_FUNC);
    int count = 0;
    Dtapi::DTAPI_RESULT status = Dtapi::DtapiHwFuncScan(int(ports.size()), count, &ports[0]);

    // If the actual number of functions is larger, increase size
    if (status == DTAPI_E_BUF_TOO_SMALL && count > int(ports.size())) {
        ports.resize(size_t(count));
        status = Dtapi::DtapiHwFuncScan(int(ports.size()), count, &ports[0]);
    }

    // Report errors
    if (status != DTAPI_OK) {
        ports.clear();
        report.error(u"error getting Dektec hardware function list: %s", {DektecStrError(status)});
        return false;
    }

    // Adjust vector size
    assert(count >= 0);
    assert(count <= int(ports.size()));
    ports.resize(size_t(count));

    // Remove non-input or non-output ports
    for (DektecPortDescVector::iterator it = ports.begin(); it != ports.end(); ) {
        const bool port_is_input = (it->m_ChanType & DTAPI_CHAN_INPUT) != 0;
        const bool port_is_output = (it->m_ChanType & DTAPI_CHAN_OUTPUT) != 0;
        const bool port_is_bidirectional = port_is_input && port_is_output;
        if ((is_input && (port_is_input || (is_bidirectional && port_is_bidirectional))) ||
            (is_output && (port_is_output || (is_bidirectional && port_is_bidirectional)))) {
            // Keep this one
            ++it;
        }
        else {
            // Remove this one
            it = ports.erase(it);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Get a string description of a Dektec device
//-----------------------------------------------------------------------------

ts::UString ts::DektecDevice::GetDeviceDescription(const Dtapi::DtDeviceDesc& dev)
{
    // Flawfinder: ignore: statically-sized arrays can be improperly restricted.
    char str[64];
    Dtapi::DTAPI_RESULT status = Dtapi::DtapiDtDeviceDesc2String(const_cast<Dtapi::DtDeviceDesc*>(&dev),
                                                                 DTAPI_DVC2STR_TYPE_NMB,
                                                                 str,
                                                                 sizeof(str));
    if (status == DTAPI_OK) {
        str[sizeof(str) - 1] = '\0';
        return UString::FromUTF8(str);
    }
    else {
        // Forge a name
        return UString::Format(u"DT%c-%03d", {dev.m_Category == DTAPI_CAT_PCI ? u'A' : u'U', dev.m_TypeNumber});
    }
}


//-----------------------------------------------------------------------------
// Get a string description of a Dektec port.
//-----------------------------------------------------------------------------

ts::UString ts::DektecDevice::GetPortDescription(const Dtapi::DtHwFuncDesc& port)
{
    // Start with device description
    UString desc(GetDeviceDescription(port.m_DvcDesc));

    // Add port number.
    desc += UString::Format(u" port %d", {port.m_Port});

    // For devices with multiple ports, indicate which one is the top-most.
    if (port.m_Port == 1 && port.m_DvcDesc.m_NumPorts > 1) {
        desc += u" (top)";
    }

    // Interface type
    desc += u", ";
    desc += GetInterfaceDescription(port);

    // IP and MAC address (TS-over-IP)
    if ((port.m_Flags & DTAPI_CAP_IP) != 0) {
        desc += UString::Format(u", IP %d.%d.%d.%d, MAC %02X:%02X:%02X:%02X:%02X:%02X",
                                {port.m_Ip[0] & 0xFF, port.m_Ip[1] & 0xFF,
                                 port.m_Ip[2] & 0xFF, port.m_Ip[3] & 0xFF,
                                 port.m_MacAddr[0] & 0xFF, port.m_MacAddr[1] & 0xFF,
                                 port.m_MacAddr[2] & 0xFF, port.m_MacAddr[3] & 0xFF,
                                 port.m_MacAddr[4] & 0xFF, port.m_MacAddr[5] & 0xFF});
    }

    // Device capabilities.
    const UString caps(DtCapsToString(port.m_Flags));
    if (!caps.empty()) {
        desc += u" (";
        desc += caps;
        desc += u")";
    }

    return desc;
}


//-----------------------------------------------------------------------------
// Append a name to a string if a condition is true.
//-----------------------------------------------------------------------------

void ts::DektecDevice::OneCap(UString& str, bool condition, const UString& name)
{
    if (condition) {
        if (!str.empty()) {
            str += u", ";
        }
        str += name;
    }
}

void ts::DektecDevice::OneCap(UString& str, Dtapi::DtCaps cap, const UString& name)
{
    OneCap(str, cap != 0, name);
}


//-----------------------------------------------------------------------------
// Get a string description of a Dektec interface type
//-----------------------------------------------------------------------------

ts::UString ts::DektecDevice::GetInterfaceDescription(const Dtapi::DtHwFuncDesc& port)
{
    UString desc;

    OneCap(desc, (port.m_Flags & DTAPI_CAP_ASI) != 0, u"ASI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SDI) != 0, u"SDI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_MOD) != 0, u"Modulator");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_IP)  != 0, u"TS-over-IP");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SPI) != 0, u"SPI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SPISDI) != 0, u"SPI_SDI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_VIRTUAL) != 0, u"Virtual Stream");

    // If none found, use DTAPI function
    if (desc.empty()) {
        // Flawfinder: ignore: statically-sized arrays can be improperly restricted.
        char str[64];
        Dtapi::DTAPI_RESULT status = Dtapi::DtapiDtHwFuncDesc2String(const_cast<Dtapi::DtHwFuncDesc*>(&port),
                                                                     DTAPI_HWF2STR_ITF_TYPE,
                                                                     str,
                                                                     sizeof(str));
        str[status == DTAPI_OK ? sizeof(str) - 1 : 0] = '\0';
        desc = UString::FromUTF8(str);
    }

    return desc;
}


//-----------------------------------------------------------------------------
// Get the list of all Dektec devices in the system.
//-----------------------------------------------------------------------------

bool ts::DektecDevice::GetAllDevices(DektecDeviceVector& devices, Report& report)
{
    Dtapi::DTAPI_RESULT status;

    // Clear result buffer

    devices.clear();

    // Get the list of all "hardware functions" (ie. channels or port)

    DektecPortDescVector hw_desc;
    if (!GetAllPorts(hw_desc, true, true, true, report)) {
        return false;
    }
    if (hw_desc.size() == 0) {
        return true; // no dektec device
    }

    // Get the list of devices. Normally, there cannot be more devices
    // than functions since each device holds at least one function.

    int dev_desc_count = 0;
    DektecDeviceDescVector dev_desc(hw_desc.size());

    status = Dtapi::DtapiDeviceScan(int(dev_desc.size()), dev_desc_count, &dev_desc[0]);

    if (status != DTAPI_OK) {
        report.error(u"error getting Dektec device list: " + DektecStrError(status));
        return false;
    }

    assert(dev_desc_count >= 0);
    assert(dev_desc_count <= int(dev_desc.size()));
    dev_desc.resize(size_t(dev_desc_count));

    // Populate the result vector

    devices.resize (dev_desc_count);
    for (size_t dev = 0; dev < dev_desc.size(); ++dev) {

        devices[dev].desc = dev_desc[dev];
        devices[dev].input.clear();
        devices[dev].output.clear();
        devices[dev].model = GetDeviceDescription (devices[dev].desc);

        // Look for the hardware capabilities on this device.
        for (size_t hw = 0; hw < hw_desc.size(); ++hw) {
            if (hw_desc[hw].m_DvcDesc.m_Serial == dev_desc[dev].m_Serial) {
                // This hardware function applies to current device
                const int type = hw_desc[hw].m_ChanType;
                if ((type & DTAPI_CHAN_INPUT) != 0) {
                    // This function has input capability
                    devices[dev].input.push_back(hw_desc[hw]);
                }
                if ((type & DTAPI_CHAN_OUTPUT) != 0) {
                    // This function has output capability
                    devices[dev].output.push_back(hw_desc[hw]);
                }
            }
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// Get a Dektec device description. Return true on success.
//-----------------------------------------------------------------------------

bool ts::DektecDevice::getDevice(int& dev_index, int& chan_index, bool is_input, Report& report)
{
    const UChar* direction = is_input ? u"input" : u"output";

    // Get all Dektec devices in the system
    DektecDeviceVector devlist;
    if (!GetAllDevices(devlist, report)) {
        return false;
    }

    // Check device validity
    if (dev_index < 0) {
        // No device is specified, look for the first device with specified capability.
        for (size_t di = 0; di < devlist.size(); di++) {
            const DektecDevice& dev(devlist[di]);
            if ((is_input && dev.input.size() > 0) || (!is_input && dev.output.size() > 0)) {
                dev_index = int(di);
                break;
            }
        }
        if (dev_index < 0) {
            report.error(u"no %s Dektec device found", {direction});
            return false;
        }
    }
    else if (dev_index >= int(devlist.size())) {
        // Invalid device index specified
        report.error(u"invalid Dektec device index: %d", {dev_index});
        return false;
    }

    // Found device
    *this = devlist[dev_index];

    // Check that device has required input or output capability
    if ((is_input && input.size() <= 0) || (!is_input && output.size() <= 0)) {
        report.error(u"Dektec device %d has no %s channel", {dev_index, direction});
        return false;
    }

    // Check channel index
    if (chan_index < 0) {
        chan_index = 0;
    }
    if ((is_input && chan_index >= int(input.size())) || (!is_input && chan_index >= int(output.size()))) {
        report.error(u"Dektec device %d has no %s channel %d", {dev_index, direction, chan_index});
        return false;
    }

    // Report selected device
    report.verbose(u"using Dektec device %d, %s channel %d (%s port %d)",
                   {dev_index, direction, chan_index, model, is_input ? input[chan_index].m_Port : output[chan_index].m_Port});
    return true;
}


//-----------------------------------------------------------------------------
// Get a string description of one Dektec device capability by index
//-----------------------------------------------------------------------------

ts::UString ts::DektecDevice::DtCapsIndexToString(int index)
{
    // Manually built from DTAPI.h
    static const UChar* const names[] = {
        // Capability group APPS - Applications
        /* 0 */   u"C2Xpert",
        /* 1 */   u"DtGrabber+ and DtTV",
        /* 2 */   u"DtTV",
        /* 3 */   u"DtEncode",
        /* 4 */   u"DtJitter",
        /* 5 */   u"J2K engine",
        /* 6 */   u"MuxXpert runtime",
        /* 7 */   u"MuxXpert SDK",
        /* 8 */   u"MuxXpert",
        /* 9 */   u"StreamXpress remote control",
        /* 10 */  u"RFXpert",
        /* 11 */  u"StreamXpert Lite",
        /* 12 */  u"StreamXpress stream player",
        /* 13 */  u"StreamXpress through local NIC",
        /* 14 */  u"StreamXpert analyzer",
        /* 15 */  u"StreamXpert via local NIC (dongled)",
        /* 16 */  u"SdEye",
        /* 17 */  u"Xpect",
        /* 18 */  u"T2Xpert",
        /* 19 */  u"VF-REC",
        /* 20 */  u"VF-REC (dongled)",

        // Capability group AUDENC - Supported audio standards
        /* 21 */  u"AAC audio encoder",
        /* 22 */  u"AC3 audio encoder",
        /* 23 */  u"GOLD for audio encoder",
        /* 24 */  u"GOLD for two audio encoders",
        /* 25 */  u"MPEG1-layer II audio encoder",

        // Capability group BOOLIO - Boolean I/O capabilities
        /* 26 */  u"26 (DEPRECATED)",
        /* 27 */  u"A fail-over relay is available",
        /* 28 */  u"Fractional mode is supported",
        /* 29 */  u"Locked to a genlock reference",
        /* 30 */  u"Genlock reference input",
        /* 31 */  u"DVB-S2 APSK mode",

        // Capability group DEMODPROPS - Demodulation properties
        /* 32 */  u"Antenna power",
        /* 33 */  u"LNB",
        /* 34 */  u"Advanced demodulation",

        // Capability group FREQBAND - Frequency band
        /* 35 */  u"L-band 950-2150MHz",
        /* 36 */  u"VHF-band 47-470MHz",
        /* 37 */  u"UHF-band 400-862MHz",

        // Capability group HDMISTD - HDMI standard
        /* 38 */  u"HDMI 1.4",
        /* 39 */  u"HDMI 2.0",

        // Capability group IODIR - I/O direction
        /* 40 */  u"Port is disabled",
        /* 41 */  u"Uni-directional input",
        /* 42 */  u"Internal input port",
        /* 43 */  u"Monitor of input or output",
        /* 44 */  u"Uni-directional output",

        // Subcapabilities of IODIR, DTAPI_CAP_INPUT
        /* 45 */  u"Get antenna signal from another port",

        // Subcapabilities of IODIR, DTAPI_CAP_OUTPUT
        /* 46 */  u"Double buffered output",
        /* 47 */  u"Loop-through of DVB-S2 in L3-frames",
        /* 48 */  u"Loop-through of an DVB-S(2) input",
        /* 49 */  u"Loop-through of another input",

        // Capability group IOPROPS - Miscellaneous I/O properties
        /* 50 */  u"ASI output signal can be inverted",
        /* 51 */  u"Slaved genlock reference",
        /* 52 */  u"Huffman coding for SDI",
        /* 53 */  u"Network port supports failover",
        /* 54 */  u"L3-frame mode",
        /* 55 */  u"Matrix API support",
        /* 56 */  u"High-level Matrix API support",
        /* 57 */  u"Raw ASI",
        /* 58 */  u"10-bit network byte order",
        /* 59 */  u"SDI timestamping",
        /* 60 */  u"64-bit timestamping",
        /* 61 */  u"Transparent mode",
        /* 62 */  u"MPEG-2 transport stream",
        /* 63 */  u"Transmit on timestamp",
        /* 64 */  u"Virtual port, no physical connection",

        // Capability group IOSTD - I/O standard
        /* 65 */  u"12G-SDI",
        /* 66 */  u"3G-SDI",
        /* 67 */  u"6G-SDI",
        /* 68 */  u"DVB-ASI transport stream",
        /* 69 */  u"Audio/video encoder",
        /* 70 */  u"Demodulation",
        /* 71 */  u"1PPS and 10MHz GPS-clock input",
        /* 72 */  u"HDMI",
        /* 73 */  u"HD-SDI",
        /* 74 */  u"IF A/D converter",
        /* 75 */  u"Transport stream over IP",
        /* 76 */  u"Modulator output",
        /* 77 */  u"Phase noise injection",
        /* 78 */  u"RS422 port",
        /* 79 */  u"SDI receiver",
        /* 80 */  u"SD-SDI",
        /* 81 */  u"DVB-SPI transport stream",
        /* 82 */  u"SD-SDI on a parallel port",

        // Subcapabilities of IOSTD, DTAPI_CAP_12GSDI
        /* 83 */  u"2160p/50 lvl A",
        /* 84 */  u"2160p/50 lvl B",
        /* 85 */  u"2160p/59.94 lvl A",
        /* 86 */  u"2160p/59.94 lvl B",
        /* 87 */  u"2160p/60 lvl A",
        /* 88 */  u"2160p/60 lvl B",

        // Subcapabilities of IOSTD, DTAPI_CAP_3GSDI
        /* 89 */  u"1080p/50 lvl A",
        /* 90 */  u"1080p/50 lvl B",
        /* 91 */  u"1080p/59.94 lvl A",
        /* 92 */  u"1080p/59.94 lvl B",
        /* 93 */  u"1080p/60 lvl A",
        /* 94 */  u"1080p/60 lvl B",

        // Subcapabilities of IOSTD, DTAPI_CAP_6GSDI
        /* 95 */  u"2160p/23.98",
        /* 96 */  u"2160p/24",
        /* 97 */  u"2160p/25",
        /* 98 */  u"2160p/29.97",
        /* 99 */  u"2160p/30",

        // Subcapabilities of IOSTD, DTAPI_CAP_HDSDI
        /* 100 */ u"1080i/50",
        /* 101 */ u"1080i/59.94",
        /* 102 */ u"1080i/60",
        /* 103 */ u"1080p/23.98",
        /* 104 */ u"1080p/24",
        /* 105 */ u"1080p/25",
        /* 106 */ u"1080p/29.97",
        /* 107 */ u"1080p/30",
        /* 108 */ u"1080psf/23.98",
        /* 109 */ u"1080psf/24",
        /* 110 */ u"1080psf/25",
        /* 111 */ u"1080psf/29.97",
        /* 112 */ u"1080psf/30",
        /* 113 */ u"720p/23.98",
        /* 114 */ u"720p/24",
        /* 115 */ u"720p/25",
        /* 116 */ u"720p/29.97",
        /* 117 */ u"720p/30",
        /* 118 */ u"720p/50",
        /* 119 */ u"720p/59.94",
        /* 120 */ u"720p/60",

        // Subcapabilities of IOSTD, DTAPI_CAP_SDI
        /* 121 */ u"525i/59.94",
        /* 122 */ u"625i/50",

        // Subcapabilities of IOSTD, DTAPI_CAP_SPISDI
        /* 123 */ u"SPI 525i/59.94",
        /* 124 */ u"SPI 625i/50",

        // Capability group PWRMODE - Power mode
        /* 125 */ u"High-quality modulation",
        /* 126 */ u"Low-power mode",

        // Capability group MODSTD - Modulation standards
        /* 127 */ u"ATSC 8-VSB modulation",
        /* 128 */ u"ATSC3.0 modulation",
        /* 129 */ u"CMMB modulation",
        /* 130 */ u"DAB modulation",
        /* 131 */ u"DTMB modulation",
        /* 132 */ u"DVB-C2 modulation",
        /* 133 */ u"DVB-S modulation",
        /* 134 */ u"DVB-S2 modulation",
        /* 135 */ u"DVB-S2X modulation",
        /* 136 */ u"DVB-T modulation",
        /* 137 */ u"DVB-T2 modulation",
        /* 138 */ u"GOLD for modulators",
        /* 139 */ u"Eight-channel HW modulation",
        /* 140 */ u"I/Q sample modulation",
        /* 141 */ u"ISDB-S modulation",
        /* 142 */ u"ISDB-T modulation",
        /* 143 */ u"ISDB-Tmm modulation",
        /* 144 */ u"ATSC-MH modulation",
        /* 145 */ u"QAM-A modulation",
        /* 146 */ u"QAM-B modulation",
        /* 147 */ u"QAM-C modulation",
        /* 148 */ u"SW multi-channel modulation",
        /* 149 */ u"T2MI transmission",
        /* 150 */ u"DVB-T2 single PLP modulation",

        // Capability group MODPROPS - Modulation properties
        /* 151 */ u"Adjustable output level",
        /* 152 */ u"Channel simulation",
        /* 153 */ u"Continuous wave",
        /* 154 */ u"Digital I/Q sample output",
        /* 155 */ u"DVB carrier ID ",
        /* 156 */ u"IF output",
        /* 157 */ u"Mute RF output signal",
        /* 158 */ u"Adjustable roll-off factor",
        /* 159 */ u"DVB-S2 16-APSK/32-APSK",
        /* 160 */ u"AWGN insertion",
        /* 161 */ u"16MHz bandwidth mode",
        /* 162 */ u"SNF operation",

        // Capability group RFCLKSEL - RF clock source selection
        /* 163 */ u"External RF clock input",
        /* 164 */ u"Internal RF clock reference",

        // Capability group RXSTD - Receiver standards
        /* 165 */ u"ATSC 8-VSB reception",
        /* 166 */ u"ATSC3.0 reception",
        /* 167 */ u"CMMB reception",
        /* 168 */ u"DAB reception",
        /* 169 */ u"DTMB reception",
        /* 170 */ u"DVB-C2 reception",
        /* 171 */ u"DVB-S reception",
        /* 172 */ u"DVB-S2 reception",
        /* 173 */ u"DVB-T reception",
        /* 174 */ u"DVB-T2 reception",
        /* 175 */ u"GOLD for receivers",
        /* 176 */ u"I/Q sample reception",
        /* 177 */ u"ISDB-S reception",
        /* 178 */ u"ISDB-T reception",
        /* 179 */ u"ATSC-MH reception",
        /* 180 */ u"QAM-A reception",
        /* 181 */ u"QAM-B reception",
        /* 182 */ u"QAM-C reception",
        /* 183 */ u"T2MI reception",

        // Capability group SPICLKSEL - Parallel port clock source selection
        /* 184 */ u"External clock input",
        /* 185 */ u"Internal clock reference",

        // Capability group SPIMODE - Parallel port mode
        /* 186 */ u"SPI fixed clock with valid signal",
        /* 187 */ u"SPI DVB mode",
        /* 188 */ u"SPI serial 8-bit mode",
        /* 189 */ u"SPI serial 10-bit mode",

        // Capability group SPISTD - Parallel port I/O standard
        /* 190 */ u"LVDS1",
        /* 191 */ u"LVDS2",
        /* 192 */ u"LVTTL",

        // Capability group TSRATESEL - Transport-stream rate selection
        /* 193 */ u"External TS rate clock input",
        /* 194 */ u"External TS rate clock with ratio",
        /* 195 */ u"Internal TS rate clock reference",
        /* 196 */ u"Lock TS rate to input port",

        // Capability group VIDENC - Supported video standards
        /* 197 */ u"H.264 video encoder",
        /* 198 */ u"MPEG2 video encoder",

        #define TS_DT_CAPS_LAST 198   // UPDATE WHEN NEW LINES ARE ADDED
    };

    static const int names_count = int(sizeof(names) / sizeof(names[0]));
    assert(names_count == TS_DT_CAPS_LAST + 1);

    const UChar* const str = (index < 0 || index >= names_count) ? nullptr : names[index];
    return (str == nullptr || str[0] == 0) ? UString::Decimal(index) : UString(str);
}


//-----------------------------------------------------------------------------
// Get a string description of a set of Dektec capabilities
//-----------------------------------------------------------------------------

ts::UString ts::DektecDevice::DtCapsToString(const Dtapi::DtCaps& flags)
{
    // Normally, this function should be as simple as;
    // return UString::FromUTF8(flags.ToString());

    // However, there are several issues.
    //
    // With GCC, starting with GCC 5.1, the ABI of std::string has changed
    // (see https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html).
    // Up to now, DTAPI is compiled only with an old version of the compiler (pre 5.1)
    // and we cannot use methods returning std::string.
    //
    // With Visual Studio, using a DTU-315 Universal modulator, the method
    // Dtapi::DtCaps::ToString() returns only "LBAND", which is only the first value.
    //
    // As a consequence, we build the list manually.
    // As of DTAPI 5.24, there are no more than 256 capabilities.

    UString caps;
    for (int c = 0; c < 256; c++) {
        if ((flags & Dtapi::DtCaps(c)) != 0) {
            if (!caps.empty()) {
                caps += u", ";
            }
            caps += DtCapsIndexToString(c);
        }
    }
    return caps;
}


//-----------------------------------------------------------------------------
// Display various Dektec data structure for debug
//-----------------------------------------------------------------------------

void ts::DektecDevice::ReportDvbT2Pars(const Dtapi::DtDvbT2Pars& pars, Report& report, int severity, const UString& margin)
{
    report.log(severity, u"%sm_T2Version = %d", {margin, pars.m_T2Version});
    report.log(severity, u"%sm_Bandwidth = %d", {margin, pars.m_Bandwidth});
    report.log(severity, u"%sm_FftMode = %d", {margin, pars.m_FftMode});
    report.log(severity, u"%sm_Miso = %d", {margin, pars.m_Miso});
    report.log(severity, u"%sm_GuardInterval = %d", {margin, pars.m_GuardInterval});
    report.log(severity, u"%sm_Papr = %d", {margin, pars.m_Papr});
    report.log(severity, u"%sm_BwtExt = %d", {margin, pars.m_BwtExt});
    report.log(severity, u"%sm_PilotPattern = %d", {margin, pars.m_PilotPattern});
    report.log(severity, u"%sm_L1Modulation = %d", {margin, pars.m_L1Modulation});
    report.log(severity, u"%sm_CellId = %d", {margin, pars.m_CellId});
    report.log(severity, u"%sm_NetworkId = %d", {margin, pars.m_NetworkId});
    report.log(severity, u"%sm_T2SystemId = %d", {margin, pars.m_T2SystemId});
    report.log(severity, u"%sm_L1Repetition = %d", {margin, pars.m_L1Repetition});
    report.log(severity, u"%sm_NumT2Frames = %d", {margin, pars.m_NumT2Frames});
    report.log(severity, u"%sm_NumDataSyms = %d", {margin, pars.m_NumDataSyms});
    report.log(severity, u"%sm_NumSubslices = %d", {margin, pars.m_NumSubslices});
    report.log(severity, u"%sm_FefEnable = %d", {margin, pars.m_FefEnable});
    report.log(severity, u"%sm_FefType = %d", {margin, pars.m_FefType});
    report.log(severity, u"%sm_FefS1 = %d", {margin, pars.m_FefS1});
    report.log(severity, u"%sm_FefS2 = %d", {margin, pars.m_FefS2});
    report.log(severity, u"%sm_FefSignal = %d", {margin, pars.m_FefSignal});
    report.log(severity, u"%sm_FefLength = %d", {margin, pars.m_FefLength});
    report.log(severity, u"%sm_FefInterval = %d", {margin, pars.m_FefInterval});
    report.log(severity, u"%sm_NumRfChans = %d", {margin, pars.m_NumRfChans});
    for (int i = 0; i < DTAPI_DVBT2_NUM_RF_MAX && i < pars.m_NumRfChans; i++) {
        report.log(severity, u"%sm_RfChanFreqs[%d] = %d", {margin, i, pars.m_RfChanFreqs[i]});
    }
    report.log(severity, u"%sm_StartRfIdx = %d", {margin, pars.m_StartRfIdx});
    report.log(severity, u"%sm_NumPlps = %d", {margin, pars.m_NumPlps});
    for (int i = 0; i < DTAPI_DVBT2_NUM_PLP_MAX && i < pars.m_NumPlps; i++) {
        ReportDvbT2PlpPars(pars.m_Plps[i], report, severity, UString::Format(u"%sm_Plps[%d].", {margin, i}));
    }
}

void ts::DektecDevice::ReportDvbT2PlpPars(const Dtapi::DtDvbT2PlpPars& pars, Report& report, int severity, const UString& margin)
{
    report.log(severity, u"%sm_Hem = %d", {margin, pars.m_Hem});
    report.log(severity, u"%sm_Npd = %d", {margin, pars.m_Npd});
    report.log(severity, u"%sm_Issy = %d", {margin, pars.m_Issy});
    report.log(severity, u"%sm_IssyBufs = %d", {margin, pars.m_IssyBufs});
    report.log(severity, u"%sm_IssyTDesign = %d", {margin, pars.m_IssyTDesign});
    report.log(severity, u"%sm_CompensatingDelay = %d", {margin, pars.m_CompensatingDelay});
    report.log(severity, u"%sm_TsRate = %d", {margin, pars.m_TsRate});
    report.log(severity, u"%sm_Id = %d", {margin, pars.m_Id});
    report.log(severity, u"%sm_GroupId = %d", {margin, pars.m_GroupId});
    report.log(severity, u"%sm_Type = %d", {margin, pars.m_Type});
    report.log(severity, u"%sm_CodeRate = %d", {margin, pars.m_CodeRate});
    report.log(severity, u"%sm_Modulation = %d", {margin, pars.m_Modulation});
    report.log(severity, u"%sm_Rotation = %d", {margin, pars.m_Rotation});
    report.log(severity, u"%sm_FecType = %d", {margin, pars.m_FecType});
    report.log(severity, u"%sm_FrameInterval = %d", {margin, pars.m_FrameInterval});
    report.log(severity, u"%sm_FirstFrameIdx = %d", {margin, pars.m_FirstFrameIdx});
    report.log(severity, u"%sm_TimeIlLength = %d", {margin, pars.m_TimeIlLength});
    report.log(severity, u"%sm_TimeIlType = %d", {margin, pars.m_TimeIlType});
    report.log(severity, u"%sm_InBandAFlag = %d", {margin, pars.m_InBandAFlag});
    report.log(severity, u"%sm_InBandBFlag = %d", {margin, pars.m_InBandBFlag});
    report.log(severity, u"%sm_NumBlocks = %d", {margin, pars.m_NumBlocks});
    report.log(severity, u"%sm_NumOtherPlpInBand = %d", {margin, pars.m_NumOtherPlpInBand});
    for (int i = 0; i < DTAPI_DVBT2_NUM_PLP_MAX - 1 && i < pars.m_NumOtherPlpInBand; i++) {
        report.log(severity, u"%sm_OtherPlpInBand[%d] = %d", {margin, i, pars.m_OtherPlpInBand[i]});
    }
    report.log(severity, u"%sm_FfFlag = %d", {margin, pars.m_FfFlag});
    report.log(severity, u"%sm_FirstRfIdx = %d", {margin, pars.m_FirstRfIdx});
}

void ts::DektecDevice::ReportDvbT2ParamInfo(const Dtapi::DtDvbT2ParamInfo& pars, Report& report, int severity, const UString& margin)
{
    report.log(severity, u"%sm_TotalCellsPerFrame = %d", {margin, pars.m_TotalCellsPerFrame});
    report.log(severity, u"%sm_L1CellsPerFrame = %d", {margin, pars.m_L1CellsPerFrame});
    report.log(severity, u"%sm_DummyCellsPerFrame = %d", {margin, pars.m_DummyCellsPerFrame});
}

#endif // TS_NO_DTAPI
