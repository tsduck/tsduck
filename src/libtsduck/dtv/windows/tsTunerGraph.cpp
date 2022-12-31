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

#include "tsTunerGraph.h"
#include "tsDuckContext.h"
#include "tsDirectShowUtils.h"


//-----------------------------------------------------------------------------
// Constructor / destructor.
//-----------------------------------------------------------------------------

ts::TunerGraph::TunerGraph() :
    DirectShowGraph(),
    _user_receiver_name(),
    _tuner_name(),
    _sink_filter(),
    _provider_filter(),
    _inet_provider(),
    _ituner(),
    _ituner_cap(),
    _net_types(),
    _tuner_filter(),
    _demods(),
    _demods2(),
    _sigstats(),
    _tunprops()
{
}

ts::TunerGraph::~TunerGraph()
{
    // Clear only that class and subclasses.
    TunerGraph::clear(NULLREP);
}


//-----------------------------------------------------------------------------
// Clear the graph back to uninitialized state.
//-----------------------------------------------------------------------------

void ts::TunerGraph::clear(Report& report)
{
    // Clear superclass.
    DirectShowGraph::clear(report);

    // Release local COM objects.
    _tuner_name.clear();
    _sink_filter.release();
    _provider_filter.release();
    _inet_provider.release();
    _ituner.release();
    _ituner_cap.release();
    _net_types.clear();
    _tuner_filter.release();
    _demods.clear();
    _demods2.clear();
    _sigstats.clear();
    _tunprops.clear();
}


