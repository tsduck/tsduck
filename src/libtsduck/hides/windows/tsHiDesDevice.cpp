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
//  An encapsulation of a HiDes modulator device - Windows implementation.
//
//----------------------------------------------------------------------------

#if defined(TS_NO_HIDES)
#include "tsHiDesDeviceStub.h"
#else

#include "tsIT950x.h"
#include "tsHiDesDevice.h"
#include "tsDirectShowUtils.h"
#include "tsMemory.h"
#include "tsNullReport.h"

#include "tsBeforeStandardHeaders.h"
#include <winioctl.h>
#include <ksproxy.h>
#include "tsAfterStandardHeaders.h"

namespace {
    // KS property list definitions for DeviceIoControl
    const ::KSPROPERTY kslist_template[ite::KSLIST_MAX] = {
        {{{
            // KSLIST_DRV_INFO_GET
            {ITE_STATIC_KSPROPSETID_IT9500Properties},
            ite::KSPROPERTY_IT95X_DRV_INFO,
            KSPROPERTY_TYPE_GET,
        }}},
        {{{
            // KSLIST_DRV_INFO_SET
            {ITE_STATIC_KSPROPSETID_IT9500Properties},
            ite::KSPROPERTY_IT95X_DRV_INFO,
            KSPROPERTY_TYPE_SET,
        }}},
        {{{
            // KSLIST_IOCTL_GET
            {ITE_STATIC_KSPROPSETID_IT9500Properties},
            ite::KSPROPERTY_IT95X_IOCTL,
            KSPROPERTY_TYPE_GET,
        }}},
        {{{
            // KSLIST_IOCTL_SET
            {ITE_STATIC_KSPROPSETID_IT9500Properties},
            ite::KSPROPERTY_IT95X_IOCTL,
            KSPROPERTY_TYPE_SET,
        }}},
        {{{
            // KSLIST_BUS_INFO_GET
            {ITE_STATIC_KSPROPSETID_IT9500PropertiesAux},
            ite::KSPROPERTY_IT95X_BUS_INFO,
            KSPROPERTY_TYPE_GET,
        }}},
    };
}


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::HiDesDevice::Guts
{
public:
    ComPtr<::IBaseFilter> filter;                  // Associated DirectShow filter.
    ::HANDLE              handle;                  // Handle to it950x device.
    ::OVERLAPPED          overlapped;              // For overlapped operations.
    ::KSPROPERTY          kslist[ite::KSLIST_MAX]; // Non-const version of KSLIST (required by DeviceIoControl).
    bool                  transmitting;            // Transmission in progress.
    HiDesDeviceInfo       info;                    // Portable device information.

    // Constructor, destructor.
    Guts();
    ~Guts();

    // Get or set a KS property.
    bool ksProperty(KSPROPERTY& prop, void *data, ::DWORD size, Report&);

    // Get or set IOCTL's.
    bool ioctlGet(void *data, ::DWORD size, Report&);
    bool ioctlSet(void *data, ::DWORD size, Report&);

    // Get one or all devices.
    // If 'list' is non-zero, get all devices here.
    // If 'index' >= 0 or 'name' is not empty, only search this one and fully initialize the device.
    bool getDevices(HiDesDeviceInfoList* list, int index, const UString& name, Report&);

    // Get information about one it950x device.
    bool getDeviceInfo(const ComPtr<::IMoniker>& moniker, Report&);

    // Redirected services for enclosing class.
    void close();
    bool setGetGain(uint32_t code, int& gain, Report&);
    bool setTransmission(bool enable, Report&);
    bool setPower(bool enable, Report&);


    // Format a 32-bit firmware version as a string.
    static UString FormatVersion(uint32_t v);
};


//----------------------------------------------------------------------------
// Public class, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::HiDesDevice() :
    _is_open(false),
    _guts(new Guts)
{
}

ts::HiDesDevice::~HiDesDevice()
{
    // Free internal resources.
    if (_guts != 0) {
        delete _guts;
        _guts = 0;
    }
}


//----------------------------------------------------------------------------
// Guts, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::Guts::Guts() :
    filter(),
    handle(INVALID_HANDLE_VALUE),
    overlapped(),
    kslist(),
    transmitting(false),
    info()
{
    TS_ZERO(overlapped);
    assert(sizeof(kslist) == sizeof(kslist_template));
    ::memcpy(kslist, kslist_template, sizeof(kslist));
}

