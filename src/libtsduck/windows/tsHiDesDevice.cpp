//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  An encapsulation of a HiDes modulator device - Windows implementation.
//
//----------------------------------------------------------------------------

#include "tsHiDesDevice.h"
#include "tsDirectShowUtils.h"
#include "tsMemoryUtils.h"
#include <winioctl.h>
#include <ksproxy.h>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// KS property sets for it950x devices.
//----------------------------------------------------------------------------

// Main property set. Control device operation and send TS data blocks.
#define STATIC_KSPROPSETID_IT9500Properties 0xf23fac2d,0xe1af,0x48e0,{0x8b,0xbe,0xa1,0x40,0x29,0xc9,0x2f,0x11}

// Auxiliary property set. Query USB mode and device IDs.
// This value is actually KSPROPERTYSET_Wd3KsproxySample, an example GUID
// used by some vendors where engineers don't run guidgen.exe.
#define STATIC_KSPROPSETID_IT9500PropertiesAux 0xc6efe5eb,0x855a,0x4f1b,{0xb7,0xaa,0x87,0xb5,0xe1,0xdc,0x41,0x13}

// For get chip type.
#define REG_CHIP_VERSION 0x1222

namespace {

    // Properties
    enum {
        KSPROPERTY_IT95X_DRV_INFO = 0,  // in KSPROPSETID_IT9500Properties
        KSPROPERTY_IT95X_IOCTL    = 1,  // in KSPROPSETID_IT9500Properties
        KSPROPERTY_IT95X_BUS_INFO = 5,  // in KSPROPSETID_IT9500PropertiesAux
    };

    // KS property list indexes for DeviceIoControl
    enum {
        KSLIST_DRV_INFO_GET = 0,
        KSLIST_DRV_INFO_SET = 1,
        KSLIST_IOCTL_GET    = 2,
        KSLIST_IOCTL_SET    = 3,
        KSLIST_BUS_INFO_GET = 4,
        KSLIST_MAX          = 5,
    };

    // KS property list definitions for DeviceIoControl
    const ::KSPROPERTY kslist_template[KSLIST_MAX] = {
        {{{
            // KSLIST_DRV_INFO_GET
            {STATIC_KSPROPSETID_IT9500Properties},
            KSPROPERTY_IT95X_DRV_INFO,
            KSPROPERTY_TYPE_GET,
        }}},
        {{{
            // KSLIST_DRV_INFO_SET
            {STATIC_KSPROPSETID_IT9500Properties},
            KSPROPERTY_IT95X_DRV_INFO,
            KSPROPERTY_TYPE_SET,
        }}},
        {{{
            // KSLIST_IOCTL_GET
            {STATIC_KSPROPSETID_IT9500Properties},
            KSPROPERTY_IT95X_IOCTL,
            KSPROPERTY_TYPE_GET,
        }}},
        {{{
            // KSLIST_IOCTL_SET
            {STATIC_KSPROPSETID_IT9500Properties},
            KSPROPERTY_IT95X_IOCTL,
            KSPROPERTY_TYPE_SET,
        }}},
        {{{
            // KSLIST_BUS_INFO_GET
            {STATIC_KSPROPSETID_IT9500PropertiesAux},
            KSPROPERTY_IT95X_BUS_INFO,
            KSPROPERTY_TYPE_GET,
        }}},
    };

