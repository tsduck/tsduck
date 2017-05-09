//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#include "tsDektecDevice.h"
#include "tsDektecUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"

#if defined(TS_NO_DTAPI)
bool tsDektecDeviceIsEmpty = true; // Avoid warning about empty module.
#else


//-----------------------------------------------------------------------------
// Return the error message corresponding to a DTAPI error code
//-----------------------------------------------------------------------------

std::string ts::DektecStrError(Dtapi::DTAPI_RESULT status)
{
    return Dtapi::DtapiResult2Str(status) + Format(" (DTAPI status %d)", int(status));
}


//-----------------------------------------------------------------------------
// Get the list of all Dektec ports in the system.
//-----------------------------------------------------------------------------

bool ts::DektecDevice::GetAllPorts(DektecPortDescVector& ports, bool is_input, bool is_output, bool is_bidirectional, ReportInterface& report)
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
        report.error("error getting Dektec hardware function list: " + DektecStrError(status));
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

std::string ts::DektecDevice::GetDeviceDescription(const Dtapi::DtDeviceDesc& dev)
{
    char str[64];
    Dtapi::DTAPI_RESULT status = Dtapi::DtapiDtDeviceDesc2String(const_cast<Dtapi::DtDeviceDesc*>(&dev), DTAPI_DVC2STR_TYPE_NMB, str, sizeof(str));
    if (status == DTAPI_OK) {
        str[sizeof(str) - 1] = '\0';
        return str;
    }
    else {
        // Forge a name
        return Format("DT%c-%03d", dev.m_Category == DTAPI_CAT_PCI ? 'A' : 'U', dev.m_TypeNumber);
    }
}


//-----------------------------------------------------------------------------
// Get a string description of a Dektec port.
//-----------------------------------------------------------------------------

std::string ts::DektecDevice::GetPortDescription(const Dtapi::DtHwFuncDesc& port)
{
    // Start with device description
    std::string desc(GetDeviceDescription(port.m_DvcDesc));

    // Add port number.
    desc += Format(" port %d", port.m_Port);

    // For devices with multiple ports, indicate which one is the top-most.
    if (port.m_Port == 1 && port.m_DvcDesc.m_NumPorts > 1) {
        desc += " (top)";
    }

    // Interface type
    desc += ", ";
    desc += GetInterfaceDescription(port);

    // IP and MAC address (TS-over-IP)
    if ((port.m_Flags & DTAPI_CAP_IP) != 0) {
        desc += Format (", IP %d.%d.%d.%d, MAC %02X:%02X:%02X:%02X:%02X:%02X",
                        int (port.m_Ip[0]) & 0xFF, int (port.m_Ip[1]) & 0xFF,
                        int (port.m_Ip[2]) & 0xFF, int (port.m_Ip[3]) & 0xFF,
                        int (port.m_MacAddr[0]) & 0xFF, int (port.m_MacAddr[1]) & 0xFF,
                        int (port.m_MacAddr[2]) & 0xFF, int (port.m_MacAddr[3]) & 0xFF,
                        int (port.m_MacAddr[4]) & 0xFF, int (port.m_MacAddr[5]) & 0xFF);
    }

    // Device capabilities.
    // With GCC, we have an issue here. Starting with GCC 5.1, the ABI of std::string
    // has changed (see https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html).
    // Up to DTAPI 5.24, DTAPI is compiled only with an old version of the compiler (pre 5.1).
    // When using DTAPI <= 5.25 with GCC >= 5.1.0, we cannot use methods handling strings.

    std::string caps;
#if defined(TS_GCC_VERSION) && (TS_GCC_VERSION >= 50100) && (TS_DTAPI_VERSION <= 525)
    // This is the configuration where we cannot use DtCaps::ToString.
    // Fallback to a list of integers. There is no public value to get the
    // maximum number of capabilities. As of DTAPI 5.24, there are 256 of them.
    for (int c = 0; c < 256; c++) {
        if ((port.m_Flags & Dtapi::DtCaps(c)) != 0) {
            if (!caps.empty()) {
                caps += ", ";
            }
            caps += Decimal(c);
        }
    }
#else
    caps = port.m_Flags.ToString();
#endif

    if (!caps.empty()) {
        desc += " (";
        desc += caps;
        desc += ")";
    }

    return desc;
}


//-----------------------------------------------------------------------------
// Append a name to a string if a condition is true.
//-----------------------------------------------------------------------------

void ts::DektecDevice::OneCap(std::string& str, bool condition, const std::string& name)
{
    if (condition) {
        if (!str.empty()) {
            str += ", ";
        }
        str += name;
    }
}

void ts::DektecDevice::OneCap(std::string& str, Dtapi::DtCaps cap, const std::string& name)
{
    OneCap(str, cap != 0, name);
}


//-----------------------------------------------------------------------------
// Get a string description of a Dektec interface type
//-----------------------------------------------------------------------------

