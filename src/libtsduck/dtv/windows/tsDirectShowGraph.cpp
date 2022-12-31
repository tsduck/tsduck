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

#include "tsDirectShowGraph.h"
#include "tsDirectShowTest.h"
#include "tsMediaTypeUtils.h"
#include "tsMemory.h"
#include "tsNullReport.h"


//-----------------------------------------------------------------------------
// Constructor / destructor.
//-----------------------------------------------------------------------------

ts::DirectShowGraph::DirectShowGraph() :
    _graph_builder(),
    _media_control()
{
}

ts::DirectShowGraph::~DirectShowGraph()
{
    // Clear only that class.
    DirectShowGraph::clear(NULLREP);
}


//-----------------------------------------------------------------------------
// Check if the graph was correctly initialized.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::isValid() const
{
    return !_graph_builder.isNull() && !_media_control.isNull();
}


//-----------------------------------------------------------------------------
// Initialize the graph.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::initialize(Report& report)
{
    if (isValid()) {
        report.error(u"graph already initialized");
        return false;
    }

    // Create the FilterGraph object and get its GraphBuilder interface.
    _graph_builder.createInstance(::CLSID_FilterGraph, ::IID_IGraphBuilder, report);
    if (!_graph_builder.isNull()) {
        // Get its MediaControl interface.
        _media_control.queryInterface(_graph_builder.pointer(), ::IID_IMediaControl, report);
        if (_media_control.isNull()) {
            _graph_builder.release();
        }
    }

    report.debug(u"DirectShowGraph init, graph build is valid: %s, media control is valid: %s", {!_graph_builder.isNull(), !_media_control.isNull()});
    return isValid();
}


//-----------------------------------------------------------------------------
// Clear the graph back to uninitialized state.
//-----------------------------------------------------------------------------

void ts::DirectShowGraph::clear(Report& report)
{
    // Stop the graph if it is running.
    stop(report);

    // Remove all filters.
    ComPtr<::IBaseFilter> first(startingFilter(report));
    cleanupDownstream(first.pointer(), report);
    removeFilter(first.pointer(), report);

    // Release the graph object.
    _media_control.release();
    _graph_builder.release();
}


//-----------------------------------------------------------------------------
// Add /remove filter in the graph.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::addFilter(::IBaseFilter* filter, const wchar_t* name, Report& report)
{
    report.debug(u"adding filter \"%s\", graph valid: %s, filter not null: %s", {ToString(name), isValid(), filter != nullptr});

    if (isValid() && filter != nullptr) {
        const ::HRESULT hr = _graph_builder->AddFilter(filter, name != 0 ? name : L"");
        return ComSuccess(hr, u"IFilterGraph::AddFilter", report);
    }
    else {
        return false;
    }
}

