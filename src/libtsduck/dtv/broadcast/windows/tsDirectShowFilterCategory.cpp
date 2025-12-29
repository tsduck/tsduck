//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDirectShowFilterCategory.h"


//-----------------------------------------------------------------------------
// Constructor from a device category.
//-----------------------------------------------------------------------------

ts::DirectShowFilterCategory::DirectShowFilterCategory(const ::GUID& category, Report& report) :
    DirectShowFilterCategory(report)
{
    getAllFiltersInstance(category);
}


//-----------------------------------------------------------------------------
// Destructor.
//-----------------------------------------------------------------------------

void ts::DirectShowFilterCategory::clear()
{
    _filters.clear();
    _moniker.release();
    _enum.release();
}


//-----------------------------------------------------------------------------
// Filter destructor.
//-----------------------------------------------------------------------------

void ts::DirectShowFilterCategory::Filter::clear()
{
    filter.release();
    moniker.release();
    name.clear();
}


//-----------------------------------------------------------------------------
// Build an instance of all filters of the specified category.
//-----------------------------------------------------------------------------

bool ts::DirectShowFilterCategory::getAllFiltersInstance(const ::GUID& category)
{
    // Release all previous instances.
    clear();

    // Create a DirectShow System Device Enumerator
    _enum.createInstance(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, _report);
    if (_enum.isNull()) {
        return false;
    }

    // Enumerate all devices for this category
    ::HRESULT hr = _enum->CreateClassEnumerator(category, _moniker.creator(), 0);
    if (!ComSuccess(hr, u"CreateClassEnumerator", _report)) {
        _enum.release();
        return false;
    }
    if (hr != S_OK || _moniker.isNull()) {
        // Empty category, not an error.
        return true;
    }

    // Loop on all enumerated devices.
    for (;;) {
        Filter flt;

        // Get next filter device.
        if (_moniker->Next(1, flt.moniker.creator(), nullptr) != S_OK) {
            break; // no more device
        }

        // Get friendly name of this filter.
        flt.name = GetStringPropertyBag(flt.moniker.pointer(), L"FriendlyName", _report);

        // Create an instance of this filter from moniker.
        if (!flt.name.empty()) {
            flt.filter.bindToObject(flt.moniker.pointer(), ::IID_IBaseFilter, _report);
            if (!flt.filter.isNull()) {
                _filters.push_back(flt);
            }
        }
    }
    return true;
}