std::string ts::DektecDevice::GetInterfaceDescription(const Dtapi::DtHwFuncDesc& port)
{
    std::string desc;

    OneCap(desc, (port.m_Flags & DTAPI_CAP_ASI) != 0, "ASI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SDI) != 0, "SDI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_MOD) != 0, "Modulator");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_IP)  != 0, "TS-over-IP");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SPI) != 0, "SPI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_SPISDI) != 0, "SPI_SDI");
    OneCap(desc, (port.m_Flags & DTAPI_CAP_VIRTUAL) != 0, "Virtual Stream");

    // If none found, use DTAPI function
    if (desc.empty()) {
        char str[64];
        Dtapi::DTAPI_RESULT status = Dtapi::DtapiDtHwFuncDesc2String(const_cast<Dtapi::DtHwFuncDesc*>(&port), DTAPI_HWF2STR_ITF_TYPE, str, sizeof(str));
        str[status == DTAPI_OK ? sizeof(str) - 1 : 0] = '\0';
        desc = str;
    }

    return desc;
}


//-----------------------------------------------------------------------------
// Get the list of all Dektec devices in the system.
//-----------------------------------------------------------------------------

bool ts::DektecDevice::GetAllDevices(DektecDeviceVector& devices, ReportInterface& report)
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
        report.error("error getting Dektec device list: " + DektecStrError(status));
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

bool ts::DektecDevice::getDevice(int& dev_index, int& chan_index, bool is_input, ReportInterface& report)
{
    const char* direction = is_input ? "input" : "output";

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
            report.error("no %s Dektec device found", direction);
            return false;
        }
    }
    else if (dev_index >= int(devlist.size())) {
        // Invalid device index specified
        report.error("invalid Dektec device index: %d", dev_index);
        return false;
    }

    // Found device
    *this = devlist[dev_index];

    // Check that device has required input or output capability
    if ((is_input && input.size() <= 0) || (!is_input && output.size() <= 0)) {
        report.error("Dektec device %d has no %s channel", dev_index, direction);
        return false;
    }

    // Check channel index
    if (chan_index < 0) {
        chan_index = 0;
    }
    if ((is_input && chan_index >= int(input.size())) || (!is_input && chan_index >= int(output.size()))) {
        report.error("Dektec device %d has no %s channel %d", dev_index, direction, chan_index);
        return false;
    }

    // Report selected device
    report.verbose("using Dektec device %d, %s channel %d (%s port %d)", dev_index, direction, chan_index, model.c_str(),
                   is_input ? input[chan_index].m_Port : output[chan_index].m_Port);
    return true;
}


//-----------------------------------------------------------------------------
// Display various Dektec data structure for debug
//-----------------------------------------------------------------------------

void ts::DektecDevice::Report(const Dtapi::DtDvbT2Pars& pars, ReportInterface& report, int severity, const std::string& margin)
{
    report.log(severity, "%sm_T2Version = %d", margin.c_str(), pars.m_T2Version);
    report.log(severity, "%sm_Bandwidth = %d", margin.c_str(), pars.m_Bandwidth);
    report.log(severity, "%sm_FftMode = %d", margin.c_str(), pars.m_FftMode);
    report.log(severity, "%sm_Miso = %d", margin.c_str(), pars.m_Miso);
    report.log(severity, "%sm_GuardInterval = %d", margin.c_str(), pars.m_GuardInterval);
    report.log(severity, "%sm_Papr = %d", margin.c_str(), pars.m_Papr);
    report.log(severity, "%sm_BwtExt = %d", margin.c_str(), int(pars.m_BwtExt));
    report.log(severity, "%sm_PilotPattern = %d", margin.c_str(), pars.m_PilotPattern);
    report.log(severity, "%sm_L1Modulation = %d", margin.c_str(), pars.m_L1Modulation);
    report.log(severity, "%sm_CellId = %d", margin.c_str(), pars.m_CellId);
    report.log(severity, "%sm_NetworkId = %d", margin.c_str(), pars.m_NetworkId);
    report.log(severity, "%sm_T2SystemId = %d", margin.c_str(), pars.m_T2SystemId);
    report.log(severity, "%sm_L1Repetition = %d", margin.c_str(), int(pars.m_L1Repetition));
    report.log(severity, "%sm_NumT2Frames = %d", margin.c_str(), pars.m_NumT2Frames);
    report.log(severity, "%sm_NumDataSyms = %d", margin.c_str(), pars.m_NumDataSyms);
    report.log(severity, "%sm_NumSubslices = %d", margin.c_str(), pars.m_NumSubslices);
    report.log(severity, "%sm_FefEnable = %d", margin.c_str(), int(pars.m_FefEnable));
    report.log(severity, "%sm_FefType = %d", margin.c_str(), pars.m_FefType);
    report.log(severity, "%sm_FefS1 = %d", margin.c_str(), pars.m_FefS1);
    report.log(severity, "%sm_FefS2 = %d", margin.c_str(), pars.m_FefS2);
    report.log(severity, "%sm_FefSignal = %d", margin.c_str(), pars.m_FefSignal);
    report.log(severity, "%sm_FefLength = %d", margin.c_str(), pars.m_FefLength);
    report.log(severity, "%sm_FefInterval = %d", margin.c_str(), pars.m_FefInterval);
    report.log(severity, "%sm_NumRfChans = %d", margin.c_str(), pars.m_NumRfChans);
    for (int i = 0; i < DTAPI_DVBT2_NUM_RF_MAX && i < pars.m_NumRfChans; i++) {
        report.log(severity, "%sm_RfChanFreqs[%d] = %d", margin.c_str(), i, pars.m_RfChanFreqs[i]);
    }
    report.log(severity, "%sm_StartRfIdx = %d", margin.c_str(), pars.m_StartRfIdx);
    report.log(severity, "%sm_NumPlps = %d", margin.c_str(), pars.m_NumPlps);
    for (int i = 0; i < DTAPI_DVBT2_NUM_PLP_MAX && i < pars.m_NumPlps; i++) {
        const std::string margin2(Format("%sm_Plps[%d].", margin.c_str(), i));
        Report(pars.m_Plps[i], report, severity, margin2.c_str());
    }
}