    // IOCTL codes for modulator
    enum {
        IOCTL_IT95X_GET_DRV_INFO = 1,
        IOCTL_IT95X_SET_POWER = 4,
        IOCTL_IT95X_SET_DVBT_MODULATION = 8,
        IOCTL_IT95X_SET_RF_OUTPUT = 9,
        IOCTL_IT95X_SEND_TS_DATA = 30,
        IOCTL_IT95X_SET_CHANNEL = 31,
        IOCTL_IT95X_SET_DEVICE_TYPE = 32,
        IOCTL_IT95X_GET_DEVICE_TYPE = 33,
        IOCTL_IT95X_SET_GAIN = 34,
        IOCTL_IT95X_RD_REG_OFDM = 35,
        IOCTL_IT95X_WR_REG_OFDM = 36,
        IOCTL_IT95X_RD_REG_LINK = 37,
        IOCTL_IT95X_WR_REG_LINK = 38,
        IOCTL_IT95X_SEND_PSI_ONCE = 39,
        IOCTL_IT95X_SET_PSI_PACKET = 40,
        IOCTL_IT95X_SET_PSI_TIMER = 41,
        IOCTL_IT95X_GET_GAIN_RANGE = 42,
        IOCTL_IT95X_SET_TPS = 43,
        IOCTL_IT95X_GET_TPS = 44,
        IOCTL_IT95X_GET_GAIN = 45,
        IOCTL_IT95X_SET_IQ_TABLE = 46,
        IOCTL_IT95X_SET_DC_CAL = 47,
        IOCTL_IT95X_SET_ISDBT_MODULATION = 60,
        IOCTL_IT95X_ADD_ISDBT_PID_FILTER = 61,
        IOCTL_IT95X_SET_TMCC = 62,
        IOCTL_IT95X_SET_TMCC2 = 63,
        IOCTL_IT95X_GET_TMCC = 64,
        IOCTL_IT95X_GET_TS_BITRATE = 65,
        IOCTL_IT95X_CONTROL_ISDBT_PID_FILTER = 66,
        IOCTL_IT95X_SET_PCR_MODE = 67,
        IOCTL_IT95X_SET_PCR_ENABLE = 68,
        IOCTL_IT95X_RESET_ISDBT_PID_FILTER = 69,
        IOCTL_IT95X_SET_OFS_CAL = 70,
        IOCTL_IT95X_ENABLE_TPS_CRYPT = 71,
        IOCTL_IT95X_DISABLE_TPS_CRYPT = 72,
    };

    enum {
        GAIN_POSITIVE = 1,
        GAIN_NEGATIVE = 2,
    };

    // Parameter structure for generic DeviceIoControl.
    struct IoctlGeneric
    {
        uint32_t code;
        uint32_t param1;
        uint32_t param2;

