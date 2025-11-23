//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMulticastGatewayConfigurationTransportSession.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::MulticastGatewayConfigurationTransportSession::clear()
{
    service_class.clear();
    transport_security.clear();
    tags.clear();
    trans_proto_id.clear();
    trans_proto_version = 0;
    bitrate_average = bitrate_maximum = 0;
    repair_base_url.clear();
    repair_obj_base_uri.clear();
    repair_recv_timeout = repair_fixed_backoff = repair_rand_backoff = cn::milliseconds::zero();
    endpoints.clear();
    macros.clear();
    fec.clear();
    carousel_transport_size = carousel_content_size = 0;
    resource_locator.clear();
    carousel_manifests.clear();
    carousel_segment.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::MulticastGatewayConfigurationTransportSession::parseXML(const xml::Element* element)
{
    if (element == nullptr) {
        return false;
    }

    const xml::Element* e = nullptr;
    bool ok =
        element->getAttribute(service_class, u"serviceClass", false) &&
        element->getAttribute(transport_security, u"transportSecurity", false, u"none") &&
        (e = element->findFirstChild(u"BitRate")) != nullptr &&
        e->getIntAttribute(bitrate_average, u"average", false) &&
        e->getIntAttribute(bitrate_maximum, u"maximum", true) &&
        (e = element->findFirstChild(u"TransportProtocol")) != nullptr &&
        e->getAttribute(trans_proto_id, u"protocolIdentifier", true) &&
        e->getIntAttribute(trans_proto_version, u"protocolVersion", true);

    if (ok) {
        // The attribute tags contains a space-separated list of URL's.
        element->attribute(u"tags", true).value().split(tags, u' ', true, true);
    }

    if (ok && (e = element->findFirstChild(u"UnicastRepairParameters", true))) {
        ok = e->getAttribute(repair_obj_base_uri, u"transportObjectBaseURI", false) &&
             e->getChronoAttribute(repair_recv_timeout, u"transportObjectReceptionTimeout", true) &&
             e->getChronoAttribute(repair_fixed_backoff, u"fixedBackOffPeriod", false) &&
             e->getChronoAttribute(repair_rand_backoff, u"randomBackOffPeriod", false);
        for (const xml::Element* bu = e->findFirstChild(u"BaseURL", true); ok && bu != nullptr; bu = bu->findNextSibling(true)) {
            repair_base_url.emplace_back();
            ok = bu->getText(repair_base_url.back().uri, true) &&
                 bu->getIntAttribute(repair_base_url.back().relative_weight, u"relativeWeight", false, 1);
        }
    }

    if (ok && (e = element->findFirstChild(u"ObjectCarousel", true))) {
        ok = e->getIntChild(carousel_content_size, u"aggregateContentSize", false) &&
             e->getIntChild(carousel_transport_size, u"aggregateTransportSize", false);
        for (const xml::Element* e1 = e->findFirstChild(u"PresentationManifests", true); ok && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            carousel_manifests.emplace_back();
            ok = carousel_manifests.back().parseXML(e1);
        }
        for (const xml::Element* e1 = e->findFirstChild(u"InitSegments", true); ok && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            carousel_segment.emplace_back();
            ok = carousel_segment.back().parseXML(e1);
        }
        for (const xml::Element* e1 = e->findFirstChild(u"ResourceLocator", true); ok && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            resource_locator.emplace_back();
            ok = e1->getText(resource_locator.back().uri, true) &&
                 e1->getBoolAttribute(resource_locator.back().compression_preferred, u"compressionPreferred") &&
                 e1->getAttribute(resource_locator.back().target_acquisition_latency, u"targetAcquisitionLatency") &&
                e1->getAttribute(resource_locator.back().revalidation_period, u"revalidationPeriod");
        }
    }

    for (e = element->findFirstChild(u"EndpointAddress", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        endpoints.emplace_back();
        ok = endpoints.back().parseXML(e);
    }

    for (e = element->findFirstChild(u"MulticastGatewayConfigurationMacro", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        UString key, value;
        ok = e->getAttribute(key, u"key", true) && e->getText(value, true);
        macros.insert(std::make_pair(key, value));
    }

    for (e = element->findFirstChild(u"ForwardErrorCorrectionParameters", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        fec.emplace_back();
        ok = e->getTextChild(fec.back().scheme_identifier, u"SchemeIdentifier", true, true) &&
             e->getIntChild(fec.back().overhead_percentage, u"OverheadPercentage", true);
        for (const xml::Element* ep = e->findFirstChild(u"EndpointAddress", true); ok && ep != nullptr; ep = ep->findNextSibling(true)) {
            fec.back().endpoints.emplace_back();
            ok = fec.back().endpoints.back().parseXML(e);
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Display the content of this structure.
//----------------------------------------------------------------------------

std::ostream& ts::MulticastGatewayConfigurationTransportSession::display(std::ostream& out, const UString& margin, int level) const
{
    out << margin << "serviceClass: " << service_class << std::endl
        << margin << "transportSecurity: " << transport_security << std::endl
        << margin << "protocolIdentifier: " << trans_proto_id << std::endl
        << margin << "protocolVersion: " << trans_proto_version << std::endl
        << margin << "BitRate average: " << bitrate_average << std::endl
        << margin << "BitRate maximum: " << bitrate_maximum << std::endl
        << margin << "tag: " << tags.size() << " values" << std::endl;
    for (const auto& it : tags) {
        out << margin << "  " << it << std::endl;
    }

    out << margin << "UnicastRepairParameters transportObjectBaseURI: " << repair_obj_base_uri << std::endl
        << margin << "UnicastRepairParameters transportObjectReceptionTimeout: " << repair_recv_timeout << std::endl
        << margin << "UnicastRepairParameters fixedBackOffPeriod: " << repair_fixed_backoff << std::endl
        << margin << "UnicastRepairParameters randomBackOffPeriod: " << repair_rand_backoff << std::endl
        << margin << "UnicastRepairParameters BaseURL: " << repair_base_url.size() << " elements" << std::endl;
    for (const auto& it : repair_base_url) {
        out << margin << "  " << it.uri << " (weigth: " << it.relative_weight << ")" << std::endl;
    }

    out << margin << "EndpointAddress: " << endpoints.size() << " elements" << std::endl;
    for (const auto& it : endpoints) {
        out << margin << "  " << it << std::endl;
    }

    out << margin << "MulticastGatewayConfigurationMacro: " << macros.size() << " elements" << std::endl;
    for (const auto& it : macros) {
        out << margin << "  " << it.first << " = " << it.second << std::endl;
    }

    out << margin << "ForwardErrorCorrectionParameters: " << fec.size() << " elements" << std::endl;
    for (const auto& it : fec) {
        out << margin << "  SchemeIdentifier: " << it.scheme_identifier << std::endl
            << margin << "  OverheadPercentage: " << it.overhead_percentage << std::endl
            << margin << "  EndpointAddress: " << it.endpoints.size() << " elements" << std::endl;
        for (const auto& it2 : it.endpoints) {
            out << margin << "    " << it2 << std::endl;
        }
    }

    out << margin << "ObjectCarousel aggregateTransportSize: " << carousel_transport_size << std::endl
        << margin << "ObjectCarousel aggregateContentSize: " << carousel_content_size << std::endl
        << margin << "ObjectCarousel ResourceLocator: " << resource_locator.size() << " elements" << std::endl;
    for (const auto& it : resource_locator) {
        out << margin << "- URI: " << it.uri << std::endl
            << margin << "  compressionPreferred: " << UString::TrueFalse(it.compression_preferred) << std::endl
            << margin << "  targetAcquisitionLatency: " << it.target_acquisition_latency << std::endl
            << margin << "  revalidationPeriod: " << it.revalidation_period << std::endl;
    }

    out << margin << "ObjectCarousel PresentationManifests: " << carousel_manifests.size() << " elements" << std::endl;
    for (const auto& it : carousel_manifests) {
        it.display(out, margin);
    }

    out << margin << "ObjectCarousel InitSegments: " << carousel_segment.size() << " elements" << std::endl;
    for (const auto& it : carousel_segment) {
        it.display(out, margin);
    }
    return out;
}


//----------------------------------------------------------------------------
// Entry of <PresentationManifests> or <InitSegments> in <ObjectCarousel>.
//----------------------------------------------------------------------------

bool ts::MulticastGatewayConfigurationTransportSession::ReferencingCarouselMediaPresentationResourceType::parseXML(const xml::Element* e)
{
    return e->getBoolAttribute(compression_preferred, u"compressionPreferred") &&
           e->getAttribute(target_acquisition_latency, u"targetAcquisitionLatency") &&
           e->getAttribute(service_id_ref, u"serviceIdRef") &&
           e->getAttribute(transport_session_id_ref, u"transportSessionIdRef");
}

std::ostream& ts::MulticastGatewayConfigurationTransportSession::ReferencingCarouselMediaPresentationResourceType::display(std::ostream& out, const UString& margin) const
{
    out << margin << "- compressionPreferred: " << UString::TrueFalse(compression_preferred) << std::endl
        << margin << "  targetAcquisitionLatency: " << target_acquisition_latency << std::endl
        << margin << "  serviceIdRef: " << service_id_ref << std::endl
        << margin << "  transportSessionIdRef: " << transport_session_id_ref << std::endl;
    return out;
}
