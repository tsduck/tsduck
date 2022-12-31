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

#include "tsDirectShowTest.h"
#include "tsDirectShowUtils.h"
#include "tsDirectShowGraph.h"
#include "tsDirectShowFilterCategory.h"
#include "tsNullReport.h"

//
// An enumeration of TestType names.
//
const ts::Enumeration ts::DirectShowTest::TestNames({
    {u"none",              NONE},
    {u"list-devices",      LIST_DEVICES},
    {u"enumerate-devices", ENUMERATE_DEVICES},
    {u"tuning-spaces",     TUNING_SPACES},
    {u"bda-tuners",        BDA_TUNERS},
});


//-----------------------------------------------------------------------------
// Constructor.
//-----------------------------------------------------------------------------

ts::DirectShowTest::DirectShowTest(std::ostream& output, Report& report) :
    _output(output),
    _report(report)
{
}


//-----------------------------------------------------------------------------
// Run one test.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::runTest(TestType type)
{
    switch (type) {
        case NONE:
            break;
        case LIST_DEVICES:
            listDevices();
            break;
        case ENUMERATE_DEVICES:
            enumerateDevices();
            break;
        case TUNING_SPACES:
            testTuningSpaces();
            break;
        case BDA_TUNERS:
            testBDATuners();
            break;
        default:
            _report.error(u"undefined DirectShow test %d", {type});
            break;
    }
}


//-----------------------------------------------------------------------------
// Test BDA tuners, same as runTest(BDA_TUNERS).
//-----------------------------------------------------------------------------

void ts::DirectShowTest::testBDATuners(const UString& margin)
{
    // Build an instance of all tuners.
    DirectShowFilterCategory tuners(KSCATEGORY_BDA_NETWORK_TUNER, _report);
    if (tuners.empty()) {
        _report.error(u"no BDA tuner found");
        return;
    }

    // Loop on all BDA tuners.
    for (size_t tuner_index = 0; tuner_index < tuners.size(); ++tuner_index) {

        // Name of this tuner.
        _output << std::endl << margin << "=== Testing \"" << tuners.name(tuner_index) << "\"" << std::endl << std::endl;

        // Build a DirectShow graph.
        DirectShowGraph graph;
        if (!graph.initialize(_report)) {
            break; // fatal error
        }

        // Add the tuner in the graph.
        if (!graph.addFilter(tuners.filter(tuner_index).pointer(), L"Tuner", _report)) {
            // This tuner cannot be added to a graph. Move to next tuner.
            continue;
        }

        // Get all network providers.
        DirectShowFilterCategory providers(KSCATEGORY_BDA_NETWORK_PROVIDER, _report);

        // Loop on all network providers.
        for (size_t prov_index = 0; prov_index < providers.size(); ++prov_index) {

            ComPtr<::IBaseFilter> provider(providers.filter(prov_index));
            _output << margin << "- Trying \"" << providers.name(prov_index) << "\"" << std::endl;

            // Get the tuner interface of the network provider.
            ComPtr<::ITuner> iTuner;
            iTuner.queryInterface(provider.pointer(), ::IID_ITuner, _report);
            if (iTuner.isNull()) {
                _output << margin << "  No ITuner interface on this network provider" << std::endl;
                continue;
            }

            // Add the network provider in the graph.
            if (!graph.addFilter(provider.pointer(), L"Net Provider", _report)) {
                continue;
            }

            // Try to connect the network provider to the tuner.
            if (graph.connectFilters(provider.pointer(), tuners.filter(tuner_index).pointer(), _report)) {
                _output << margin << "  Successful connection" << std::endl;

                // Now, try to associate each tuning space.
                std::vector<ComPtr<::ITuningSpace>> spaces;
                getAllTuningSpaces(spaces);
                for (size_t i = 0; i < spaces.size(); ++i) {
                    const UString name(GetTuningSpaceFriendlyName(spaces[i].pointer(), _report));
                    ::HRESULT hr = iTuner->put_TuningSpace(spaces[i].pointer());
                    _output << margin << "  Tuning space \"" << name << "\": "
                            << (SUCCEEDED(hr) ? u"accepted" : ComMessage(hr)) << std::endl;
                }
            }

            // Remove the network provider from the graph.
            // Will be automatically disconnected if the connection succeeded.
            graph.removeFilter(provider.pointer(), _report);
        }
    }
}


//-----------------------------------------------------------------------------
// Test tuning spaces, same as runTest(TUNING_SPACES).
//-----------------------------------------------------------------------------

