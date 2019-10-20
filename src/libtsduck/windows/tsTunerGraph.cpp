//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsDirectShowUtils.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Constructor / destructor.
//-----------------------------------------------------------------------------

ts::TunerGraph::TunerGraph() :
    DirectShowGraph(),
    _sink_filter(),
    _provider_filter(),
    _inet_provider(),
    _ituner(),
    _ituning_space(),
    _tuning_space_fname(),
    _tuning_space_uname(),
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
    _sink_filter.release();
    _provider_filter.release();
    _inet_provider.release();
    _ituner.release();
    _ituning_space.release();
    _tuning_space_fname.clear();
    _tuning_space_uname.clear();
    _tuner_filter.release();
    _demods.clear();
    _demods2.clear();
    _sigstats.clear();
    _tunprops.clear();
}


//-----------------------------------------------------------------------------
// Send a tune request.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::putTuneRequest(::ITuneRequest* request, Report& report)
{
    ::HRESULT hr = _ituner->put_TuneRequest(request);
    return ComSuccess(hr, u"DirectShow tuning error", report);
}


//-----------------------------------------------------------------------------
// Initialize the graph.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::initialize(::IMoniker* tuner_moniker, DeliverySystemSet& delivery_systems, Report& report)
{
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
    if (_provider_filter.isNull() || _inet_provider.isNull() || _ituner.isNull()) {
        report.debug(u"failed to create an instance of network provider");
        return false;
    }

    // Create an instance of tuner from moniker
    _tuner_filter.bindToObject(tuner_moniker, ::IID_IBaseFilter, report);
    if (_tuner_filter.isNull()) {
        report.debug(u"failed to create an instance of BDA tuner");
        return false;
    }

    // Create the Filter Graph, add the filters and connect network provider to tuner.
    if (!DirectShowGraph::initialize(report) ||
        !addFilter(_provider_filter.pointer(), L"NetworkProvider", report) ||
        !addFilter(_tuner_filter.pointer(), L"Tuner", report) ||
        !connectFilters(_provider_filter.pointer(), _tuner_filter.pointer(), report))
    {
        report.debug(u"failed to initiate the graph with network provider => tuner");
        clear(report);
        return false;
    }

    // Now, the network provider is connected to the tuner.

    // In debug mode, get all supported network types.
    // For debug only, sometimes it does not work.
    if (report.debug()) {
        ComPtr<::ITunerCap> tuner_cap;
        tuner_cap.queryInterface(_ituner.pointer(), ::IID_ITunerCap, debug_report);
        if (tuner_cap.isNull()) {
            report.error(u"failed to get ITunerCap interface");
            // Debug only: return false;
        }
        else {
            std::array<::GUID,10> net_types;
            ::ULONG net_count = ::LONG(net_types.size());
            ::HRESULT hr = tuner_cap->get_SupportedNetworkTypes(net_count, &net_count, net_types.data());
            if (!ComSuccess(hr, u"ITunerCap::get_SupportedNetworkTypes", report)) {
                // Debug only: return false;
            }
            else if (net_count == 0) {
                report.error(u"tuner did not return any supported network types");
                // Debug only: return false;
            }
            else {
                report.debug(u"Supported Network Types:");
                for (size_t n = 0; n < net_count; n++) {
                    report.debug(u"  %d) %s", {n, NameGUID(net_types[n])});
                }
            }
        }
    }

    // Now, we are going to try all tuning spaces. Normally, the network provider will
    // reject the tuning spaces which are not compatible with the tuner.

    // Enumerate all tuning spaces in the system.
    ComPtr<::ITuningSpaceContainer> tsContainer(::CLSID_SystemTuningSpaces, ::IID_ITuningSpaceContainer, report);
    if (tsContainer.isNull()) {
        clear(report);
        return false;
    }
    ComPtr<::IEnumTuningSpaces> tsEnum;
    ::HRESULT hr = tsContainer->get_EnumTuningSpaces(tsEnum.creator());
    if (!ComSuccess(hr, u"ITuningSpaceContainer::get_EnumTuningSpaces", report)) {
        clear(report);
        return false;
    }

    // Loop on all tuning spaces.
    bool tspace_found = false;
    ts::ComPtr<::ITuningSpace> tspace;
    while (!tspace_found && tsEnum->Next(1, tspace.creator(), nullptr) == S_OK) {

        // Display tuning space in debug mode
        const UString fname(GetTuningSpaceFriendlyName(tspace.pointer(), report));
        const UString uname(GetTuningSpaceUniqueName(tspace.pointer(), report));
        report.debug(u"found tuning space \"%s\" (%s)", {fname, uname});

        // Try to use this tuning space with our tuner.
        hr = _ituner->put_TuningSpace(tspace.pointer());
        if (ComSuccess(hr, u"fail to set tuning space \"" + fname + u"\"", debug_report)) {

            // This tuning space is compatible with our tuner.
            // Check if this is a tuning space we can support by getting its DVB system type:
            // first get IDVBTuningSpace interface of tuning space (may not support it)
            ComPtr<::IDVBTuningSpace> dvb_tspace;
            dvb_tspace.queryInterface(tspace.pointer(), ::IID_IDVBTuningSpace, debug_report);
            if (!dvb_tspace.isNull()) {

                // Get DVB system type
                ::DVBSystemType systype;
                hr = dvb_tspace->get_SystemType(&systype);
                if (!ComSuccess(hr, u"cannot get DVB system type from tuning space \"" + fname + u"\"", report)) {
                    continue;
                }
                report.debug(u"DVB system type is %d for tuning space \"%s\"", { systype, fname });

                // Check if DVB system type matches our tuner type.
                switch (systype) {
                    case ::DVB_Satellite: {
                        tspace_found = true;
                        delivery_systems.insert(DS_DVB_S);
                        delivery_systems.insert(DS_DVB_S2);
                        // No way to check if DS_DVB_S2 is supported, assume it.
                        break;
                    }
                    case ::DVB_Terrestrial: {
                        tspace_found = true;
                        delivery_systems.insert(DS_DVB_T);
                        delivery_systems.insert(DS_DVB_T2);
                        // No way to check if DS_DVB_T2 is supported, assume it.
                        break;
                    }
                    case ::DVB_Cable: {
                        tspace_found = true;
                        delivery_systems.insert(DS_DVB_C_ANNEX_A);
                        delivery_systems.insert(DS_DVB_C_ANNEX_C);
                        // No way to check which annex is supported. Skip annex B (too special).
                        break;
                    }
                    case ::ISDB_Terrestrial:
                    case ::ISDB_Satellite:
                    default: {
                        // Not a kind of tuning space we support.
                        break;
                    }
                }
            }
            else {
                // Not a DVB tuning space, silently ignore it.
                report.debug(u"tuning space \"%s\" does not support IID_IDVBTuningSpace interface", {fname});
            }

            // Check if this is a tuning space we can support by getting its ATSC network type:
            // first get IATSCTuningSpace interface of tuning space (may not support it)
            ComPtr<::IATSCTuningSpace> atsc_tspace;
            atsc_tspace.queryInterface(tspace.pointer(), ::IID_IATSCTuningSpace, debug_report);
            if (!atsc_tspace.isNull()) {

                // Get ATSC network type
                ::GUID nettype;
                hr = atsc_tspace->get__NetworkType(&nettype);
                if (!ComSuccess(hr, u"cannot get ATSC network type from tuning space \"" + fname + u"\"", report)) {
                    continue;
                }
                report.debug(u"ATSC network type is \"%s\" for tuning space \"%s\"", { GetTuningSpaceNetworkType(tspace.pointer(), report), fname });

                // Check if ATSC network type matches our tuner type.
                if (nettype == CLSID_ATSCNetworkProvider) {
                    tspace_found = true;
                    delivery_systems.insert(DS_ATSC);
                }
            }
            else {
                // Not an ATSC tuning space, silently ignore it.
                report.debug(u"tuning space \"%s\" does not support IID_IATSCTuningSpace interface", {fname});
            }
        }
    }

    // Give up the tuner if no tuning space was found.
    if (!tspace_found) {
        report.debug(u"no supported tuning space found for this tuner");
        clear(report);
        return false;
    }

    // Keep this tuning space.
    _ituning_space = tspace;
    _tuning_space_fname = GetTuningSpaceFriendlyName(_ituning_space.pointer(), report);
    _tuning_space_uname = GetTuningSpaceUniqueName(_ituning_space.pointer(), report);
    report.debug(u"using tuning space \"%s\" (\"%s\")", {_tuning_space_uname, _tuning_space_fname});

    // Try to build the rest of the graph starting at tuner filter.
    // Usually work with Terratec driver for instance.
    report.debug(u"trying direct connection from tuner (no receiver)");
    bool graph_done = buildCaptureGraph(_tuner_filter, report);

    // If the tuner cannot be directly connected to the rest of the
    // graph, we need to find a specific "receiver" filter (usually
    // provided by the same vendor as the tuner filter). Needed by
    // Hauppauge or Pinnacle drivers for instance.
    if (!graph_done) {

        // Enumerate all filters with category KSCATEGORY_BDA_RECEIVER_COMPONENT
        std::vector <ComPtr<::IMoniker>> receiver_monikers;
        if (!EnumerateDevicesByClass(KSCATEGORY_BDA_RECEIVER_COMPONENT, receiver_monikers, report)) {
            clear(report);
            return false;
        }

        // Loop on all enumerated receiver filters
        for (size_t receiver_index = 0; !graph_done && receiver_index < receiver_monikers.size(); ++receiver_index) {

            // Get friendly name of this network provider
            UString receiver_name(GetStringPropertyBag(receiver_monikers[receiver_index].pointer(), L"FriendlyName", debug_report));
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
            if (buildCaptureGraph(receiver_filter, report)) {
                graph_done = true;
                report.debug(u"using receiver filter \"%s\"", {receiver_name});
            }
        }
    }
    if (!graph_done) {
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

bool ts::TunerGraph::buildCaptureGraph(const ComPtr<::IBaseFilter>& base_filter, Report& report)
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
         buildGraphEnd(demux_filter, report);

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
// Try to build the end of the graph, after the demux filter.
//-----------------------------------------------------------------------------

bool ts::TunerGraph::buildGraphEnd(const ComPtr <::IBaseFilter>& demux, Report& report)
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
