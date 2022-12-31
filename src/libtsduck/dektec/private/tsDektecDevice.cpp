//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#include "tsDektecDevice.h"
#include "tsDektecUtils.h"
#include "tsNamesFile.h"

#if defined(TS_NO_DTAPI)
TS_LLVM_NOWARNING(missing-variable-declarations)
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
    for (auto it = ports.begin(); it != ports.end(); ) {
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
        devices[dev].model = GetDeviceDescription(devices[dev].desc);

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
    // As of DTAPI 5.35, there are no more than 256 capabilities.

    UString caps;
    for (int c = 0; c < 256; c++) {
        if ((flags & Dtapi::DtCaps(c)) != 0) {
            if (!caps.empty()) {
                caps += u", ";
            }
            caps += NamesFile::Instance(NamesFile::Predefined::DEKTEC)->nameFromSection(u"DtCaps", c, NamesFlags::NAME_OR_VALUE | NamesFlags::DECIMAL);
        }
    }
    return caps;
}


//-----------------------------------------------------------------------------
// Display various Dektec data structure for debug
//-----------------------------------------------------------------------------

void ts::DektecDevice::ReportDvbT2Pars(const Dtapi::DtDvbT2Pars& pars, Report& report, int severity, const UString& margin)
{
    // Don't lose time on multiple reports which won't do anything.
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%sm_T2Version = %d", {margin, pars.m_T2Version});
        report.log(severity, u"%sm_T2Profile = %d", {margin, pars.m_T2Profile});
        report.log(severity, u"%sm_T2BaseLite = %d", {margin, pars.m_T2BaseLite});
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
        report.log(severity, u"%sm_ComponentStartTime = %d", {margin, pars.m_ComponentStartTime});
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
}

void ts::DektecDevice::ReportDvbT2PlpPars(const Dtapi::DtDvbT2PlpPars& pars, Report& report, int severity, const UString& margin)
{
    // Don't lose time on multiple reports which won't do anything.
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%sm_Hem = %d", {margin, pars.m_Hem});
        report.log(severity, u"%sm_Npd = %d", {margin, pars.m_Npd});
        report.log(severity, u"%sm_Issy = %d", {margin, pars.m_Issy});
        report.log(severity, u"%sm_IssyBufs = %d", {margin, pars.m_IssyBufs});
        report.log(severity, u"%sm_IssyTDesign = %d", {margin, pars.m_IssyTDesign});
        report.log(severity, u"%sm_CompensatingDelay = %d", {margin, pars.m_CompensatingDelay});
        report.log(severity, u"%sm_TsRate = %d", {margin, pars.m_TsRate});
        report.log(severity, u"%sm_GseLabelType = %d", {margin, pars.m_GseLabelType});
        report.log(severity, u"%sm_Id = %d", {margin, pars.m_Id});
        report.log(severity, u"%sm_GroupId = %d", {margin, pars.m_GroupId});
        report.log(severity, u"%sm_Type = %d", {margin, pars.m_Type});
        report.log(severity, u"%sm_PayloadType = %d", {margin, pars.m_PayloadType});
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
        report.log(severity, u"%sm_PlpMute = %d", {margin, pars.m_PlpMute});
        report.log(severity, u"%sm_NumOtherPlpInBand = %d", {margin, pars.m_NumOtherPlpInBand});
        for (int i = 0; i < DTAPI_DVBT2_NUM_PLP_MAX - 1 && i < pars.m_NumOtherPlpInBand; i++) {
            report.log(severity, u"%sm_OtherPlpInBand[%d] = %d", {margin, i, pars.m_OtherPlpInBand[i]});
        }
        report.log(severity, u"%sm_FfFlag = %d", {margin, pars.m_FfFlag});
        report.log(severity, u"%sm_FirstRfIdx = %d", {margin, pars.m_FirstRfIdx});
    }
}

