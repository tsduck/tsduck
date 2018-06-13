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
}


//----------------------------------------------------------------------------
// Class internals, the "guts" internal class.
//----------------------------------------------------------------------------

class ts::HiDesDevice::Guts
{
public:
    ComPtr<::IBaseFilter> filter;             // Associated DirectShow filter.
    ::HANDLE              file;               // Handle to it950x device.
    ::OVERLAPPED          overlapped;         // For overlapped operations.
    ::KSPROPERTY          kslist[KSLIST_MAX]; // Non-const version of KSLIST (required by DeviceIoControl).
    Info                  info;               // Portable device information.

    // Constructor, destructor.
    Guts();
    ~Guts();

    // Get or set a KS property via device handle.
    ::HRESULT ksProperty(KSPROPERTY& prop, void *data, ::DWORD size);

    // Get one or all devices. Exactly one of 'list' or 'guts' must be non-zero.
    // If 'list' is non-zero, get all devices here.
    // If 'guts' is non-zero, get the device matching 'index' (if >= 0) or 'name'.
    static bool GetDevices(InfoList* list, Guts* guts, int index, const UString& name, Report& report);
};


//----------------------------------------------------------------------------
// Public class, constructor and destructor.
//----------------------------------------------------------------------------

ts::HiDesDevice::HiDesDevice() :
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
    file(INVALID_HANDLE_VALUE),
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
    if (file != INVALID_HANDLE_VALUE) {
        ::CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
}


//----------------------------------------------------------------------------
// Get or set a KS property via device handle.
//----------------------------------------------------------------------------

::HRESULT ts::HiDesDevice::Guts::ksProperty(KSPROPERTY& prop, void *data, ::DWORD size)
{

    ::DWORD written = 0;
    ::BOOL ok = ::DeviceIoControl(file, IOCTL_KS_PROPERTY, &prop, sizeof(prop), data, size, &written, &overlapped);

    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        ok = ::GetOverlappedResult(file, &overlapped, &written, TRUE);
    }

    return ok ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}


//----------------------------------------------------------------------------
// Get one or all devices.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::Guts::GetDevices(InfoList* list, Guts* guts, int index, const UString& name, Report& report)
{
    // Get monikers to all devices with categories of ITE devices.
    std::vector<ComPtr<::IMoniker>> monikers;
    if (!EnumerateDevicesByClass(KSCATEGORY_AUDIO_DEVICE, monikers, report, CDEF_DEVMON_PNP_DEVICE)) {
        return false;
    }

    // Get a canonical version of CLSID_Proxy, the expected class id of it950x devices.
    const UString itclsid(CanonicalGUID(CLSID_Proxy));
    report.debug(u"HiDes: CLSID_Proxy: %s", {itclsid});

    // Count devices to match index.
    int current_index = 0;
    bool found = false;

    // Loop on all monikers, check name and property.
    for (size_t i = 0; !found && i < monikers.size(); ++i) {

        // Get friendly name and class id of this filter.
        const UString fname(GetStringPropertyBag(monikers[i].pointer(), L"FriendlyName", report));
        const UString clsid(GetStringPropertyBag(monikers[i].pointer(), L"CLSID", report));
        report.debug(u"HiDes: checking \"%s\", CLSID %s", {fname, clsid});

        // Check if the name has the required prefix and class id for an it950x device.
        if (fname.startWith(u"IT95") && CanonicalGUID(clsid) == itclsid) {
            report.debug(u"HiDes: found device \"%s\"", {fname});

            // Start building a new device info block.
            Info info;
            info.index = current_index++;
            info.name = fname;
            info.path = GetStringPropertyBag(monikers[i].pointer(), L"DevicePath", report);

            // Create an instance of this filter from moniker.
            ComPtr<::IBaseFilter> filter;
            filter.bindToObject(monikers[i].pointer(), ::IID_IBaseFilter, report);
            if (filter.isNull()) {
                continue;
            }

            // Get the device handle to the filter.
            // WARNING: there is a risk here, we do strange things, see GetHandleFromObject in tsWinUtils.cpp.
            ::HANDLE handle = GetHandleFromObject(filter.pointer(), report);
            if (handle == INVALID_HANDLE_VALUE) {
                continue;
            }


            //@@@@@@@@@@@

            // Keep this device in the list, if we need a list.
            if (list != 0) {
                list->push_back(info);
            }
            else if (guts != 0) {
                found = (index >= 0 && index == info.index) || (!name.empty() && (name.similar(info.name) || name.similar(info.path)));
                if (found) {
                    guts->info = info;
                    guts->filter = filter;
                    guts->file = handle;
                    //@@@@@
                }
            }
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Get all HiDes devices in the system.
//----------------------------------------------------------------------------

bool ts::HiDesDevice::GetAllDevices(InfoList& devices, Report& report)
{
    devices.clear();
    return Guts::GetDevices(&devices, 0, -1, UString(), report);
}
