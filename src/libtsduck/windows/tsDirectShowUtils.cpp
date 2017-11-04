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
//
//  DirectShow & BDA utilities, Windows-specific.
//
//-----------------------------------------------------------------------------

#include "tsDirectShowUtils.h"
#include "tsMediaTypeUtils.h"
#include "tsComUtils.h"
#include "tsMemoryUtils.h"
#include "tsNullReport.h"
#include "tsFormat.h"
#include "tsDecimal.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Get list of pins on a filter (use flags from enum above)
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::GetPin(PinPtrVector& result, ::IBaseFilter* filter, int flags, ReportInterface& report)
{
    // Clear result vector (explicitely nullify previous values to release objects)
    result.clear();

    // If neither input nor output, neither connected nor unconnected, nothing to search.
    if ((flags & (xPIN_INPUT | xPIN_OUTPUT)) == 0 || (flags & (xPIN_CONNECTED | xPIN_UNCONNECTED)) == 0) {
        return true;
    }

    // Create a pin enumerator
    ComPtr <::IEnumPins> enum_pins;
    ::HRESULT hr = filter->EnumPins(enum_pins.creator());
    if (!ComSuccess(hr, "IBaseFilter::EnumPins", report)) {
        return false;
    }

    // Loop on all pins
    ComPtr <::IPin> pin;
    while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {
        // Query direction of this pin
        ::PIN_DIRECTION dir;
        if (FAILED(pin->QueryDirection(&dir)) ||
            ((dir != ::PINDIR_INPUT || (flags & xPIN_INPUT) == 0) &&
             (dir != ::PINDIR_OUTPUT || (flags & xPIN_OUTPUT) == 0))) {
            continue; // not the right direction, see next pin
        }
        // Query connected pin
        ComPtr<::IPin> partner;
        bool connected = SUCCEEDED(pin->ConnectedTo(partner.creator()));
        if ((connected && (flags & xPIN_CONNECTED) != 0) || (!connected && (flags & xPIN_UNCONNECTED) != 0)) {
            // Keep this pin
            result.push_back(pin);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Directly connect two filters using whatever output and input pin.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::ConnectFilters(::IGraphBuilder* graph,
                        ::IBaseFilter* filter1,
                        ::IBaseFilter* filter2,
                        ReportInterface& report)
{
    // Get unconnected pins
    PinPtrVector pins1;
    PinPtrVector pins2;
    if (!GetPin(pins1, filter1, xPIN_OUTPUT | xPIN_UNCONNECTED, report) ||
        !GetPin(pins2, filter2, xPIN_INPUT | xPIN_UNCONNECTED, report)) {
        return false;
    }

    // Try all combinations
    for (size_t i1 = 0; i1 < pins1.size(); ++i1) {
        for (size_t i2 = 0; i2 < pins2.size(); ++i2) {
            ::HRESULT hr = graph->Connect(pins1[i1].pointer(), pins2[i2].pointer());
            if (SUCCEEDED(hr)) {
                return true;
            }
            report.debug(Format("failed to connect pins, status = 0x%08X, ", int(hr)) + ComMessage(hr));
        }
    }

    // No connection made
    return false;
}


//-----------------------------------------------------------------------------
// In a DirectShow filter graph, cleanup everything downstream a filter.
//-----------------------------------------------------------------------------

bool ts::CleanupDownstream(::IGraphBuilder* graph, ::IBaseFilter* filter, ReportInterface& report)
{
    // Get connected output pins
    PinPtrVector pins;
    if (!GetPin(pins, filter, xPIN_OUTPUT | xPIN_CONNECTED, report)) {
        return false;
    }

    // Final status
    bool ok = true;

    // Loop on all connected output pins
    for (size_t pin_index = 0; pin_index < pins.size(); ++pin_index) {

        const ComPtr<::IPin>& pin(pins[pin_index]);
        ComPtr<::IPin> next_pin;
        ComPtr<::IBaseFilter> next_filter;

        // Get connected pin (input pin of next filter)
        ::HRESULT hr = pin->ConnectedTo(next_pin.creator());
        ok = ComSuccess(hr, "IPin::ConnectedTo", report) && ok;

        // Get next filter
        if (!next_pin.isNull()) {
            ::PIN_INFO pin_info;
            TS_ZERO(pin_info);
            hr = next_pin->QueryPinInfo(&pin_info);
            ok = ComSuccess(hr, "IPin::QueryPinInfo", report) && ok;
            next_filter = pin_info.pFilter; // pointer becomes managed if not null
        }

        // Recurse to cleanup downstream next filter
        if (!next_filter.isNull()) {
            ok = CleanupDownstream(graph, next_filter.pointer(), report) && ok;
        }

        // Disconnect pin to next filter
        hr = pin->Disconnect();
        ok = ComSuccess(hr, "IPin::Disconnect", report) && ok;

        // Remove next filter from the graph
        if (!next_filter.isNull()) {
            hr = graph->RemoveFilter(next_filter.pointer());
            ok = ComSuccess(hr, "IFilterGraph::RemoveFilter", report) && ok;
        }
    }

    return ok;
}


//-----------------------------------------------------------------------------
// Map a DirectShow network provider class id to a tuner type.
// Return false if no match found.
//-----------------------------------------------------------------------------

bool ts::NetworkProviderToTunerType(const ::GUID provider_clsid, TunerType& tuner_type)
{
    if (provider_clsid == ::CLSID_DVBTNetworkProvider) {
        tuner_type = ts::DVB_T;
        return true;
    }
    else if (provider_clsid == ::CLSID_DVBSNetworkProvider) {
        tuner_type = ts::DVB_S;
        return true;
    }
    else if (provider_clsid == ::CLSID_DVBCNetworkProvider) {
        tuner_type = ts::DVB_C;
        return true;
    }
    else if (provider_clsid == ::CLSID_ATSCNetworkProvider) {
        tuner_type = ts::ATSC;
        return true;
    }
    else {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Enumerate all devices of the specified class.
// Fill a vector of monikers to these objects.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::EnumerateDevicesByClass(const ::CLSID& clsid, std::vector <ComPtr <::IMoniker>>& monikers, ReportInterface& report)
{
    // Reset content of vector
    monikers.clear();

    // Create a DirectShow System Device Enumerator
    ComPtr <::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Enumerate devices
    ComPtr <::IEnumMoniker> enum_monikers;
    ::HRESULT hr = enum_devices->CreateClassEnumerator(clsid, enum_monikers.creator(), 0);
    if (!ComSuccess(hr, "CreateClassEnumerator", report)) {
        return false;
    }
    if (hr != S_OK) {
        return true; // empty category
    }

    // Loop on all enumerated providers.
    ComPtr <::IMoniker> moniker;
    while (enum_monikers->Next(1, moniker.creator(), NULL) == S_OK) {
        monikers.push_back(moniker);
    }

    return true;
}


//-----------------------------------------------------------------------------
// Get names of a tuning space. Return empty string on error.
//-----------------------------------------------------------------------------

namespace {
    std::string ToStringAndFree(::HRESULT hr, ::BSTR& name, const char* message, ts::ReportInterface& report)
    {
        std::string cname;
        if (ts::ComSuccess(hr, message, report)) {
            cname = ts::ToString(name);
        }
        if (name != NULL) {
            ::SysFreeString(name);
            name = NULL;
        }
        return cname;
    }
}

std::string ts::GetTuningSpaceFriendlyName(::ITuningSpace* tspace, ReportInterface& report)
{
    if (tspace == 0) {
        return std::string();
    }
    else {
        ::BSTR name = NULL;
        return ToStringAndFree(tspace->get_FriendlyName(&name), name, "ITuningSpace::get_FriendlyName", report);
    }
}

std::string ts::GetTuningSpaceUniqueName(::ITuningSpace* tspace, ReportInterface& report)
{
    if (tspace == 0) {
        return std::string();
    }
    else {
        ::BSTR name = NULL;
        return ToStringAndFree(tspace->get_UniqueName(&name), name, "ITuningSpace::get_UniqueName", report);
    }
}

std::string ts::GetTuningSpaceDescription(::ITuningSpace* tspace, ReportInterface& report)
{
    if (tspace == 0) {
        return std::string();
    }

    // Get tuning space names.
    const std::string fname(GetTuningSpaceFriendlyName(tspace, report));
    const std::string uname(GetTuningSpaceUniqueName(tspace, report));
    std::string tname;

    // Build description.
    if (!fname.empty()) {
        tname = "\"" + fname + "\"";
    }
    if (!uname.empty()) {
        if (!fname.empty()) {
            tname += " (";
        }
        tname += uname;
        if (!fname.empty()) {
            tname += ")";
        }
    }

    // Check if this tuning space supports IDVBTuningSpace interface.
    ComPtr<::IDVBTuningSpace> dvb_tspace;
    dvb_tspace.queryInterface(tspace, ::IID_IDVBTuningSpace, NULLREP);
    if (!dvb_tspace.isNull()) {
        // This is a DVB tuning space. Get DVB system type.
        ::DVBSystemType sys_type = ::DVB_Cable;
        ::HRESULT hr = dvb_tspace->get_SystemType(&sys_type);
        if (ComSuccess(hr, "cannot get DVB system type from tuning space \"" + fname + "\"", report)) {
            if (!tname.empty()) {
                tname += ", DVB type: ";
            }
            tname += DVBSystemTypeName(sys_type);
        }
    }

    return tname;
}


//-----------------------------------------------------------------------------
// Get the name for various enum values.
//-----------------------------------------------------------------------------

std::string ts::PinDirectionName(::PIN_DIRECTION dir)
{
    switch (dir) {
        case ::PINDIR_INPUT:  return "input";
        case ::PINDIR_OUTPUT: return "output";
        default:              return Decimal(int(dir));
    }
}

std::string ts::DVBSystemTypeName(::DVBSystemType type)
{
    switch (type) {
        case ::DVB_Cable:        return "DVB_Cable";
        case ::DVB_Terrestrial:  return "DVB_Terrestrial";
        case ::DVB_Satellite:    return "DVB_Satellite";
        case ::ISDB_Terrestrial: return "ISDB_Terrestrial";
        case ::ISDB_Satellite:   return "ISDB_Satellite";
        default:                 return Decimal(int(type));
    }
}