void ts::DirectShowTest::testTuningSpaces(const UString& margin)
{
    // Build an instance of all network providers.
    DirectShowFilterCategory filters(KSCATEGORY_BDA_NETWORK_PROVIDER, _report);

    // Loop on all network providers.
    for (size_t index = 0; index < filters.size(); ++index) {

        // Characteristics of this network provider.
        _output << std::endl << margin << "=== Testing \"" << filters.name(index) << "\"" << std::endl << std::endl;

        // Get tuner interface of this network provider.
        ComPtr<::ITuner> tuner;
        tuner.queryInterface(filters.filter(index).pointer(), ::IID_ITuner, _report);
        if (tuner.isNull()) {
            // No tuner interface, skip this network provider.
            _output << margin << "  No ITuner interface" << std::endl;
            continue;
        }

        // Get an all tuning spaces.
        std::vector<ComPtr<::ITuningSpace>> spaces;
        if (!getAllTuningSpaces(spaces)) {
            return;
        }

        // Build a list of compatible and incompatible tuning spaces.
        UStringList good;
        UStringList bad;

        // Loop on all tuning spaces.
        for (size_t i = 0; i < spaces.size(); ++i) {

            // Try to apply this tuning space to the network provider.
            ::HRESULT hr = tuner->put_TuningSpace(spaces[i].pointer());

            // Store either good or bad.
            const UString ts_name(GetTuningSpaceFriendlyName(spaces[i].pointer(), _report) + u" (" +
                                  GetTuningSpaceUniqueName(spaces[i].pointer(), _report) + u", " +
                                  GetTuningSpaceNetworkType(spaces[i].pointer(), _report) + u")");
            if (SUCCEEDED(hr)) {
                good.push_back(u"    " + ts_name);
            }
            else {
                bad.push_back(u"    " + ts_name + u": " + ComMessage(hr));
            }
        }

        // Display report.
        _output << margin << "  Compatible tuning spaces:" << std::endl;
        if (good.empty()) {
            _output << margin << "    None" << std::endl;
        }
        else {
            UString::Save(good, _output);
        }
        _output << std::endl << margin << "  Incompatible tuning spaces:" << std::endl;
        if (bad.empty()) {
            _output << margin << "    None" << std::endl;
        }
        else {
            UString::Save(bad, _output);
        }
    }

    _output << std::endl;
}


//-----------------------------------------------------------------------------
// Brief list of DirectShow devices.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::listDevices(const UString& margin)
{
    displayDevicesByCategory(KSCATEGORY_BDA_NETWORK_PROVIDER, u"Netwok providers", false, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_NETWORK_TUNER, u"Tuners", false, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_RECEIVER_COMPONENT, u"Receivers", false, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_TRANSPORT_INFORMATION, u"Transport information", false, margin);
    _output << std::endl;
}


//-----------------------------------------------------------------------------
// Enumerate DirectShow devices.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::enumerateDevices(const UString& margin)
{
    displayDevicesByCategory(KSCATEGORY_CAPTURE, u"CAPTURE", true, margin);
    displayDevicesByCategory(KSCATEGORY_SPLITTER, u"SPLITTER", true, margin);
    displayDevicesByCategory(KSCATEGORY_TVTUNER, u"TVTUNER", true, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_NETWORK_PROVIDER, u"BDA_NETWORK_PROVIDER", true, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_TRANSPORT_INFORMATION, u"BDA_TRANSPORT_INFORMATION", true, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_RECEIVER_COMPONENT, u"BDA_RECEIVER_COMPONENT", true, margin);
    displayDevicesByCategory(KSCATEGORY_BDA_NETWORK_TUNER, u"BDA_NETWORK_TUNER", true, margin);
    displayTuningSpaces(margin);
    _output << std::endl;
}


//-----------------------------------------------------------------------------
// Display all devices of the specified category
//-----------------------------------------------------------------------------