//-----------------------------------------------------------------------------
// Initialize the graph.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::initialize(const UString& tuner_name, ::IMoniker* tuner_moniker, DeliverySystemSet& delivery_systems, Report& report)
{
    // Initialize this object.
    clear(report);
    _tuner_name = tuner_name;

    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Instantiate the "Microsoft Network Provider". In the past, we tried all specific providers
    // like "Microsoft DVBT Network Provider". However, these are now deprecated and Microsoft
    // advises to use the new generic one. This provider can work with all tuners. It will accept
    // only the tuning spaces which are compatible with the connected tuner.
    // Also get a few interfaces interfaces of the network provider filter

    _provider_filter.createInstance(::CLSID_NetworkProvider, ::IID_IBaseFilter, report);
    _inet_provider.queryInterface(_provider_filter.pointer(), ::IID_IBDA_NetworkProvider, report);
    _ituner.queryInterface(_provider_filter.pointer(), ::IID_ITuner, report);
    _ituner_cap.queryInterface(_provider_filter.pointer(), ::IID_ITunerCap, debug_report);
    if (_provider_filter.isNull() || _inet_provider.isNull() || _ituner.isNull() || _ituner_cap.isNull()) {
        report.debug(u"failed to create an instance of network provider for \"%s\"", {_tuner_name});
        return false;
    }

    // Create an instance of tuner from moniker
    _tuner_filter.bindToObject(tuner_moniker, ::IID_IBaseFilter, report);
    if (_tuner_filter.isNull()) {
        report.debug(u"failed to create an instance of BDA tuner for \"%s\"", {_tuner_name});
        return false;
    }

    // Create the Filter Graph, add the filters and connect network provider to tuner.
    if (!DirectShowGraph::initialize(report) ||
        !addFilter(_provider_filter.pointer(), L"NetworkProvider", report) ||
        !addFilter(_tuner_filter.pointer(), L"Tuner", report) ||
        !connectFilters(_provider_filter.pointer(), _tuner_filter.pointer(), report))
    {
        report.debug(u"failed to initiate the graph with network provider => tuner for \"%s\"", {_tuner_name});
        clear(report);
        return false;
    }

    // Now, the network provider is connected to the tuner.
    // We can get all supported network types. Note that we query the network provider
    // filter for the capabilities of the tuner filter. Strange but this is the way it works.
    std::array<::GUID,16> net_types;
    ::ULONG net_count = ::LONG(net_types.size());
    ::HRESULT hr = _ituner_cap->get_SupportedNetworkTypes(net_count, &net_count, net_types.data());
    if (!ComSuccess(hr, u"ITunerCap::get_SupportedNetworkTypes", report)) {
        clear(report);
        return false;
    }
    else if (net_count == 0) {
        report.error(u"tuner \"%s\" did not return any supported network type", {_tuner_name});
        clear(report);
        return false;
    }

    // Loop on all supported network types and build corresponding tuning spaces.
    for (size_t net_index = 0; net_index < net_count; ++net_index) {
        DirectShowNetworkType net;
        if (!net.initialize(net_types[net_index], report)) {
            report.debug(u"failed to set network type %s", {NameGUID(net_types[net_index])});
        }
        else {
            // Add the delivery systems for the network type to the tuner.
            net.getDeliverySystems(delivery_systems);
            // Register the network type by tuner type.
            _net_types.insert(std::make_pair(net.tunerType(), net));
        }
    }

    // Check that we have at least one network type.
    if (_net_types.empty()) {
        report.error(u"tuner \"%s\" failed to support all network types", {_tuner_name});
        clear(report);
        return false;
    }

    bool graph_done = false;
    bool user_receiver_found = false;
    report.debug(u"user-specified receiver filter name: \"%s\"", {_user_receiver_name});

    // Try to build the rest of the graph starting at tuner filter, without receiver filter.
    // Usually work with Terratec driver for instance.
    if (_user_receiver_name.empty()) {
        report.debug(u"trying direct connection from tuner (no receiver)");
        graph_done = buildGraphAtTee(_tuner_filter, report);
    }

    // If the tuner cannot be directly connected to the rest of the graph, we need to find
    // a specific "receiver" filter (usually provided by the same vendor as the tuner filter).
    // Needed by Hauppauge or Pinnacle drivers for instance.
    if (!graph_done) {

        // Enumerate all filters with category KSCATEGORY_BDA_RECEIVER_COMPONENT
        std::vector <ComPtr<::IMoniker>> receiver_monikers;
        if (!EnumerateDevicesByClass(KSCATEGORY_BDA_RECEIVER_COMPONENT, receiver_monikers, report)) {
            clear(report);
            return false;
        }

        // Loop on all enumerated receiver filters
        for (size_t receiver_index = 0; !graph_done && !user_receiver_found && receiver_index < receiver_monikers.size(); ++receiver_index) {

            // Get friendly name of this network provider
            UString receiver_name(GetStringPropertyBag(receiver_monikers[receiver_index].pointer(), L"FriendlyName", debug_report));

            // Check if a specific receiver is provider by the user.
            user_receiver_found = !_user_receiver_name.empty() && _user_receiver_name.similar(receiver_name);

            // Try this receiver if there is no user-specified one or it is the user-specified one.
            if (_user_receiver_name.empty() || user_receiver_found) {
                report.debug(u"trying receiver filter \"%s\"", {receiver_name});

                // Create an instance of this receiver filter from moniker
                ComPtr<::IBaseFilter> receiver_filter;
                receiver_filter.bindToObject(receiver_monikers[receiver_index].pointer(), ::IID_IBaseFilter, debug_report);
                if (receiver_filter.isNull()) {
                    continue; // give up this receiver filter
                }

                // Add the filter in the graph
                if (!addFilter(receiver_filter.pointer(), L"Receiver", report)) {
                    continue; // give up this receiver filter
                }

                // Try to connect the tuner to the receiver
                if (!connectFilters(_tuner_filter.pointer(), receiver_filter.pointer(), debug_report)) {
                    // This receiver is not compatible, remove it from the graph
                    removeFilter(receiver_filter.pointer(), debug_report);
                    continue;
                }

                // Try to build the rest of the graph
                if (buildGraphAtTee(receiver_filter, report)) {
                    graph_done = true;
                    report.debug(u"using receiver filter \"%s\"", {receiver_name});
                }
            }
        }
    }
    if (!graph_done) {
        if (!_user_receiver_name.empty() && !user_receiver_found) {
            report.error(u"receiver filter \"%s\" not found", {_user_receiver_name});
        }
        clear(report);
        return false;
    }

    // Locate all instances of some interfaces in the tuner topology
    _demods.clear();
    _demods2.clear();
    _sigstats.clear();
    _tunprops.clear();

    // Lookup all internal nodes in the BDA topology
    ComPtr<::IBDA_Topology> topology;
    topology.queryInterface(_tuner_filter.pointer(), ::IID_IBDA_Topology, NULLREP);
    if (!topology.isNull()) {
        // Get node types
        static const ::ULONG MAX_NODES = 64;
        ::ULONG count = MAX_NODES;
        ::ULONG types[MAX_NODES];
        if (SUCCEEDED(topology->GetNodeTypes(&count, MAX_NODES, types))) {
            // Enumerate all node types
            for (::ULONG n = 0; n < count; ++n) {
                // Get control node for this type
                ComPtr<::IUnknown> cnode;
                if (SUCCEEDED(topology->GetControlNode(0, 1, types[n], cnode.creator()))) {
                    findTunerSubinterfaces(cnode);
                }
            }
        }
    }

    // Look at all connected pins
    ComPtr<::IEnumPins> enum_pins;
    if (SUCCEEDED(_tuner_filter->EnumPins(enum_pins.creator()))) {
        // Enumerate all pins in tuner filter
        ComPtr<::IPin> pin;
        while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {
            // Check if this pin is connected
            ComPtr<::IPin> partner;
            if (SUCCEEDED(pin->ConnectedTo(partner.creator()))) {
                findTunerSubinterfaces(pin);
            }
        }
    }

    report.debug(u"IBDA_DigitalDemodulator in tuner: %d", {_demods.size()});
    report.debug(u"IBDA_DigitalDemodulator2 in tuner: %d", {_demods2.size()});
    report.debug(u"IBDA_SignalStatistics in tuner: %d", {_sigstats.size()});
    report.debug(u"IKsPropertySet in tuner: %d", {_tunprops.size()});

    return true;
}


