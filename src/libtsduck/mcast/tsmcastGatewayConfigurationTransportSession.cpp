//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    BaseMulticastTransportSession::clear();
    tags.clear();
    macros.clear();
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

    bool ok = BaseMulticastTransportSession::parseXML(element, strict);

    if (ok) {
        // The attribute tags contains a space-separated list of URL's.
        element->attribute(u"tags").value().split(tags, u' ', true, true);
    }

    for (auto& e : element->children(u"ObjectCarousel", &ok, 0, 1)) {
        ok = e.getIntChild(carousel_content_size, u"aggregateContentSize", false) &&
             e.getIntChild(carousel_transport_size, u"aggregateTransportSize", false);
        for (auto& e1 : e.children(u"PresentationManifests", &ok)) {
            carousel_manifests.emplace_back();
            ok = carousel_manifests.back().parseXML(&e1, strict);
        }
        for (auto& e1 : e.children(u"InitSegments", &ok)) {
            carousel_segment.emplace_back();
            ok = carousel_segment.back().parseXML(&e1, strict);
        }
        for (auto& e1 : e.children(u"ResourceLocator", &ok)) {
            resource_locator.emplace_back();
            ok = e1.getText(resource_locator.back().uri, true) &&
                 e1.getBoolAttribute(resource_locator.back().compression_preferred, u"compressionPreferred") &&
                 e1.getAttribute(resource_locator.back().target_acquisition_latency, u"targetAcquisitionLatency") &&
                 e1.getAttribute(resource_locator.back().revalidation_period, u"revalidationPeriod");
        }
    }

    for (auto& e1 : element->children(u"GatewayConfigurationMacro", &ok)) {
        UString key, value;
        ok = e1.getAttribute(key, u"key", true) && e1.getText(value, true);
        macros.insert(std::make_pair(key, value));
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