bool ts::DirectShowTest::displayDevicesByCategory(const ::GUID& category, const UString& name, bool details, const UString& margin)
{
    // Build an instance of all devices of this category.
    DirectShowFilterCategory filters(category, _report);

    if (details) {
        // Full display.
        _output << std::endl << margin << "=== Device category " << name << std::endl;

        // Loop on all enumerated devices.
        for (size_t index = 0; index < filters.size(); ++index) {

            // Display characteristics of this device filter
            _output << std::endl << margin << "device \"" << filters.name(index) << "\"" << std::endl;
            displayObject(filters.filter(index).pointer(), margin + u"  ");

            // List all pins on the filter
            // Create a pin enumerator
            ComPtr<::IEnumPins> enum_pins;
            ::HRESULT hr = filters.filter(index)->EnumPins(enum_pins.creator());
            if (!ComSuccess(hr, u"IBaseFilter::EnumPins", _report)) {
                return false;
            }
            if (enum_pins.isNull()) {
                // Not an expected result, probably no pin, not an error.
                continue;
            }

            // Loop on all pins
            ComPtr<::IPin> pin;
            while (enum_pins->Next(1, pin.creator(), NULL) == S_OK) {

                // Query direction of this pin
                ::PIN_DIRECTION dir;
                hr = pin->QueryDirection(&dir);
                if (!ComSuccess(hr, u"IPin::QueryDirection", _report)) {
                    return false;
                }

                // Get pin info
                ::PIN_INFO pin_info;
                hr = pin->QueryPinInfo(&pin_info);
                if (!ComSuccess(hr, u"IPin::QueryPinInfo", _report)) {
                    return false;
                }
                UString pin_name(ToString(pin_info.achName));
                pin_info.pFilter->Release();

                _output << std::endl << margin << "  - Pin \"" << pin_name << "\", direction: " << PinDirectionName(dir) << std::endl;
                displayObject(pin.pointer(), margin + u"    ");
            }
        }
    }
    else {
        // Short display, list only.
        _output << std::endl << margin << "=== " << name;
        if (filters.empty()) {
            _output << " (none)"<< std::endl ;
        }
        else {
            _output << " (" << filters.size() << " found)" << std::endl << std::endl;
        }

        // Loop on all enumerated devices.
        for (size_t index = 0; index < filters.size(); ++index) {
            _output << margin << index <<  ": \"" << filters.name(index) << "\"" << std::endl;
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// Get all tuning spaces.
//-----------------------------------------------------------------------------

bool ts::DirectShowTest::getAllTuningSpaces(std::vector<ComPtr<::ITuningSpace>>& spaces)
{
    spaces.clear();

    // Get an enumerator to all tuning spaces.
    ComPtr<::ITuningSpaceContainer> tsContainer;
    ComPtr<::IEnumTuningSpaces> tsEnum;
    if (!getAllTuningSpaces(tsContainer, tsEnum)) {
        return false;
    }

    // Loop on all tuning spaces.
    ts::ComPtr<::ITuningSpace> tspace;
    while (tsEnum->Next(1, tspace.creator(), NULL) == S_OK) {
        spaces.push_back(tspace);
    }
    return true;
}


//-----------------------------------------------------------------------------
// Get an enumerator for all tuning spaces.
//-----------------------------------------------------------------------------

bool ts::DirectShowTest::getAllTuningSpaces(ComPtr<::ITuningSpaceContainer>& tsContainer, ComPtr<::IEnumTuningSpaces>& tsEnum)
{
    // Create a Tuning Space Container.
    tsContainer.createInstance(::CLSID_SystemTuningSpaces, ::IID_ITuningSpaceContainer, _report);
    if (tsContainer.isNull()) {
        return false;
    }

    // Enumerate all tuning spaces.
    ::HRESULT hr = tsContainer->get_EnumTuningSpaces(tsEnum.creator());
    return ComSuccess(hr, u"ITuningSpaceContainer::get_EnumTuningSpaces", _report);
}


//-----------------------------------------------------------------------------
// Display all DirectShow tuning spaces.
//-----------------------------------------------------------------------------

bool ts::DirectShowTest::displayTuningSpaces(const UString& margin)
{
    ComPtr<::ITuningSpaceContainer> tsContainer;
    ComPtr<::IEnumTuningSpaces> tsEnum;

    _output << std::endl << margin << "=== Tuning spaces" << std::endl << std::endl;

    const bool ok = getAllTuningSpaces(tsContainer, tsEnum);
    if (ok) {
        displayEnumerateTuningSpaces(tsEnum.pointer(), margin + u"  ");
    }
    return ok;
}


//-----------------------------------------------------------------------------
// Show one property support through IKsPropertySet for a COM object.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayOneIKsPropertySet(::IKsPropertySet* ps, const ::GUID& psGuid, const char* psName, ::DWORD propId, const char* propName, const UString& margin)
{
    if (ps == 0) {
        return;
    }

    ::DWORD support = 0;

    if (SUCCEEDED(ps->QuerySupported(psGuid, propId, &support)) && support != 0) {
        _output << margin << propName << " (" << psName << ") :";
        if (support & KSPROPERTY_SUPPORT_GET) {
            _output << " get";
        }
        if (support & KSPROPERTY_SUPPORT_SET) {
            _output << " set";
        }
        _output << std::endl;
    }
}


//-----------------------------------------------------------------------------
// Show properties support through IKsPropertySet for a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayIKsPropertySet(::IUnknown* object, const UString& margin)
{
    if (object == 0) {
        return;
    }

    // Check if the filter supports IKsPropertySet
    ComPtr<::IKsPropertySet> propset;
    propset.queryInterface(object, IID_IKsPropertySet, NULLREP);
    if (propset.isNull()) {
        return;
    }
    _output << margin << u"IKsPropertySet properties support:" << std::endl;

    // Check some supported properties
#define _P_(ps,id) displayOneIKsPropertySet(propset.pointer(), KSPROPSETID_Bda##ps, #ps, KSPROPERTY_BDA_##id, #id, margin + u"  ")

    _P_(SignalStats, SIGNAL_STRENGTH);
    _P_(SignalStats, SIGNAL_QUALITY);
    _P_(SignalStats, SIGNAL_PRESENT);
    _P_(SignalStats, SIGNAL_LOCKED);
    _P_(SignalStats, SAMPLE_TIME);
    _P_(SignalStats, SIGNAL_LOCK_CAPS);
    _P_(SignalStats, SIGNAL_LOCK_TYPE);

    _P_(FrequencyFilter, RF_TUNER_FREQUENCY);
    _P_(FrequencyFilter, RF_TUNER_POLARITY);
    _P_(FrequencyFilter, RF_TUNER_RANGE);
    _P_(FrequencyFilter, RF_TUNER_TRANSPONDER);
    _P_(FrequencyFilter, RF_TUNER_BANDWIDTH);
    _P_(FrequencyFilter, RF_TUNER_FREQUENCY_MULTIPLIER);
    _P_(FrequencyFilter, RF_TUNER_CAPS);
    _P_(FrequencyFilter, RF_TUNER_SCAN_STATUS);
    _P_(FrequencyFilter, RF_TUNER_STANDARD);
    _P_(FrequencyFilter, RF_TUNER_STANDARD_MODE);

    _P_(DigitalDemodulator, MODULATION_TYPE);
    _P_(DigitalDemodulator, INNER_FEC_TYPE);
    _P_(DigitalDemodulator, INNER_FEC_RATE);
    _P_(DigitalDemodulator, OUTER_FEC_TYPE);
    _P_(DigitalDemodulator, OUTER_FEC_RATE);
    _P_(DigitalDemodulator, SYMBOL_RATE);
    _P_(DigitalDemodulator, SPECTRAL_INVERSION);
    _P_(DigitalDemodulator, GUARD_INTERVAL);
    _P_(DigitalDemodulator, TRANSMISSION_MODE);
    _P_(DigitalDemodulator, ROLL_OFF);
    _P_(DigitalDemodulator, PILOT);

    _P_(LNBInfo, LNB_LOF_LOW_BAND);
    _P_(LNBInfo, LNB_LOF_HIGH_BAND);
    _P_(LNBInfo, LNB_SWITCH_FREQUENCY);

#undef _P_
}


//-----------------------------------------------------------------------------
// Show one properties support through IKsControl for a COM object.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayOneIKsControl(::IKsControl* iks, const ::GUID& propSetGuid, const char* propSetName, ::ULONG propId, const char* propName, const UString& margin)
{
    if (iks == 0) {
        return;
    }

    ::KSPROPERTY prop;
    prop.Set = propSetGuid;
    prop.Id = propId;
    prop.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

    ::DWORD support = 0;
    ::ULONG retsize = 0;
    ::HRESULT hr = iks->KsProperty(&prop, sizeof(prop), &support, sizeof(support), &retsize);

    if (SUCCEEDED(hr) && support != 0) {
        _output << margin << propName << " (" << propSetName << ") :";
        if (support & KSPROPERTY_TYPE_GET) {
            _output << " get";
        }
        if (support & KSPROPERTY_TYPE_SET) {
            _output << " set";
        }
        _output << std::endl;
    }
}


//-----------------------------------------------------------------------------
// Show properties support through IKsControl for a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayIKsControl(::IUnknown* object, const UString& margin)
{
    if (object == 0) {
        return;
    }

    // Check if the filter supports IKsPropertySet
    ComPtr<::IKsControl> control;
    control.queryInterface(object, IID_IKsControl, NULLREP);
    if (control.isNull()) {
        return;
    }
    _output << margin << "IKsControl properties support:" << std::endl;

    // Check some supported properties
#define _P_(ps,id) displayOneIKsControl(control.pointer(), KSPROPSETID_Bda##ps, #ps, KSPROPERTY_BDA_##id, #id, margin + u"  ")

    _P_(SignalStats, SIGNAL_STRENGTH);
    _P_(SignalStats, SIGNAL_QUALITY);
    _P_(SignalStats, SIGNAL_PRESENT);
    _P_(SignalStats, SIGNAL_LOCKED);
    _P_(SignalStats, SAMPLE_TIME);
    _P_(SignalStats, SIGNAL_LOCK_CAPS);
    _P_(SignalStats, SIGNAL_LOCK_TYPE);

    _P_(FrequencyFilter, RF_TUNER_FREQUENCY);
    _P_(FrequencyFilter, RF_TUNER_POLARITY);
    _P_(FrequencyFilter, RF_TUNER_RANGE);
    _P_(FrequencyFilter, RF_TUNER_TRANSPONDER);
    _P_(FrequencyFilter, RF_TUNER_BANDWIDTH);
    _P_(FrequencyFilter, RF_TUNER_FREQUENCY_MULTIPLIER);
    _P_(FrequencyFilter, RF_TUNER_CAPS);
    _P_(FrequencyFilter, RF_TUNER_SCAN_STATUS);
    _P_(FrequencyFilter, RF_TUNER_STANDARD);
    _P_(FrequencyFilter, RF_TUNER_STANDARD_MODE);

    _P_(DigitalDemodulator, MODULATION_TYPE);
    _P_(DigitalDemodulator, INNER_FEC_TYPE);
    _P_(DigitalDemodulator, INNER_FEC_RATE);
    _P_(DigitalDemodulator, OUTER_FEC_TYPE);
    _P_(DigitalDemodulator, OUTER_FEC_RATE);
    _P_(DigitalDemodulator, SYMBOL_RATE);
    _P_(DigitalDemodulator, SPECTRAL_INVERSION);
    _P_(DigitalDemodulator, GUARD_INTERVAL);
    _P_(DigitalDemodulator, TRANSMISSION_MODE);
    _P_(DigitalDemodulator, ROLL_OFF);
    _P_(DigitalDemodulator, PILOT);

    _P_(LNBInfo, LNB_LOF_LOW_BAND);
    _P_(LNBInfo, LNB_LOF_HIGH_BAND);
    _P_(LNBInfo, LNB_SWITCH_FREQUENCY);

#undef _P_
}


//-----------------------------------------------------------------------------
// Show IKsTopologyInfo for a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayIKsTopologyInfo(::IUnknown* object, const UString& margin)
{
    if (object == 0) {
        return;
    }

    // Check if the filter supports IKsTopologyInfo
    ComPtr<::IKsTopologyInfo> topinfo;
    topinfo.queryInterface(object, IID_IKsTopologyInfo, NULLREP);
    if (topinfo.isNull()) {
        return;
    }
    _output << margin << "IKsTopologyInfo:" << std::endl;

    // List categories
    ::DWORD cat_count;
    ::HRESULT hr = topinfo->get_NumCategories(&cat_count);
    if (ComSuccess(hr, u"IKsTopologyInfo::get_NumCategories", _report)) {
        _output << margin << "  Categories:";
        if (cat_count <= 0) {
            _output << " none";
        }
        for (::DWORD cat = 0; cat < cat_count; cat++) {
            ::GUID category;
            hr = topinfo->get_Category(cat, &category);
            if (ComSuccess(hr, u"IKsTopologyInfo::get_Category", _report)) {
                _output << " " << NameGUID(category);
            }
        }
        _output << std::endl;
    }

    // List nodes
    ::DWORD node_count;
    hr = topinfo->get_NumNodes(&node_count);
    if (ComSuccess(hr, u"IKsTopologyInfo::get_NumNodes", _report)) {
        if (node_count <= 0) {
            _output << margin << "  No node found" << std::endl;
        }
        for (::DWORD n = 0; n < node_count; n++) {
            _output << margin << "  Node " << n;
            ::GUID node_type;
            hr = topinfo->get_NodeType(n, &node_type);
            if (ComSuccess(hr, u"IKsTopologyInfo::get_NodeType", _report)) {
                _output << ", type " << NameGUID(node_type);
            }
            static const ::DWORD MAX_NODE_NAME = 256;
            ::WCHAR name [MAX_NODE_NAME];
            ::DWORD name_size;
            hr = topinfo->get_NodeName(n, name, MAX_NODE_NAME, &name_size);
            if (ComSuccess(hr, u"IKsTopologyInfo::get_NodeName", NULLREP)) {
                _output << ", name \"" << ToString(name) << "\"";
            }
            _output << std::endl;
        }
    }
}


//-----------------------------------------------------------------------------
// Show IBDA_Topology for a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayBDATopology(::IUnknown* object, const UString& margin)
{
    if (object == 0) {
        return;
    }

    // Check if the filter supports IBDA_Topology
    ComPtr<::IBDA_Topology> topology;
    topology.queryInterface(object, ::IID_IBDA_Topology, NULLREP);
    if (topology.isNull()) {
        return;
    }
    _output << margin << "IBDA_Topology:" << std::endl;

    static const ::ULONG MAX_NODES = 64;
    ::ULONG count;
    ::HRESULT hr;

    // Get node descriptors
    ::BDANODE_DESCRIPTOR desc [MAX_NODES];
    count = MAX_NODES;
    hr = topology->GetNodeDescriptors(&count, MAX_NODES, desc);
    if (!ComSuccess(hr, u"IBDA_Topology::GetNodeDescriptors", _report)) {
        return;
    }
    _output << margin << "  Node descriptors:" << std::endl;
    for (::ULONG node = 0; node < count; node++) {
        _output << margin << "    type " << desc[node].ulBdaNodeType
                << ": function " << NameGUID(desc[node].guidFunction)
                << ", name " << NameGUID(desc[node].guidName)
                << std::endl;
    }

    // Get node types
    ::ULONG types [MAX_NODES];
    count = MAX_NODES;
    hr = topology->GetNodeTypes(&count, MAX_NODES, types);
    if (!ComSuccess(hr, u"IBDA_Topology::GetNodeTypes", _report)) {
        return;
    }
    for (::ULONG node = 0; node < count; node++) {
        _output << margin << "  Node type " << types[node] << ":" << std::endl;

        // List all interfaces for this node
        ::GUID interfaces [MAX_NODES];
        ::ULONG interfaces_count = MAX_NODES;
        hr = topology->GetNodeInterfaces(types[node], &interfaces_count, MAX_NODES, interfaces);
        if (ts::ComSuccess(hr, u"IBDA_Topology::GetNodeInterfaces", _report)) {
            for (::ULONG n = 0; n < interfaces_count; n++) {
                _output << margin << "    interface " << ts::NameGUID(interfaces[n]) << std::endl;
            }
        }

        // Get control node for this type
        ts::ComPtr<::IUnknown> cnode;
        hr = topology->GetControlNode(0, 1, types[node], cnode.creator());
        if (ts::ComSuccess(hr, u"IBDA_Topology::GetControlNode", _report)) {
            displayObject(cnode.pointer(), margin + u"    ");
        }
    }

    // Get pin types
    count = MAX_NODES;
    hr = topology->GetPinTypes(&count, MAX_NODES, types);
    if (!ts::ComSuccess(hr, u"IBDA_Topology::GetPinTypes", _report)) {
        return;
    }
    _output << margin << "  Pin types:";
    if (count <= 0) {
        _output << " none";
    }
    else {
        for (::ULONG n = 0; n < count; n++) {
            _output << " " << types[n];
        }
    }
    _output << std::endl;

    // Get template connections
    ::BDA_TEMPLATE_CONNECTION conn [MAX_NODES];
    count = MAX_NODES;
    hr = topology->GetTemplateConnections(&count, MAX_NODES, conn);
    if (!ts::ComSuccess(hr, u"IBDA_Topology::GetTemplateConnections", _report)) {
        return;
    }

    _output << margin << "  Template connections:" << std::endl;
    for (::ULONG n = 0; n < count; n++) {
        _output << margin << "    node type " << conn[n].FromNodeType
            << " / pin type " << conn[n].FromNodePinType
            << " -> node type " << conn[n].ToNodeType
            << " / pin type " << conn[n].ToNodePinType
            << std::endl;
    }
}


//-----------------------------------------------------------------------------
// Display all tuning spaces from an enumerator.
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayEnumerateTuningSpaces(::IEnumTuningSpaces* enum_tspace, const UString& margin)
{
    if (enum_tspace == 0) {
        return;
    }

    ts::ComPtr<::ITuningSpace> tspace;
    while (enum_tspace->Next(1, tspace.creator(), NULL) == S_OK) {
        const UString name(GetTuningSpaceDescription(tspace.pointer(), _report));
        if (!name.empty()) {
            _output << margin << "Tuning space " << name << std::endl;
        }
    }
}


//-----------------------------------------------------------------------------
// Show ITuner for a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayITuner(::IUnknown* object, const UString& margin)
{
    if (object == 0) {
        return;
    }

    // Check if the filter supports ITuner.
    ComPtr<::ITuner> tuner;
    tuner.queryInterface(object, ::IID_ITuner, NULLREP);
    if (tuner.isNull()) {
        return;
    }
    _output << margin << "ITuner:" << std::endl;

    // List tuning spaces.
    ComPtr<::IEnumTuningSpaces> enum_tspace;
    ::HRESULT hr = tuner->EnumTuningSpaces(enum_tspace.creator());
    if (ComSuccess(hr, u"cannot enumerate tuning spaces", _report)) {
        if (hr != S_OK) {
            _output << margin << "  No tuning space found" << std::endl;
        }
        else {
            displayEnumerateTuningSpaces(enum_tspace.pointer(), margin + u"  ");
        }
    }
}


//-----------------------------------------------------------------------------
// Show selected properties of a COM object
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayObject(::IUnknown* object, const UString& margin)
{
    _output << margin << "Some supported interfaces:" << std::endl;
    displayInterfaces(object, margin + u"  ");
    displayIKsPropertySet(object, margin);
    displayIKsControl(object , margin);
    displayIKsTopologyInfo(object, margin);
    displayBDATopology(object, margin);
    displayITuner(object, margin);
}


//-----------------------------------------------------------------------------
// List some known interfaces that an object may expose
//-----------------------------------------------------------------------------

void ts::DirectShowTest::displayInterfaces(::IUnknown* object, const UString& margin)
{
    struct Interface {
        const ::IID* iid;
        const char*  name;
    };

    // List of interfaces we test on the object:
    static const Interface interfaces[] = {
#define _I_(id) {&IID_##id, #id},
        _I_(IAMAnalogVideoDecoder)
        _I_(IAMAnalogVideoEncoder)
        _I_(IAMAudioInputMixer)
        _I_(IAMAudioRendererStats)
        _I_(IAMBufferNegotiation)
        _I_(IAMCameraControl)
        _I_(IAMCertifiedOutputProtection)
        _I_(IAMClockAdjust)
        _I_(IAMClockSlave)
        _I_(IAMCopyCaptureFileProgress)
        _I_(IAMCrossbar)
        _I_(IAMDecoderCaps)
        _I_(IAMDevMemoryAllocator)
        _I_(IAMDevMemoryControl)
        _I_(IAMDeviceRemoval)
        _I_(IAMDroppedFrames)
        _I_(IAMErrorLog)
        _I_(IAMExtDevice)
        _I_(IAMExtTransport)
        _I_(IAMFilterGraphCallback)
        _I_(IAMFilterMiscFlags)
        _I_(IAMGraphBuilderCallback)
        _I_(IAMGraphStreams)
        _I_(IAMLatency)
        _I_(IAMMediaStream)
        _I_(IAMMediaTypeSample)
        _I_(IAMMediaTypeStream)
        _I_(IAMMultiMediaStream)
        _I_(IAMOpenProgress)
        _I_(IAMOverlayFX)
        _I_(IAMPhysicalPinInfo)
        _I_(IAMPushSource)
        _I_(IAMResourceControl)
        _I_(IAMSetErrorLog)
        _I_(IAMStreamConfig)
        _I_(IAMStreamControl)
        _I_(IAMStreamSelect)
        _I_(IAMTVAudio)
        _I_(IAMTVAudioNotification)
        _I_(IAMTVTuner)
        _I_(IAMTimecodeDisplay)
        _I_(IAMTimecodeGenerator)
        _I_(IAMTimecodeReader)
        _I_(IAMTimeline)
        _I_(IAMTimelineComp)
        _I_(IAMTimelineEffect)
        _I_(IAMTimelineEffectable)
        _I_(IAMTimelineGroup)
        _I_(IAMTimelineObj)
        _I_(IAMTimelineSplittable)
        _I_(IAMTimelineSrc)
        _I_(IAMTimelineTrack)
        _I_(IAMTimelineTrans)
        _I_(IAMTimelineTransable)
        _I_(IAMTimelineVirtualTrack)
        _I_(IAMTuner)
        _I_(IAMTunerNotification)
        _I_(IAMVfwCaptureDialogs)
        _I_(IAMVfwCompressDialogs)
        _I_(IAMVideoAccelerator)
        _I_(IAMVideoAcceleratorNotify)
        _I_(IAMVideoCompression)
        _I_(IAMVideoControl)
        _I_(IAMVideoDecimationProperties)
        _I_(IAMVideoProcAmp)
        _I_(IAMWMBufferPass)
        _I_(IAMWMBufferPassCallback)
        _I_(IAMovieSetup)
        _I_(IAsyncReader)
        _I_(IATSCChannelTuneRequest)
        _I_(IATSCComponentType)
        _I_(IATSCLocator)
        _I_(IATSCLocator2)
        _I_(IATSCTuningSpace)
        _I_(IAttributeGet)
        _I_(IAttributeSet)
        _I_(IBDAComparable)
        _I_(IBDA_AutoDemodulate)
        _I_(IBDA_AutoDemodulateEx)
        _I_(IBDA_ConditionalAccess)
        _I_(IBDA_DRM)
        _I_(IBDA_DeviceControl)
        _I_(IBDA_DiagnosticProperties)
        _I_(IBDA_DigitalDemodulator)
        _I_(IBDA_DigitalDemodulator2)
        _I_(IBDA_DigitalDemodulator3)
        _I_(IBDA_DiseqCommand)
        _I_(IBDA_EasMessage)
        _I_(IBDA_EthernetFilter)
        _I_(IBDA_FrequencyFilter)
        _I_(IBDA_IPSinkControl)
        _I_(IBDA_IPSinkInfo)
        _I_(IBDA_IPV4Filter)
        _I_(IBDA_IPV6Filter)
        _I_(IBDA_LNBInfo)
        _I_(IBDA_NetworkProvider)
        _I_(IBDA_NullTransform)
        _I_(IBDA_PinControl)
        _I_(IBDA_SignalProperties)
        _I_(IBDA_SignalStatistics)
        _I_(IBDA_TIF_REGISTRATION)
        _I_(IBDA_Topology)
        _I_(IBDA_TransportStreamInfo)
        _I_(IBDA_VoidTransform)
        _I_(IBPCSatelliteTuner)
        _I_(IBaseFilter)
        _I_(ICaptureGraphBuilder)
        _I_(ICaptureGraphBuilder2)
        _I_(ICodecAPI)
        _I_(IConfigAviMux)
        _I_(IConfigInterleaving)
        _I_(ICreateDevEnum)
        _I_(IDDrawExclModeVideo)
        _I_(IDDrawExclModeVideoCallback)
        _I_(IDVEnc)
        _I_(IDVRGB219)
        _I_(IDVSplitter)
        _I_(IDecimateVideoImage)
        _I_(IDigitalCableLocator)
        _I_(IDigitalCableTuningSpace)
        _I_(IDigitalCableTuneRequest)
        _I_(IDigitalLocator)
        _I_(IDistributorNotify)
        _I_(IDrawVideoImage)
        _I_(IDVBCLocator)
        _I_(IDVBSLocator)
        _I_(IDVBSLocator2)
        _I_(IDVBSTuningSpace)
        _I_(IDVBTLocator)
        _I_(IDVBTLocator2)
        _I_(IDVBTuneRequest)
        _I_(IDVBTuningSpace)
        _I_(IDVBTuningSpace2)
        _I_(IDvbCableDeliverySystemDescriptor)
        _I_(IDvbFrequencyListDescriptor)
        _I_(IDvbLogicalChannelDescriptor)
        _I_(IDvbSatelliteDeliverySystemDescriptor)
        _I_(IDvbServiceDescriptor)
        _I_(IDvbSiParser)
        _I_(IDvbTerrestrialDeliverySystemDescriptor)
        _I_(IDvdCmd)
        _I_(IDvdControl)
        _I_(IDvdControl2)
        _I_(IDvdGraphBuilder)
        _I_(IDvdInfo)
        _I_(IDvdInfo2)
        _I_(IDvdState)
        _I_(IEncoderAPI)
        _I_(IEnumFilters)
        _I_(IEnumMediaTypes)
        _I_(IEnumPins)
        _I_(IEnumRegFilters)
        _I_(IEnumStreamIdMap)
        _I_(IEnumTuneRequests)
        _I_(IEnumTuningSpaces)
        _I_(IFileSinkFilter)
        _I_(IFileSinkFilter2)
        _I_(IFileSourceFilter)
        _I_(IFilterChain)
        _I_(IFilterGraph)
        _I_(IFilterGraph2)
        _I_(IFilterGraph3)
        _I_(IFilterMapper)
        _I_(IFilterMapper2)
        _I_(IFilterMapper3)
        _I_(IFrequencyMap)
        _I_(IGetCapabilitiesKey)
        _I_(IGraphBuilder)
        _I_(IGraphConfig)
        _I_(IGraphConfigCallback)
        _I_(IGraphVersion)
        _I_(IIPDVDec)
        _I_(IISDBSLocator)
        _I_(IKsControl)
        _I_(IKsDataTypeHandler)
        _I_(IKsInterfaceHandler)
        _I_(IKsPin)
        _I_(IKsPropertySet)
        _I_(IKsTopologyInfo)
        _I_(IMPEG2Component)
        _I_(IMPEG2ComponentType)
        _I_(IMPEG2PIDMap)
        _I_(IMPEG2StreamIdMap)
        _I_(IMPEG2TuneRequest)
        _I_(IMPEG2TuneRequestFactory)
        _I_(IMPEG2TuneRequestSupport)
        _I_(IMPEG2_TIF_CONTROL)
        _I_(IMediaEventSink)
        _I_(IMediaFilter)
        _I_(IMediaPropertyBag)
        _I_(IMediaSample)
        _I_(IMediaSample2)
        _I_(IMediaSample2Config)
        _I_(IMediaSeeking)
        _I_(IMemAllocator)
        _I_(IMemAllocatorCallbackTemp)
        _I_(IMemAllocatorNotifyCallbackTemp)
        _I_(IMemInputPin)
        _I_(IMpeg2Data)
        _I_(IMpeg2Demultiplexer)
        _I_(IMpeg2Stream)
        _I_(IMpeg2TableFilter)
        _I_(IOverlay)
        _I_(IOverlayNotify)
        _I_(IOverlayNotify2)
        _I_(IPersistMediaPropertyBag)
        _I_(IPin)
        _I_(IPinConnection)
        _I_(IPinFlowControl)
        _I_(IQualityControl)
        _I_(IReferenceClock)
        _I_(IReferenceClock2)
        _I_(IReferenceClockTimerControl)
        _I_(IRegisterServiceProvider)
        _I_(IRegisterTuner)
        _I_(IResourceConsumer)
        _I_(IResourceManager)
        _I_(IScanningTuner)
        _I_(IScanningTunerEx)
        _I_(ISeekingPassThru)
        _I_(ISelector)
        _I_(IStreamBuilder)
        _I_(ITuneRequest)
        _I_(ITuneRequestInfo)
        _I_(ITuner)
        _I_(ITunerCap)
        _I_(ITuningSpace)
        _I_(ITuningSpaceContainer)
        _I_(ITuningSpaces)
        _I_(IVMRAspectRatioControl)
        _I_(IVMRDeinterlaceControl)
        _I_(IVMRFilterConfig)
        _I_(IVMRImageCompositor)
        _I_(IVMRImagePresenter)
        _I_(IVMRImagePresenterConfig)
        _I_(IVMRImagePresenterExclModeConfig)
        _I_(IVMRMixerBitmap)
        _I_(IVMRMixerControl)
        _I_(IVMRMonitorConfig)
        _I_(IVMRSurface)
        _I_(IVMRSurfaceAllocator)
        _I_(IVMRSurfaceAllocatorNotify)
        _I_(IVMRVideoStreamControl)
        _I_(IVMRWindowlessControl)
        _I_(IVPManager)
        _I_(IVideoEncoder)
        _I_(IVideoFrameStep)
#undef _I_
        {0, 0}
    };

    for (const Interface* p = interfaces; p->iid != 0; ++p) {
        if (ComExpose(object, *(p->iid))) {
            _output << margin << "interface " << p->name << std::endl;
        }
    }
}