bool ts::DirectShowGraph::removeFilter(::IBaseFilter* filter, Report& report)
{
    if (isValid() && filter != nullptr) {
        const ::HRESULT hr = _graph_builder->RemoveFilter(filter);
        return ComSuccess(hr, u"IFilterGraph::RemoveFilter", report);
    }
    else {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Run the graph.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::run(Report& report)
{
    if (isValid()) {
        const ::HRESULT hr = _media_control->Run();
        return ComSuccess(hr, u"cannot start DirectShow graph", report);
    }
    else {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Stop the graph.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::stop(Report& report)
{
    if (!isValid()) {
        return false;
    }

    // Get graph state
    ::OAFilterState state;
    ::HRESULT hr = _media_control->GetState(1000, &state); // 1000 ms timeout
    const bool stopped = ComSuccess(hr, u"IMediaControl::GetState", report) && state == ::State_Stopped;

    // Stop the graph when necessary
    if (stopped) {
        return true;
    }
    else {
        hr = _media_control->Stop();
        return ComSuccess(hr, u"IMediaControl::Stop", report);
    }
}


//-----------------------------------------------------------------------------
// Directly connect two filters using whatever output and input pin.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::connectFilters(::IBaseFilter* filter1, ::IBaseFilter* filter2, Report& report)
{
    if (isValid() && filter1 != 0 && filter2 != 0) {
        // Get unconnected pins
        PinPtrVector pins1;
        PinPtrVector pins2;
        if (getPin(pins1, filter1, xPIN_OUTPUT | xPIN_UNCONNECTED, report) && getPin(pins2, filter2, xPIN_INPUT | xPIN_UNCONNECTED, report)) {
            // Try all combinations
            for (size_t i1 = 0; i1 < pins1.size(); ++i1) {
                for (size_t i2 = 0; i2 < pins2.size(); ++i2) {
                    const ::HRESULT hr = _graph_builder->Connect(pins1[i1].pointer(), pins2[i2].pointer());
                    if (ComSuccess(hr, u"failed to connect pins", report)) {
                        return true;
                    }
                }
            }
        }
    }

    // No connection made.
    report.debug(u"failed to connect filters");
    return false;
}


//-----------------------------------------------------------------------------
// In the graph, cleanup everything downstream a specified filter.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::cleanupDownstream(::IBaseFilter* filter, Report& report)
{
    // Eliminate invalid parameters.
    if (!isValid() || filter == 0) {
        return false;
    }

    // Get connected output pins
    PinPtrVector pins;
    if (!getPin(pins, filter, xPIN_OUTPUT | xPIN_CONNECTED, report)) {
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
        ok = ComSuccess(hr, u"IPin::ConnectedTo", report) && ok;

        // Get next filter
        if (!next_pin.isNull()) {
            ::PIN_INFO pin_info;
            TS_ZERO(pin_info);
            hr = next_pin->QueryPinInfo(&pin_info);
            ok = ComSuccess(hr, u"IPin::QueryPinInfo", report) && ok;
            // If not null, pFilter has a reference, manage it to release it later.
            next_filter = pin_info.pFilter;
            // Pin is no longer needed. The object will be destroyed with the next
            // filter. We must no try to release next_pin afterward. So, release it now.
            next_pin.release();
        }

        // Recurse to cleanup downstream next filter
        if (!next_filter.isNull()) {
            ok = cleanupDownstream(next_filter.pointer(), report) && ok;
        }

        // Disconnect pin to next filter
        hr = pin->Disconnect();
        ok = ComSuccess(hr, u"IPin::Disconnect", report) && ok;

        // Remove next filter from the graph
        if (!next_filter.isNull()) {
            ok = removeFilter(next_filter.pointer(), report) && ok;
        }
    }

    return ok;
}


//-----------------------------------------------------------------------------
// Get the list of pins on a filter.
//-----------------------------------------------------------------------------

bool ts::DirectShowGraph::getPin(PinPtrVector& pins, ::IBaseFilter* filter, int flags, Report& report)
{
    // Clear result vector (explicitly nullify previous values to release objects)
    pins.clear();

    // If neither input nor output, neither connected nor unconnected, nothing to search.
    if ((flags & (xPIN_INPUT | xPIN_OUTPUT)) == 0 || (flags & (xPIN_CONNECTED | xPIN_UNCONNECTED)) == 0) {
        return true;
    }

    // Create a pin enumerator
    ComPtr<::IEnumPins> enum_pins;
    ::HRESULT hr = filter->EnumPins(enum_pins.creator());
    if (!ComSuccess(hr, u"IBaseFilter::EnumPins", report)) {
        return false;
    }

    // Loop on all pins
    ComPtr<::IPin> pin;
    while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {
        // Query direction of this pin
        ::PIN_DIRECTION dir;
        if (FAILED(pin->QueryDirection(&dir)) ||
            ((dir != ::PINDIR_INPUT || (flags & xPIN_INPUT) == 0) &&
            (dir != ::PINDIR_OUTPUT || (flags & xPIN_OUTPUT) == 0)))
        {
            continue; // not the right direction, see next pin
        }
        // Query connected pin
        ComPtr<::IPin> partner;
        const bool connected = SUCCEEDED(pin->ConnectedTo(partner.creator()));
        if ((connected && (flags & xPIN_CONNECTED) != 0) || (!connected && (flags & xPIN_UNCONNECTED) != 0)) {
            // Keep this pin
            pins.push_back(pin);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Get the starting filter of the graph.
//-----------------------------------------------------------------------------

ts::ComPtr<::IBaseFilter> ts::DirectShowGraph::startingFilter(Report& report)
{
    if (isValid()) {
        // Enumerate all filters in the graph.
        ComPtr<::IEnumFilters> enum_filters;
        const ::HRESULT hr = _graph_builder->EnumFilters(enum_filters.creator());
        if (ComSuccess(hr, u"IFilterGraph::EnumFilters", report)) {
            // Find first filter with no connected in pin.
            ComPtr<::IBaseFilter> filter;
            PinPtrVector pins;
            while (enum_filters->Next(1, filter.creator(), NULL) == S_OK) {
                if (!getPin(pins, filter.pointer(), xPIN_INPUT | xPIN_CONNECTED, report)) {
                    break;
                }
                if (pins.empty()) {
                    // Found one without connected input pin, this is a starting point of the graph.
                    return filter;
                }
            }
        }
    }

    // Found no starting point (invalid or empty graph)
    return ComPtr<::IBaseFilter>();
}


//-----------------------------------------------------------------------------
// Display the description of a DirectShow filter graph.
//-----------------------------------------------------------------------------

void ts::DirectShowGraph::display(std::ostream& output, Report& report, const UString& margin, bool verbose)
{
    display(output, report, startingFilter(report), margin, verbose);
}


//-----------------------------------------------------------------------------
// Display the description of a partial DirectShow filter graph.
//-----------------------------------------------------------------------------

void ts::DirectShowGraph::display(std::ostream& output, Report& report, const ComPtr<::IBaseFilter>& start_filter, const UString& margin, bool verbose)
{
    DirectShowTest test(output, report);
    ComPtr<::IBaseFilter> filter(start_filter);

    // Loop on all filters in the graph
    while (isValid() && !filter.isNull()) {

        // Get filter name
        ::FILTER_INFO filter_info;
        ::HRESULT hr = filter->QueryFilterInfo(&filter_info);
        if (!ComSuccess(hr, u"IBaseFilter::QueryFilterInfo", report)) {
            return;
        }
        filter_info.pGraph->Release();
        UString filter_name(ToString(filter_info.achName));

        // Get filter vendor info (may be unimplemented)
        UString filter_vendor;
        ::WCHAR* wstring;
        hr = filter->QueryVendorInfo(&wstring);
        if (SUCCEEDED(hr)) {
            filter_vendor = ToString(wstring);
            ::CoTaskMemFree(wstring);
        }

        // Get filter class GUID if persistent
        ::CLSID class_id(GUID_NULL);
        ComPtr<::IPersist> persist;
        persist.queryInterface(filter.pointer(), ::IID_IPersist, NULLREP);
        if (!persist.isNull()) {
            // Filter class implements IPersist
            hr = persist->GetClassID(&class_id);
            if (!ComSuccess(hr, u"get filter class guid", report)) {
                return;
            }
        }

        // Get connected output pins
        PinPtrVector pins;
        if (!getPin(pins, filter.pointer(), xPIN_OUTPUT | xPIN_CONNECTED, report)) {
            return;
        }

        // Display the filter info
        char bar = pins.size() > 1 ? '|' : ' ';
        if (verbose) {
            output << margin << std::endl;
        }
        output << margin << "- Filter \"" << filter_name << "\"" << std::endl;
        if (verbose) {
            if (!filter_vendor.empty()) {
                output << margin << bar << " vendor: \"" << filter_vendor << "\"" << std::endl;
            }
            output << margin << bar << " class GUID: " << NameGUID(class_id) << " " << FormatGUID(class_id) << std::endl;
            test.displayObject(filter.pointer(), margin + bar + u" ");
        }

        // Loop on all connected output pins
        for (size_t pin_index = 0; pin_index < pins.size(); ++pin_index) {
            const ComPtr <::IPin>& out_pin(pins[pin_index]);

            // If more than one output pin, we need to indent and recurse
            bool last_pin = pin_index == pins.size() - 1;
            UString margin0(margin);
            UString margin1(margin);
            UString margin2(margin);
            if (pins.size() > 1) {
                margin0 += u"|";
                margin1 += u"+--";
                margin2 += last_pin ? u"   " : u"|  ";
            }

            // Get output pin info
            ::PIN_INFO pin_info;
            hr = out_pin->QueryPinInfo(&pin_info);
            if (!ComSuccess(hr, u"IPin::QueryPinInfo", report)) {
                return;
            }
            UString pin_name(ToString(pin_info.achName));
            pin_info.pFilter->Release();

            // Get output pin id
            ::WCHAR* wid;
            hr = out_pin->QueryId(&wid);
            if (!ComSuccess(hr, u"IPin::QueryPinId", report)) {
                return;
            }
            UString pin_id(ToString(wid));
            ::CoTaskMemFree(wid);

            // Display output pin info
            if (verbose) {
                output << margin0 << std::endl;
            }
            output << margin1 << "- Output pin \"" << pin_name << "\", id \"" << pin_id << "\"" << std::endl;
            if (verbose) {
                test.displayObject(out_pin.pointer(), margin2 + u"  ");
            }

            // Get connection media type
            ::AM_MEDIA_TYPE media;
            hr = out_pin->ConnectionMediaType(&media);
            if (!ComSuccess(hr, u"IPin::ConnectionMediaType", report)) {
                return;
            }

            // Display media type (and free its resources)
            if (verbose) {
                output << margin2 << std::endl
                       << margin2 << "- Media major type " << NameGUID(media.majortype) << std::endl
                       << margin2 << "  subtype " << NameGUID(media.subtype) << std::endl
                       << margin2 << "  format " << NameGUID(media.formattype) << std::endl;
            }
            else {
                output << margin2 << "- Media type "
                       << NameGUID(media.majortype) << " / "
                       << NameGUID(media.subtype) << std::endl;
            }
            FreeMediaType(media);

            // Get connected pin (input pin of next filter)
            ComPtr<::IPin> in_pin;
            hr = out_pin->ConnectedTo(in_pin.creator());
            if (!ComSuccess(hr, u"IPin::ConnectedTo", report)) {
                return;
            }

            // Get next input pin info
            hr = in_pin->QueryPinInfo(&pin_info);
            if (!ComSuccess(hr, u"IPin::QueryPinInfo", report)) {
                return;
            }
            pin_name = ToString(pin_info.achName);
            filter = pin_info.pFilter; // next filter in the chain

            // Get input pin id
            hr = in_pin->QueryId(&wid);
            if (!ComSuccess(hr, u"IPin::QueryPinId", report)) {
                return;
            }
            pin_id = ToString(wid);
            ::CoTaskMemFree(wid);

            // Display input pin info
            if (verbose) {
                output << margin2 << std::endl;
            }
            output << margin2 << "- Input pin \"" << pin_name << "\", id \"" << pin_id << "\"" << std::endl;
            if (verbose) {
                test.displayObject(in_pin.pointer(), margin2 + u"  ");
            }

            // If more than one branch, recurse
            if (pins.size() > 1) {
                display(output, report, filter, margin2, verbose);
            }
        }
        if (pins.size() != 1) {
            // no connected output pin, end of graph, or more than one and we recursed.
            break;
        }
    }
}