        // Constructor.
        IoctlGeneric(uint32_t c = 0, uint32_t p1 = 0, uint32_t p2 = 0) : code(c), param1(p1), param2(p2) {}
    };


}


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::HiDesDevice::Guts
{
public:
    ComPtr<::IBaseFilter> filter;             // Associated DirectShow filter.
    ::HANDLE              handle;             // Handle to it950x device.
    ::OVERLAPPED          overlapped;         // For overlapped operations.
    ::KSPROPERTY          kslist[KSLIST_MAX]; // Non-const version of KSLIST (required by DeviceIoControl).
    HiDesDeviceInfo       info;               // Portable device information.

    // Constructor, destructor.
    Guts();
    ~Guts();

    // Get or set a KS property.
    bool ksProperty(KSPROPERTY& prop, void *data, ::DWORD size, Report& report);

    // Get or set IOCTL's.
    bool ioctlGet(void *data, ::DWORD size, Report& report);
    bool ioctlSet(void *data, ::DWORD size, Report& report);

    // Get one or all devices.
    // If 'list' is non-zero, get all devices here.
    // If 'index' >= 0 or 'name' is not empty, only search this one and fully initialize the device.
    bool getDevices(HiDesDeviceInfoList* list, int index, const UString& name, Report& report);

    // Get information about one it950x device.
    bool getDeviceInfo(const ComPtr<::IMoniker>& moniker, Report& report);

    // Close the device.
    void close();

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
// Close a Guts internal object (close references to objects).
//----------------------------------------------------------------------------

void ts::HiDesDevice::Guts::close()
{
    // Release pointer to COM object.
    filter.release();

    // Close handle.
    // WARNING: It is unclear if this handle should be closed here or not.
    // The handle is returned by IKsObject::KsGetObjectHandle. There is no
    // evidence if this is a permanent handle which was returned (and we
    // should not close it) or if this handle was specially created for
    // us in KsGetObjectHandle (and we should close it).

    if (handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }

    // Close event handle used in overlapped operations.
    if (overlapped.hEvent != 0 && overlapped.hEvent != INVALID_HANDLE_VALUE) {
        ::CloseHandle(overlapped.hEvent);
        overlapped.hEvent = INVALID_HANDLE_VALUE;
    }
}


//----------------------------------------------------------------------------
// Get or set a KS property via device handle.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::ksProperty(KSPROPERTY& prop, void *data, ::DWORD size, Report& report)
{
    ::DWORD written = 0;
    ::BOOL ok = ::DeviceIoControl(handle, IOCTL_KS_PROPERTY, &prop, sizeof(prop), data, size, &written, &overlapped);

    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        ok = ::GetOverlappedResult(handle, &overlapped, &written, TRUE);
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
    return ksProperty(kslist[KSLIST_IOCTL_GET], data, size, report);
}

bool ts::HiDesDevice::Guts::ioctlSet(void *data, ::DWORD size, Report& report)
{
    return ksProperty(kslist[KSLIST_IOCTL_SET], data, size, report);
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
        if (fname.startWith(u"IT95") && CanonicalGUID(clsid) == itclsid) {
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
    return !searchOne || (found && infoOK);
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
    overlapped.hEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        report.error(u"CreateEvent error: %s", {WinErrorMessage(::GetLastError())});
        close();
        return false;
    }

    // After this point, we don't return on error, but we report the final status.
    bool status = true;

    // Check that all expected properties are supported by the device.
    for (size_t i = 0; i < KSLIST_MAX; i++) {

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
    if (!ksProperty(kslist[KSLIST_BUS_INFO_GET], &busInfo, sizeof(busInfo), report)) {
        status = false;
    }
    else {
        info.usb_mode = busInfo.usb_mode;
        info.vendor_id = busInfo.vendor_id ;
        info.product_id = busInfo.product_id;
    }

    // Get driver info. These information are different between Windows and Linux.
    IoctlGeneric ioc1(IOCTL_IT95X_GET_DRV_INFO);
    struct {
        uint32_t drv_pid;
        uint32_t drv_version;
        uint32_t fw_link;
        uint32_t fw_ofdm;
        uint32_t tuner_id;
    } driverInfo;
    TS_ZERO(driverInfo);

    report.log(2, u"HiDesDevice: getting driver information");
    if (!ksProperty(kslist[KSLIST_DRV_INFO_SET], &ioc1, sizeof(ioc1), report) ||
        !ksProperty(kslist[KSLIST_DRV_INFO_GET], &driverInfo, sizeof(driverInfo), report))
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
    IoctlGeneric ioc_lsb(IOCTL_IT95X_RD_REG_LINK, REG_CHIP_VERSION + 1);
    IoctlGeneric ioc_msb(IOCTL_IT95X_RD_REG_LINK, REG_CHIP_VERSION + 2);
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
    IoctlGeneric iocDeviceType(IOCTL_IT95X_GET_DEVICE_TYPE);
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


//----------------------------------------------------------------------------
// Tune the modulator with DVB-T modulation parameters.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::tune(const TunerParametersDVBT& params, Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    return false; //@@@@@@
}


//----------------------------------------------------------------------------
// Start transmission (after having set tuning parameters).
//----------------------------------------------------------------------------

bool ts::HiDesDevice::startTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    return false; //@@@@@@
}


//----------------------------------------------------------------------------
// Stop transmission.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::stopTransmission(Report& report)
{
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    return false; //@@@@@@
}


//----------------------------------------------------------------------------
// Send TS packets.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::send(const TSPacket* data, size_t packet_count, Report& report)
{
    // Check that we are ready to transmit.
    if (!_is_open) {
        report.error(u"HiDes device not open");
        return false;
    }

    return false; //@@@@@@
}