//-----------------------------------------------------------------------------
// Try to build the part of the graph starting at the tee filter.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::buildGraphAtTee(const ComPtr<::IBaseFilter>& base_filter, Report& report)
{
    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Create an "infinite tee filter"
    ComPtr<::IBaseFilter> tee_filter(CLSID_InfTee, IID_IBaseFilter, report);
    if (tee_filter.isNull()) {
        return false;
    }

    // Add the tee filter to the graph
    if (!addFilter(tee_filter.pointer(), L"Tee", report)) {
        return false;
    }

    // After this point, we cannot simply return false on error since the graph needs some cleanup.
    // Try to connect the "base" filter (tuner or receiver) to the tee filter.
    bool ok = connectFilters(base_filter.pointer(), tee_filter.pointer(), debug_report);

    // Create branch A of graph: Create a sink filter, add it to the graph and connect it to the tee.
    ComPtr<SinkFilter> sink(new SinkFilter(report));
    CheckNonNull(sink.pointer());
    ok = ok &&
         addFilter(sink.pointer(), L"Sink/Capture", report) &&
         connectFilters(tee_filter.pointer(), sink.pointer(), debug_report);

    // Create branch B of graph: Create an MPEG-2 demultiplexer followed by a Transport Information Filter (TIF).
    ComPtr<::IBaseFilter> demux_filter(::CLSID_MPEG2Demultiplexer, ::IID_IBaseFilter, report);
    ok = ok &&
         !demux_filter.isNull() &&
         addFilter(demux_filter.pointer(), L"Demux", report) &&
         connectFilters(tee_filter.pointer(), demux_filter.pointer(), debug_report) &&
         buildGraphAtTIF(demux_filter, report);

    // If successful so far, done
    if (ok) {
        _sink_filter = sink;
        return true;
    }

    // Not successful, cleanup everything.
    // Cleanup the graph downstream the tuner filter.
    // This will also remove any optional receiver filter between the tuner and the tee.
    cleanupDownstream(_tuner_filter.pointer(), debug_report);

    // Remove all created filters from the graph. Ignore errors.
    // This is necessary if a filter was created and added to the graph but
    // not connected (if connected, it was removed by CleanupDownstream).
    if (!tee_filter.isNull()) {
        removeFilter(tee_filter.pointer(), report);
    }
    if (!sink.isNull()) {
        removeFilter(sink.pointer(), report);
    }
    if (!demux_filter.isNull()) {
        removeFilter(demux_filter.pointer(), report);
    }

    return false;
}


