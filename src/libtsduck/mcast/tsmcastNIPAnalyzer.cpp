//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPAnalyzer.h"
#include "tsmcastNIPActualCarrierInformation.h"
#include "tsmcast.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::mcast::NIPAnalyzer::NIPAnalyzer(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Reset the analysis.
//----------------------------------------------------------------------------

bool ts::mcast::NIPAnalyzer::reset(const FluteDemuxArgs& args)
{
    bool ok = _flute_demux.reset(args);
    _session_filter.clear();
    _service_lists.clear();
    _services.clear();

    // Filter the DVB-NIP announcement channel (IPv4 and IPv6).
    static const FluteSessionId announce4(IPAddress(), NIPSignallingAddress4(), NIP_SIGNALLING_TSI);
    static const FluteSessionId announce6(IPAddress(), NIPSignallingAddress6(), NIP_SIGNALLING_TSI);
    addSession(announce4);
    addSession(announce6);

    return ok;
}


//----------------------------------------------------------------------------
// Add a FLUTE session in the DVB-NIP analyzer.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::addProtocolSession(const TransportProtocol& protocol, const FluteSessionId& session)
{
    // We currently support FLUTE only.
    if (protocol.protocol == FT_FLUTE) {
        addSession(session);
    }
    else {
        _report.warning(u"ignoring session %s, unsupported protocol %s", protocol.protocol_identifier);
    }
}

void ts::mcast::NIPAnalyzer::addSession(const FluteSessionId& session)
{
    if (!_session_filter.contains(session)) {
        _report.verbose(u"adding session %s", session);
        _session_filter.insert(session);
    }
}


//----------------------------------------------------------------------------
// Check if a UDP packet or FLUTE file is part of a filtered session.
//----------------------------------------------------------------------------

bool ts::mcast::NIPAnalyzer::isFiltered(const IPAddress& source, const IPSocketAddress& destination) const
{
    for (const auto& it : _session_filter) {
        if (it.source.match(source) && it.destination.match(destination)) {
            return true;
        }
    }
    return false;
}

bool ts::mcast::NIPAnalyzer::isFiltered(const FluteSessionId& session) const
{
    for (const auto& it : _session_filter) {
        if (it.match(session)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Process a FLUTE file.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::handleFluteFile(const FluteFile& file)
{
    // Filter out files from non-filtered sessions.
    if (!isFiltered(file.sessionId())) {
        _report.debug(u"ignoring %s from %s", file.name(), file.sessionId());
        return;
    }

    // Is this a file from the DVB-NIP announcement channel?
    const bool is_announce = file.sessionId().nipAnnouncementChannel();

    if (is_announce && file.name().similar(u"urn:dvb:metadata:nativeip:ServiceInformationFile")) {
        // Process a SIF.
        const ServiceInformationFile sif(_report, file);
        if (sif.isValid()) {
            processSIF(sif);
        }
    }
    else if (is_announce && file.name().similar(u"urn:dvb:metadata:nativeip:dvb-i-slep")) {
        // Process a service list entry points.
        const ServiceListEntryPoints slep(_report, file);
        if (slep.isValid()) {
            processSLEP(slep);
        }
    }
    else if (file.type().similar(u"application/xml+dvb-mabr-session-configuration")) {
        // Process gateway configurations to find other sessions.
        const GatewayConfiguration mgc(_report, file);
        if (mgc.isValid()) {
            processGatewayConfiguration(mgc);
        }
    }
    else if (file.type().similar(u"application/vnd.dvb.dvbisl+xml")) {
        // Process service lists.
        const ServiceList slist(_report, file);
        if (slist.isValid()) {
            processServiceList(slist);
        }
    }
}


//----------------------------------------------------------------------------
// Process a bootstrap or multicast gateway configuration.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processGatewayConfiguration(const GatewayConfiguration& mgc)
{
    // Add all transport sessions in the session filter.
    for (const auto& sess : mgc.transport_sessions) {
        for (const auto& id : sess.endpoints) {
            addProtocolSession(sess.protocol, id);
        }
    }

    for (const auto& sess1 : mgc.multicast_sessions) {
        for (const auto& sess2 : sess1.transport_sessions) {
            for (const auto& id : sess2.endpoints) {
                addProtocolSession(sess2.protocol, id);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a Service Information File (SIF).
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processSIF(const ServiceInformationFile& sif)
{
    // Register all NIP actual carrier information.
    // Typically used by a subclass, if necessary.
    NIPActualCarrierInformation naci;
    naci.valid = true;
    naci.stream_provider_name = sif.provider_name;
    for (const auto& st : sif.streams) {
        naci.stream_id = st.stream_id;
        handleFluteNACI(naci);
    }
}


//----------------------------------------------------------------------------
// Process a Service List Entry Points (SLEP).
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processSLEP(const ServiceListEntryPoints& slep)
{
    // Grab all service lists.
    for (const auto& prov : slep.providers) {
        for (const auto& l1 : prov.lists) {
            for (const auto& l2 : l1.lists) {
                if (l2.type.contains(u"xml", CASE_INSENSITIVE)) {
                    auto& slc(_service_lists[l2.uri]);
                    slc.list_name = l1.name;
                    slc.provider_name = prov.provider.name;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Process a Service List.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processServiceList(const ServiceList& slist)
{
    // Report a verbose message if not yet registered from a service list entry point.
    if (!_service_lists.contains(slist.name())) {
        _report.verbose(u"unannounced service list %s on %s", slist.name(), slist.sessionId());
    }

    // Service list global properties.
    auto& slc(_service_lists[slist.name()]);
    slc.session_id = slist.sessionId();
    slc.list_name = slist.list_name;
    slc.provider_name = slist.provider_name;

    // Set of unique ids of new services.
    std::set<UString> new_services;

    // Process each service.
    for (const auto& it1 : slist.services) {
        if (!_services.contains(it1.unique_id)) {
            new_services.insert(it1.unique_id);
        }
        auto& serv(_services[it1.unique_id]);
        serv.service_name = it1.service_name;
        serv.provider_name = it1.provider_name;
        serv.service_type = it1.service_type;
        for (const auto& it2 : it1.instances) {
            auto& inst(serv.instances[it2.media_params]);
            inst.instance_priority = it2.priority;
            inst.media_type = it2.media_params_type;
        }
    }

    // Assign logical channel numbers.
    for (const auto& it1 : slist.lcn_tables) {
        for (const auto& it2 : it1.lcns) {
            auto& serv(_services[it2.service_ref]);
            serv.channel_number = it2.channel_number;
            serv.selectable = it2.selectable;
            serv.visible = it2.visible;
        }
    }

    // Notify new services, when the descriptions are complete.
    for (const auto& it : new_services) {
        processNewService(_services[it]);
    }
}


//----------------------------------------------------------------------------
// This virtual method is invoked for each new service.
//----------------------------------------------------------------------------

void ts::mcast::NIPAnalyzer::processNewService(const ServiceContext& service)
{
    // Nothing to do by default, let subclasses use this.
}