ts::HiDesDevice::Guts::~Guts()
{
    close();
}


//----------------------------------------------------------------------------
// Get or set a KS property via device handle.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::ksProperty(KSPROPERTY& prop, void *data, ::DWORD size, Report& report)
{
    ::DWORD written = 0;
    ::BOOL ok = ::DeviceIoControl(handle, IOCTL_KS_PROPERTY, &prop, sizeof(prop), data, size, &written, &overlapped);

    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        ok = ::GetOverlappedResult(handle, &overlapped, &written, true);
    }

    if (!ok) {
        report.error(u"IOCTL_KS_PROPERTY error: %s", {WinErrorMessage(::GetLastError())});
    }

    return ok;
}

//----------------------------------------------------------------------------
// Get or set IOCTL's.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::ioctlGet(void *data, ::DWORD size, Report& report)
{
    return ksProperty(kslist[ite::KSLIST_IOCTL_GET], data, size, report);
}

bool ts::HiDesDevice::Guts::ioctlSet(void *data, ::DWORD size, Report& report)
{
    return ksProperty(kslist[ite::KSLIST_IOCTL_SET], data, size, report);
}


//----------------------------------------------------------------------------
// Get one or all devices.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::getDevices(HiDesDeviceInfoList* list, int index, const UString& name, Report& report)
{
    // Check if we are looking for one specific or all devices.
    const bool searchOne = index >= 0 || !name.empty();

    // There must be exactly one operation: search one (and open it) or get all (and open none).
    assert(searchOne || list != 0);
    assert(!searchOne || list == 0);

    // Get monikers to all devices with categories of ITE devices.
    // For some reason, the category is "audio device".
    std::vector<ComPtr<::IMoniker>> monikers;
    if (!EnumerateDevicesByClass(KSCATEGORY_AUDIO_DEVICE, monikers, report, CDEF_DEVMON_PNP_DEVICE)) {
        return false;
    }

    // Get a canonical version of CLSID_Proxy, the expected class id of it950x devices.
    const UString itclsid(CanonicalGUID(CLSID_Proxy));
    report.debug(u"HiDes: CLSID_Proxy: %s", {itclsid});

    // Count devices to match index.
    int deviceIndex = 0;
    bool found = false;
    bool infoOK = true;

    // Loop on all monikers, check name and property.
    for (size_t i = 0; !found && i < monikers.size(); ++i) {

        // Get friendly name and class id of this filter.
        const UString fname(GetStringPropertyBag(monikers[i].pointer(), L"FriendlyName", report));
        const UString clsid(GetStringPropertyBag(monikers[i].pointer(), L"CLSID", report));
        report.debug(u"HiDes: checking \"%s\", CLSID %s", {fname, clsid});

        // Check if the name has the required prefix and class id for an it950x device.
        // Filter out names containing " RX " in case this means a receiver (not verified yet).
        if (fname.startWith(u"IT95") && !fname.contain(u" RX ") && CanonicalGUID(clsid) == itclsid) {
            report.debug(u"HiDes: found device \"%s\"", {fname});

            // We must increment deviceIndex now because this is an index of all it950x devices.
            // If there is an error later, this means that we may have no right to access this device.
            // But the device still exists.
            const int currentIndex = deviceIndex++;

            // Get the device path.
            const UString path(GetStringPropertyBag(monikers[i].pointer(), L"DevicePath", report));

            // If we are looking for one specific device, check now, before fetching additional info.
            if (searchOne) {
                found = (index >= 0 && index == currentIndex) || (!name.empty() && (name.similar(fname) || name.similar(path)));
                if (!found) {
                    // Not the one we are looking for, skip it without fetching its properties.
                    continue;
                }
            }

            // We need to continue on this device, initialize its info block.
            info.clear();
            info.index = currentIndex;
            info.name = fname;
            info.path = path;

            // Fetch additional information on the device.
            infoOK = getDeviceInfo(monikers[i], report) && infoOK;

            // Keep this device in the list, if we need a list.
            if (list != 0) {
                list->push_back(info);
                // And we also don't keep them open.
                close();
            }
        }
    }

    // Error when:
    // - Looking for one specific device and did not find it.
    // - Looking for one specific device, found it but could not fetch its properties.
    // There is no error at this point if we just wanted to get the list of devices.
    if (!searchOne || (found && infoOK)) {
        return true;
    }
    else if (index >= 0) {
        report.error(u"device index %d not found", {index});
        return false;
    }
    else {
        report.error(u"device %s not found", {name});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get information about one it950x device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::getDeviceInfo(const ComPtr<::IMoniker>& moniker, Report& report)
{
    // This method dives into DirectShow.
    // Unclear what happens when compiled for 32-bit and running on 64-bit system.
    // Use --debug=2 to activate these traces.
    report.log(2, u"HiDesDevice: getting device information");

    // Create an instance of this filter from moniker.
    report.log(2, u"HiDesDevice: get filter instance");
    filter.bindToObject(moniker.pointer(), ::IID_IBaseFilter, report);
    if (filter.isNull()) {
        return false;
    }

    // Get the device handle to the filter.
    // WARNING: in case of problem here, see GetHandleFromObject in tsWinUtils.cpp.
    report.log(2, u"HiDesDevice: calling GetHandleFromObject");
    handle = GetHandleFromObject(filter.pointer(), report);
    if (handle == INVALID_HANDLE_VALUE) {
        close();
        return false;
    }
    report.log(2, u"HiDesDevice: GetHandleFromObject successful");

    // Create an event for overlapped operations.
    report.log(2, u"HiDesDevice: creating event for overlapped");
    overlapped.hEvent = ::CreateEventW(NULL, true, false, NULL);
    if (overlapped.hEvent == NULL) {
        report.error(u"CreateEvent error: %s", {WinErrorMessage(::GetLastError())});
        close();
        return false;
    }

    // After this point, we don't return on error, but we report the final status.
    bool status = true;

    // Check that all expected properties are supported by the device.
    for (size_t i = 0; i < ite::KSLIST_MAX; i++) {

        ::KSPROPERTY prop = kslist[i];
        const ::ULONG flags = prop.Flags;
        prop.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
        ::DWORD want = 0;

        report.log(2, u"HiDesDevice: checking support for property %d, index %d", {prop.Id, i});

        // Check that basic support is provided.
        ::DWORD got = 0;
        const bool ok = ksProperty(prop, &got, sizeof(got), report);

        // Check that the requested operation (get or set) is supported.
        if (ok) {
            if (flags == KSPROPERTY_TYPE_GET) {
                want = KSPROPERTY_SUPPORT_GET;
            }
            else if (flags == KSPROPERTY_TYPE_SET) {
                want = KSPROPERTY_SUPPORT_SET;
            }
        }
        if (!ok || (got & want) == 0) {
            report.error(u"Property %d not fully supported on %s (%s)", {prop.Id, info.name, info.path});
            status = false;
        }
    }

    // Get USB mode and vendor info.
    struct {
        uint16_t usb_mode;
        uint16_t vendor_id;
        uint16_t product_id;
    } busInfo;
    TS_ZERO(busInfo);

    report.log(2, u"HiDesDevice: getting USB mode");
    if (!ksProperty(kslist[ite::KSLIST_BUS_INFO_GET], &busInfo, sizeof(busInfo), report)) {
        status = false;
    }
    else {
        info.usb_mode = busInfo.usb_mode;
        info.vendor_id = busInfo.vendor_id ;
        info.product_id = busInfo.product_id;
    }

    // Get driver info. These information are different between Windows and Linux.
    ite::IoctlGeneric ioc1(ite::IOCTL_IT95X_GET_DRV_INFO);
    struct {
        uint32_t drv_pid;
        uint32_t drv_version;
        uint32_t fw_link;
        uint32_t fw_ofdm;
        uint32_t tuner_id;
    } driverInfo;
    TS_ZERO(driverInfo);

    report.log(2, u"HiDesDevice: getting driver information");
    if (!ksProperty(kslist[ite::KSLIST_DRV_INFO_SET], &ioc1, sizeof(ioc1), report) ||
        !ksProperty(kslist[ite::KSLIST_DRV_INFO_GET], &driverInfo, sizeof(driverInfo), report))
    {
        status = false;
    }
    else {
        info.driver_version = FormatVersion(driverInfo.drv_version);
        info.link_fw_version = FormatVersion(driverInfo.fw_link);
        info.ofdm_fw_version = FormatVersion(driverInfo.fw_ofdm);
    }

    // Get chip type.
    uint32_t lsb = 0;
    uint32_t msb = 0;
    ite::IoctlGeneric ioc_lsb(ite::IOCTL_IT95X_RD_REG_LINK, IT95X_REG_CHIP_VERSION + 1);
    ite::IoctlGeneric ioc_msb(ite::IOCTL_IT95X_RD_REG_LINK, IT95X_REG_CHIP_VERSION + 2);
    report.log(2, u"HiDesDevice: getting chip type");
    if (!ioctlSet(&ioc_lsb, sizeof(ioc_lsb), report) ||
        !ioctlGet(&lsb, sizeof(lsb), report) ||
        !ioctlSet(&ioc_msb, sizeof(ioc_msb), report) ||
        !ioctlGet(&msb, sizeof(msb), report))
    {
        status = false;
    }
    else {
        info.chip_type = uint16_t(((msb & 0xFF) << 8) | (lsb & 0xFF));
    }

    // Get device type.
    ite::IoctlGeneric iocDeviceType(ite::IOCTL_IT95X_GET_DEVICE_TYPE);
    report.log(2, u"HiDesDevice: getting device type");
    if (!ioctlSet(&iocDeviceType, sizeof(iocDeviceType), report) ||
        !ioctlGet(&iocDeviceType, sizeof(iocDeviceType), report))
    {
        status = false;
    }
    else {
        info.device_type = int(iocDeviceType.param2);
    }

    // Free resources on error.
    if (!status) {
        close();
    }
    return status;
}


//----------------------------------------------------------------------------
// Format a 32-bit firmware version as a string.
//----------------------------------------------------------------------------

ts::UString ts::HiDesDevice::Guts::FormatVersion(uint32_t v)
{
    return v == 0 ? UString() : UString::Format(u"%d.%d.%d.%d", {(v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF});
}


//----------------------------------------------------------------------------
// Get all HiDes devices in the system.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::GetAllDevices(HiDesDeviceInfoList& devices, Report& report)
{
    // Clear previous content.
    devices.clear();

    // Use a dummy Guts object to get the list of devices.
    Guts guts;
    return guts.getDevices(&devices, -1, UString(), report);
}


//----------------------------------------------------------------------------
// Open the HiDes device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::open(int index, Report& report)
{
    // Error if already open.
    if (_is_open) {
        report.error(u"%s already open", {_guts->info.path});
        return false;
    }

    // Perform opening. No name is provided.
    _is_open = _guts->getDevices(0, index, UString(), report);
    return _is_open;
}

bool ts::HiDesDevice::open(const UString& name, Report& report)
{
    // Error if already open.
    if (_is_open) {
        report.error(u"%s already open", {_guts->info.path});
        return false;
    }

    // Perform opening. No index provided.
    _is_open = _guts->getDevices(0, -1, name, report);
    return _is_open;
}


//----------------------------------------------------------------------------
// Get information about the device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::getInfo(HiDesDeviceInfo& info, Report& report) const
{
    if (_is_open) {
        info = _guts->info;
        return true;
    }
    else {
        report.error(u"HiDes device not open");
        return false;
    }
}


//----------------------------------------------------------------------------
// Close the device.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::close(Report& report)
{
    // Silently ignore "already closed".
    _guts->close();
    _is_open = false;
    return true;
}

void ts::HiDesDevice::Guts::close()
{
    // Stop transmission, if currently in progress, and power off.
    setTransmission(false, NULLREP);
    setPower(false, NULLREP);

    // Release pointer to COM object.
    filter.release();

    // Close handle ?
    // WARNING: It is unclear if this handle should be closed here or not.
    //
    // The handle is returned by IKsObject::KsGetObjectHandle. There is no
    // evidence if this is a permanent handle which was returned (and we
    // should not close it) or if this handle was specially created for
    // us in KsGetObjectHandle (and we should close it).
    //
    // When executing under control of the debugger, CloseHandle throws an
    // "invalid handle" exception. It is probable that this handle is not
    // recognized as the kind of handle which is open by the system.
    //
    // if (handle != 0 && handle != INVALID_HANDLE_VALUE) {
    //     ::CloseHandle(handle);
    // }
    //
    handle = INVALID_HANDLE_VALUE;

    // Close event handle used in overlapped operations.
    if (overlapped.hEvent != 0 && overlapped.hEvent != INVALID_HANDLE_VALUE) {
        ::CloseHandle(overlapped.hEvent);
        overlapped.hEvent = INVALID_HANDLE_VALUE;
    }
}


//----------------------------------------------------------------------------
// Enable or disable power and transmission.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::setTransmission(bool enable, Report& report)
{
    ite::IoctlGeneric ioc(ite::IOCTL_IT95X_SET_RF_OUTPUT, enable);
    if (!ioctlSet(&ioc, sizeof(ioc), report)) {
        report.error(u"error setting transmission %s", {UString::OnOff(enable)});
        return false;
    }
    else {
        transmitting = enable;
        return true;
    }
}

bool ts::HiDesDevice::Guts::setPower(bool enable, Report& report)
{
    ite::IoctlGeneric ioc(ite::IOCTL_IT95X_SET_POWER, enable);
    if (!ioctlSet(&ioc, sizeof(ioc), report)) {
        report.error(u"error setting power %s", {UString::OnOff(enable)});
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Set or get the output gain in dB.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::setGain(int& gain, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->setGetGain(ite::IOCTL_IT95X_SET_GAIN, gain, report);
    }
}

bool ts::HiDesDevice::getGain(int& gain, Report& report)
{
    gain = 0;
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->setGetGain(ite::IOCTL_IT95X_GET_GAIN, gain, report);
    }
}

bool ts::HiDesDevice::Guts::setGetGain(uint32_t code, int& gain, Report& report)
{
    ite::IoctlGeneric ioc(code, std::abs(gain), gain < 0 ? ite::GAIN_NEGATIVE : ite::GAIN_POSITIVE);
    if (!ioctlSet(&ioc, sizeof(ioc), report) || !ioctlGet(&ioc, sizeof(ioc), report)) {
        report.error(u"error accessing output gain");
        return false;
    }
    else {
        switch (ioc.param2) {
            case ite::GAIN_POSITIVE:
                gain = int(ioc.param1);
                break;
            case ite::GAIN_NEGATIVE:
                gain = - int(ioc.param1);
                break;
            default:
                report.error(u"error setting output gain, invalid returned sign value: %d", {ioc.param2});
                return false;
        }
        return true;
    }

}


//----------------------------------------------------------------------------
// Get the allowed range of output gain in dB.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::getGainRange(int& minGain, int& maxGain, uint64_t frequency, BandWidth bandwidth, Report& report)
{
    minGain = maxGain = 0;

    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    // Frequency and bandwidth are in kHz
    ite::IoctlGainRange ioc(ite::IOCTL_IT95X_GET_GAIN_RANGE);
    ioc.frequency = uint32_t(frequency / 1000);
    ioc.bandwidth = bandwidth / 1000;

    if (ioc.bandwidth == 0) {
        report.error(u"unsupported bandwidth");
        return false;
    }
    else if (!_guts->ioctlSet(&ioc, sizeof(ioc), report) || !_guts->ioctlGet(&ioc, sizeof(ioc), report)) {
        report.error(u"error getting output gain range");
        return false;
    }
    else {
        maxGain = ioc.max_gain;
        minGain = ioc.min_gain;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set DC calibration values.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::setDCCalibration(int dcI, int dcQ, ts::Report &report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    ite::IoctlDCCalibration ioc(ite::IOCTL_IT95X_SET_DC_CAL);
    ioc.dc_i = int32_t(dcI);
    ioc.dc_q = int32_t(dcQ);

    if (!_guts->ioctlSet(&ioc, sizeof(ioc), report)) {
        report.error(u"error setting DC calibration");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Tune the modulator with DVB-T modulation parameters.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::tune(const ModulationArgs& in_params, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    // Get tuning parameters with default values.
    ModulationArgs params(in_params);
    params.delivery_system.setDefault(DS_DVB_T);
    params.setDefaultValues();
    if (params.delivery_system != DS_DVB_T) {
        report.error(u"invalid tuning parameters for HiDes device, not DVB-T parameters");
    }

    // Stop transmission while tuning.
    if (!_guts->setTransmission(false, report)) {
        return false;
    }

    // Build frequency + bandwidth parameters.
    // Frequency and bandwidth are in kHz
    ite::IoctlGeneric freqRequest(ite::IOCTL_IT95X_SET_CHANNEL);
    freqRequest.param1 = uint32_t(params.frequency.value() / 1000);
    freqRequest.param2 = params.bandwidth.value() / 1000;

    if (freqRequest.param2 == 0) {
        report.error(u"unsupported bandwidth");
        return false;
    }

    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    // Build modulation parameters.
    // Translate TSDuck enums into HiDes codes.
    ite::IoctlDVBT modRequest(ite::IOCTL_IT95X_SET_DVBT_MODULATION);

    switch (params.modulation.value()) {
        case QPSK:
            modRequest.constellation = uint8_t(ite::IT95X_CONSTELLATION_QPSK);
            break;
        case QAM_16:
            modRequest.constellation = uint8_t(ite::IT95X_CONSTELLATION_16QAM);
            break;
        case QAM_64:
            modRequest.constellation = uint8_t(ite::IT95X_CONSTELLATION_64QAM);
            break;
        default:
            report.error(u"unsupported constellation");
            return false;
    }

    switch (params.fec_hp.value()) {
        case FEC_1_2:
            modRequest.code_rate = uint8_t(ite::IT95X_CODERATE_1_2);
            break;
        case FEC_2_3:
            modRequest.code_rate = uint8_t(ite::IT95X_CODERATE_2_3);
            break;
        case FEC_3_4:
            modRequest.code_rate = uint8_t(ite::IT95X_CODERATE_3_4);
            break;
        case FEC_5_6:
            modRequest.code_rate = uint8_t(ite::IT95X_CODERATE_5_6);
            break;
        case FEC_7_8:
            modRequest.code_rate = uint8_t(ite::IT95X_CODERATE_7_8);
            break;
        default:
            report.error(u"unsupported high priority code rate");
            return false;
    }

    switch (params.guard_interval.value()) {
        case GUARD_1_32:
            modRequest.guard_interval = uint8_t(ite::IT95X_GUARD_1_32);
            break;
        case GUARD_1_16:
            modRequest.guard_interval = uint8_t(ite::IT95X_GUARD_1_16);
            break;
        case GUARD_1_8:
            modRequest.guard_interval = uint8_t(ite::IT95X_GUARD_1_8);
            break;
        case GUARD_1_4:
            modRequest.guard_interval = uint8_t(ite::IT95X_GUARD_1_4);
            break;
        default:
            report.error(u"unsupported guard guard_interval");
            return false;
    }

    switch (params.transmission_mode.value()) {
        case TM_2K:
            modRequest.tx_mode = uint8_t(ite::IT95X_TX_MODE_2K);
            break;
        case TM_4K:
            modRequest.tx_mode = uint8_t(ite::IT95X_TX_MODE_4K);
            break;
        case TM_8K:
            modRequest.tx_mode = uint8_t(ite::IT95X_TX_MODE_8K);
            break;
        default:
            report.error(u"unsupported transmission mode");
            return false;
    }

    TS_POP_WARNING()

    // Don't know how to set spectral inversion on Windows.

    // Now all parameters are validated, call the driver.
    if (!_guts->ioctlSet(&freqRequest, sizeof(freqRequest), report)) {
        report.error(u"error setting frequency & bandwidth");
        return false;
    }
    else if (!_guts->ioctlSet(&modRequest, sizeof(modRequest), report)) {
        report.error(u"error setting modulation parameters");
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Start / stop transmission.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::startTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->setTransmission(true, report);
    }
}

bool ts::HiDesDevice::stopTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }
    else {
        return _guts->setTransmission(false, report);
    }
}


//----------------------------------------------------------------------------
// Send TS packets.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::send(const TSPacket* packets, size_t packet_count, Report& report, AbortInterface* abort)
{
    // Check that we are ready to transmit.
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    // Prepare a data block for transmission. We cannot "write" to the device.
    // We must send an ioctl with a data block containing the TS packets.
    ite::IoctlTransmission ioc(ite::IOCTL_IT95X_SEND_TS_DATA);

    // Send packets by chunks of 348 (IT95X_TX_BLOCK_PKTS).
    while (packet_count > 0) {

        // Abort on user's request.
        if (abort != 0 && abort->aborting()) {
            report.debug(u"HiDesDevice: user requested abort");
            return false;
        }

        // Copy a chunk of packets in the transmission control block.
        const size_t count = std::min<size_t>(packet_count, IT95X_TX_BLOCK_PKTS);
        ioc.size = uint32_t(count * PKT_SIZE);
        ::memcpy(ioc.data, packets, ioc.size);

        report.log(2, u"HiDesDevice: calling IOCTL_IT95X_SEND_TS_DATA, size = %d, packets: %d", {ioc.size, count});

        // Send packets.
        if (!_guts->ioctlSet(&ioc, sizeof(ioc), report)) {
            report.error(u"error sending data");
            return false;
        }

        report.log(2, u"HiDesDevice: after IOCTL_IT95X_SEND_TS_DATA, size = %d", {ioc.size});

        packets += count;
        packet_count -= count;
    }

    return true;
}

#endif // TS_NO_HIDES