void ts::DektecDevice::ReportDvbT2ParamInfo(const Dtapi::DtDvbT2ParamInfo& pars, Report& report, int severity, const UString& margin)
{
    // Don't lose time on multiple reports which won't do anything.
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%sm_TotalCellsPerFrame = %d", {margin, pars.m_TotalCellsPerFrame});
        report.log(severity, u"%sm_L1CellsPerFrame = %d", {margin, pars.m_L1CellsPerFrame});
        report.log(severity, u"%sm_DummyCellsPerFrame = %d", {margin, pars.m_DummyCellsPerFrame});
    }
}

void ts::DektecDevice::ReportIpPars(const Dtapi::DtIpPars2& pars, Report& report, int severity, const UString& margin)
{
    // Don't lose time on multiple reports which won't do anything.
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%sm_Ip = %s", {margin, UString::Dump(pars.m_Ip, sizeof(pars.m_Ip), UString::SINGLE_LINE)});
        report.log(severity, u"%sm_Port = %d", {margin, pars.m_Port});
        report.log(severity, u"%sm_Gateway = %s", {margin, UString::Dump(pars.m_Gateway, sizeof(pars.m_Gateway), UString::SINGLE_LINE)});
        for (size_t i = 0; i < pars.m_SrcFlt.size(); ++i) {
            report.log(severity, u"%sm_SrcFlt[%d].m_SrcFltIp = %s", {margin, i, UString::Dump(pars.m_SrcFlt[i].m_SrcFltIp, sizeof(pars.m_SrcFlt[i].m_SrcFltIp), UString::SINGLE_LINE)});

        }
        report.log(severity, u"%sm_VlanId = %d", {margin, pars.m_VlanId});
        report.log(severity, u"%sm_VlanPriority = %d", {margin, pars.m_VlanPriority});
        report.log(severity, u"%sm_Ip2 = %s", {margin, UString::Dump(pars.m_Ip2, sizeof(pars.m_Ip2), UString::SINGLE_LINE)});
        report.log(severity, u"%sm_Port2 = %d", {margin, pars.m_Port2});
        report.log(severity, u"%sm_Gateway2 = %s", {margin, UString::Dump(pars.m_Gateway2, sizeof(pars.m_Gateway2), UString::SINGLE_LINE)});
        for (size_t i = 0; i < pars.m_SrcFlt2.size(); ++i) {
            report.log(severity, u"%sm_SrcFlt2[%d].m_SrcFltIp = %s", {margin, i, UString::Dump(pars.m_SrcFlt2[i].m_SrcFltIp, sizeof(pars.m_SrcFlt2[i].m_SrcFltIp), UString::SINGLE_LINE)});

        }
        report.log(severity, u"%sm_VlanId2 = %d", {margin, pars.m_VlanId2});
        report.log(severity, u"%sm_VlanPriority2 = %d", {margin, pars.m_VlanPriority2});
        report.log(severity, u"%sm_TimeToLive = %d", {margin, pars.m_TimeToLive});
        report.log(severity, u"%sm_NumTpPerIp = %d", {margin, pars.m_NumTpPerIp});
        report.log(severity, u"%sm_Protocol = %d", {margin, pars.m_Protocol});
        report.log(severity, u"%sm_DiffServ = %d", {margin, pars.m_DiffServ});
        report.log(severity, u"%sm_FecMode = %d", {margin, pars.m_FecMode});
        report.log(severity, u"%sm_FecNumRows = %d", {margin, pars.m_FecNumRows});
        report.log(severity, u"%sm_FecNumCols = %d", {margin, pars.m_FecNumCols});
        report.log(severity, u"%sm_Flags = 0x%X", {margin, pars.m_Flags});
        report.log(severity, u"%sm_Mode = %d", {margin, pars.m_Mode});
        report.log(severity, u"%sm_IpProfile.m_Profile = %d", {margin, pars.m_IpProfile.m_Profile});
        report.log(severity, u"%sm_IpProfile.m_MaxBitrate = %d", {margin, pars.m_IpProfile.m_MaxBitrate});
        report.log(severity, u"%sm_IpProfile.m_MaxSkew = %d", {margin, pars.m_IpProfile.m_MaxSkew});
        report.log(severity, u"%sm_IpProfile.m_VideoStandard = %d", {margin, pars.m_IpProfile.m_VideoStandard});
    }
}

#endif // TS_NO_DTAPI