//-----------------------------------------------------------------------------
// Try to build the end of the graph starting at the TIF, after the demux.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::buildGraphAtTIF(const ComPtr <::IBaseFilter>& demux, Report& report)
{
    // Connect a Transport Information Filter (TIF).
    //
    // The usual TIF is "BDA MPEG2 Transport Information Filter" but there is no predefined
    // CLSID for this one and it is not guaranteed that this TIF will remain on all versions.
    // The recommended procedure is to enumerate and try all filters with category
    // KSCATEGORY_BDA_TRANSPORT_INFORMATION. But we can see that two TIF exist in the
    // system. The second one is "MPEG-2 Sections and Tables". So, we are facing a dilemma:
    //
    // 1) If we hard-code the CLSID for "BDA MPEG2 Transport Information Filter" and use it
    //    directly, we may get an error in some future version of Windows if this filter
    //    is no longer supported.
    //
    // 2) If we follow the recommended procedure, it works only because the "BDA MPEG2
    //    Transport Information Filter" comes first in the enumeration. What will happen
    //    if the order changes in some future version?
    //
    // So, to stay on the safe side, we first try a direct activation of "BDA MPEG2
    // Transport Information Filter" using its known (although not predefined) CLSID.
    // And if it fails, we fallback into the enumeration.

    // Try "BDA MPEG2 Transport Information Filter" using a known CLSID.
    ComPtr<::IBaseFilter> tif(::CLSID_BDA_MPEG2TransportInformationFilter, ::IID_IBaseFilter, report);
    if (installTIF(demux, tif, report)) {
        // Know TIF properly installed, use it.
        return true;
    }
    tif.release();

    // Failed to use the known TIF, enumerate them all.
    // Create a DirectShow System Device Enumerator
    ComPtr<::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Enumerate all TIF.
    ComPtr<::IEnumMoniker> enum_tif;
    ::HRESULT hr = enum_devices->CreateClassEnumerator(KSCATEGORY_BDA_TRANSPORT_INFORMATION, enum_tif.creator(), 0);
    if (!ComSuccess(hr, u"CreateClassEnumerator (for TIF)", report) || hr != S_OK) {
        // Must use ComSuccess to get a message in case of error.
        // Must also explicitly test for S_OK because empty categories return another success status.
        return false;
    }

    // Loop on all enumerated TIF
    ComPtr<::IMoniker> tif_moniker;
    while (enum_tif->Next(1, tif_moniker.creator(), NULL) == S_OK) {

        // Get friendly name of this TIF
        UString tif_name(GetStringPropertyBag(tif_moniker.pointer(), L"FriendlyName", report));
        report.debug(u"trying TIF \"%s\"", {tif_name});

        // Create an instance of this TIF from moniker
        tif.bindToObject(tif_moniker.pointer(), ::IID_IBaseFilter, report);
        if (installTIF(demux, tif, report)) {
            // TIF properly installed, use it.
            return true;
        }
        tif.release();
    }

    // All TIF were rejected.
    report.debug(u"all TIF failed");
    return false;
}


//-----------------------------------------------------------------------------
// Try to install a "transport information filter" (TIF), after the demux filter.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::installTIF(const ComPtr<::IBaseFilter>& demux, const ComPtr<::IBaseFilter>& tif, Report& report)
{
    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // Add the TIF in the graph
    if (tif.isNull() || !addFilter(tif.pointer(), L"TIF", report)) {
        return false;
    }

    // Try to connect demux filter to tif
    if (connectFilters(demux.pointer(), tif.pointer(), debug_report)) {
        return true;
    }
    else {
        // This tif is not compatible, remove it from the graph
        removeFilter(tif.pointer(), report);
        return false;
    }
}


//-----------------------------------------------------------------------------
// Send a tune request.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::sendTuneRequest(DuckContext& duck, const ModulationArgs& params)
{
    // Check if we have a corresponding network type.
    if (!params.delivery_system.set()) {
        duck.report().error(u"no delivery system specified");
        return false;
    }
    const TunerType ttype = TunerTypeOf(params.delivery_system.value());
    const auto it = _net_types.find(ttype);
    if (it == _net_types.end()) {
        duck.report().error(u"tuner \"%s\" does not support %s", {_tuner_name, TunerTypeEnum.name(ttype)});
        return false;
    }

    // The network type object.
    DirectShowNetworkType& net(it->second);
    assert(net.tuningSpace() != nullptr);

    // Set the tuning space for this network type as current tuning space in the network provider.
    ::HRESULT hr = _ituner->put_TuningSpace(net.tuningSpace());
    if (!ComSuccess(hr, u"setting tuning space " + net.tuningSpaceName(), duck.report())) {
        return false;
    }

    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    if (!CreateTuneRequest(duck, tune_request, net.tuningSpace(), params)) {
        return false;
    }
    assert(!tune_request.isNull());

    // Send the tune request to the network provider.
    hr = _ituner->put_TuneRequest(tune_request.pointer());
    return ComSuccess(hr, u"DirectShow tuning error", duck.report());
}