void ts::DektecDevice::Report(const Dtapi::DtDvbT2PlpPars& pars, ReportInterface& report, int severity, const std::string& margin)
{
    report.log(severity, "%sm_Hem = %d", margin.c_str(), pars.m_Hem);
    report.log(severity, "%sm_Npd = %d", margin.c_str(), pars.m_Npd);
    report.log(severity, "%sm_Issy = %d", margin.c_str(), pars.m_Issy);
    report.log(severity, "%sm_IssyBufs = %d", margin.c_str(), pars.m_IssyBufs);
    report.log(severity, "%sm_IssyTDesign = %d", margin.c_str(), pars.m_IssyTDesign);
    report.log(severity, "%sm_CompensatingDelay = %d", margin.c_str(), pars.m_CompensatingDelay);
    report.log(severity, "%sm_TsRate = %d", margin.c_str(), pars.m_TsRate);
    report.log(severity, "%sm_Id = %d", margin.c_str(), pars.m_Id);
    report.log(severity, "%sm_GroupId = %d", margin.c_str(), pars.m_GroupId);
    report.log(severity, "%sm_Type = %d", margin.c_str(), pars.m_Type);
    report.log(severity, "%sm_CodeRate = %d", margin.c_str(), pars.m_CodeRate);
    report.log(severity, "%sm_Modulation = %d", margin.c_str(), pars.m_Modulation);
    report.log(severity, "%sm_Rotation = %d", margin.c_str(), pars.m_Rotation);
    report.log(severity, "%sm_FecType = %d", margin.c_str(), pars.m_FecType);
    report.log(severity, "%sm_FrameInterval = %d", margin.c_str(), pars.m_FrameInterval);
    report.log(severity, "%sm_FirstFrameIdx = %d", margin.c_str(), pars.m_FirstFrameIdx);
    report.log(severity, "%sm_TimeIlLength = %d", margin.c_str(), pars.m_TimeIlLength);
    report.log(severity, "%sm_TimeIlType = %d", margin.c_str(), pars.m_TimeIlType);
    report.log(severity, "%sm_InBandAFlag = %d", margin.c_str(), int(pars.m_InBandAFlag));
    report.log(severity, "%sm_InBandBFlag = %d", margin.c_str(), int(pars.m_InBandBFlag));
    report.log(severity, "%sm_NumBlocks = %d", margin.c_str(), pars.m_NumBlocks);
    report.log(severity, "%sm_NumOtherPlpInBand = %d", margin.c_str(), pars.m_NumOtherPlpInBand);
    for (int i = 0; i < DTAPI_DVBT2_NUM_PLP_MAX - 1 && i < pars.m_NumOtherPlpInBand; i++) {
        report.log(severity, "%sm_OtherPlpInBand[%d] = %d", margin.c_str(), i, pars.m_OtherPlpInBand[i]);
    }
    report.log(severity, "%sm_FfFlag = %d", margin.c_str(), int(pars.m_FfFlag));
    report.log(severity, "%sm_FirstRfIdx = %d", margin.c_str(), pars.m_FirstRfIdx);
}

void ts::DektecDevice::Report(const Dtapi::DtDvbT2ParamInfo& pars, ReportInterface& report, int severity, const std::string& margin)
{
    report.log(severity, "%sm_TotalCellsPerFrame = %d", margin.c_str(), pars.m_TotalCellsPerFrame);
    report.log(severity, "%sm_L1CellsPerFrame = %d", margin.c_str(), pars.m_L1CellsPerFrame);
    report.log(severity, "%sm_DummyCellsPerFrame = %d", margin.c_str(), pars.m_DummyCellsPerFrame);
}

#endif // TS_NO_DTAPI
