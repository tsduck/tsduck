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

#include "tsDirectShowFilterCategory.h"


//-----------------------------------------------------------------------------
// Constructor.
//-----------------------------------------------------------------------------

ts::DirectShowFilterCategory::DirectShowFilterCategory(Report& report) :
    _report(report),
    _enum(),
    _moniker(),
    _filters()
{
}


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

ts::DirectShowFilterCategory::~DirectShowFilterCategory()
{
    // Enforce cleanup in the right order
    clear();
}

void ts::DirectShowFilterCategory::clear()
{
    _filters.clear();
    _moniker.release();
    _enum.release();
}


//-----------------------------------------------------------------------------
// Filter destructor.
//-----------------------------------------------------------------------------

ts::DirectShowFilterCategory::Filter::~Filter()
{
    // Enforce cleanup in the right order
    clear();
}

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
        if (_moniker->Next(1, flt.moniker.creator(), NULL) != S_OK) {
            break; // no more device
        }

        // Get friendly name of this filter.
        flt.name = GetStringPropertyBag(flt.moniker.pointer(), L"FriendlyName", _report);

        // Create an instance of this filter from moniker.
        flt.filter.bindToObject(flt.moniker.pointer(), ::IID_IBaseFilter, _report);
        if (!flt.filter.isNull()) {
            _filters.push_back(flt);
        }
    }
    return true;
}
