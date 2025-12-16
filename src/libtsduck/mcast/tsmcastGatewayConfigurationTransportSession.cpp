//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastGatewayConfigurationTransportSession.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::GatewayConfigurationTransportSession::clear()
{
    service_class.clear();
    transport_security.clear();
    tags.clear();
    protocol.clear();
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

bool ts::mcast::GatewayConfigurationTransportSession::parseXML(const xml::Element* element, bool strict)
{
    if (element == nullptr) {
        return false;
    }

    const xml::Element* e = nullptr;
    bool ok = element->getAttribute(service_class, u"serviceClass", false) &&
              element->getAttribute(transport_security, u"transportSecurity", false, u"none") &&
              ((e = element->findFirstChild(u"BitRate", !strict)) != nullptr || !strict) &&
              protocol.parseXML(element, strict);

    if (ok && e != nullptr) {
        ok = e->getIntAttribute(bitrate_average, u"average", false) &&
             e->getIntAttribute(bitrate_maximum, u"maximum", strict);
    }

    if (ok) {
        // The attribute tags contains a space-separated list of URL's.
        element->attribute(u"tags", true).value().split(tags, u' ', true, true);
    }

    if (ok && (e = element->findFirstChild(u"UnicastRepairParameters", true))) {
        ok = e->getAttribute(repair_obj_base_uri, u"transportObjectBaseURI", false) &&
             e->getChronoAttribute(repair_recv_timeout, u"transportObjectReceptionTimeout", strict) &&
             e->getChronoAttribute(repair_fixed_backoff, u"fixedBackOffPeriod", false) &&
             e->getChronoAttribute(repair_rand_backoff, u"randomBackOffPeriod", false);
        for (const xml::Element* bu = e->findFirstChild(u"BaseURL", true); ok && bu != nullptr; bu = bu->findNextSibling(true)) {
            repair_base_url.emplace_back();
            ok = bu->getText(repair_base_url.back().uri, strict) &&
                 bu->getIntAttribute(repair_base_url.back().relative_weight, u"relativeWeight", false, 1);
        }
    }

    if (ok && (e = element->findFirstChild(u"ObjectCarousel", true))) {
        ok = e->getIntChild(carousel_content_size, u"aggregateContentSize", false) &&
             e->getIntChild(carousel_transport_size, u"aggregateTransportSize", false);
        for (const xml::Element* e1 = e->findFirstChild(u"PresentationManifests", true); ok && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            carousel_manifests.emplace_back();
            ok = carousel_manifests.back().parseXML(e1, strict);
        }
        for (const xml::Element* e1 = e->findFirstChild(u"InitSegments", true); ok && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            carousel_segment.emplace_back();
            ok = carousel_segment.back().parseXML(e1, strict);
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
        ok = endpoints.back().parseXML(e, strict);
    }

    for (e = element->findFirstChild(u"GatewayConfigurationMacro", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        UString key, value;
        ok = e->getAttribute(key, u"key", true) && e->getText(value, true);
        macros.insert(std::make_pair(key, value));
    }

    for (e = element->findFirstChild(u"ForwardErrorCorrectionParameters", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        fec.emplace_back();
        ok = e->getTextChild(fec.back().scheme_identifier, u"SchemeIdentifier", true, strict) &&
             e->getIntChild(fec.back().overhead_percentage, u"OverheadPercentage", strict);
        for (const xml::Element* ep = e->findFirstChild(u"EndpointAddress", true); ok && ep != nullptr; ep = ep->findNextSibling(true)) {
            fec.back().endpoints.emplace_back();
            ok = fec.back().endpoints.back().parseXML(e, strict);
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Entry of <PresentationManifests> or <InitSegments> in <ObjectCarousel>.
//----------------------------------------------------------------------------

bool ts::mcast::GatewayConfigurationTransportSession::ReferencingCarouselMediaPresentationResourceType::parseXML(const xml::Element* e, bool strict)
{
    return e->getBoolAttribute(compression_preferred, u"compressionPreferred") &&
           e->getAttribute(target_acquisition_latency, u"targetAcquisitionLatency") &&
           e->getAttribute(service_id_ref, u"serviceIdRef") &&
           e->getAttribute(transport_session_id_ref, u"transportSessionIdRef");
}
