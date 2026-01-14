//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastMulticastSession.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::MulticastSession::clear()
{
    service_identifier.clear();
    content_playback_availability_offset = cn::milliseconds::zero();
    manifest_locators.clear();
    reporting_locators.clear();
    transport_sessions.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::MulticastSession::parseXML(const xml::Element* element, bool strict)
{
    if (element == nullptr) {
        return false;
    }

    bool ok = element->getAttribute(service_identifier, u"serviceIdentifier", false) &&
              element->getISODurationAttribute(content_playback_availability_offset, u"contentPlaybackAvailabilityOffset", false, strict);

    for (auto& pml : element->children(u"PresentationManifestLocator", &ok, strict ? 1 : 0)) {
        auto& ml(manifest_locators.emplace_back());
        ok = pml.getText(ml.uri, true) &&
             pml.getAttribute(ml.manifest_id, u"manifestId", strict) &&
             pml.getAttribute(ml.content_type, u"contentType", strict) &&
             pml.getAttribute(ml.transport_object_uri, u"transportObjectURI", false) &&
             pml.getAttribute(ml.content_playback_path_pattern, u"contentPlaybackPathPattern", false);
    }

    for (auto& e1 : element->children(u"MulticastGatewaySessionReporting", &ok, 0, 1)) {
        for (auto& e2 : e1.children(u"ReportingLocator", &ok, strict ? 1 : 0)) {
            ok = reporting_locators.emplace_back().parseXML(&e2, strict);
        }
    }

    for (auto& mts : element->children(u"MulticastTransportSession", &ok)) {
        auto& sess(transport_sessions.emplace_back());
        ok = sess.parseXML(&mts, strict) &&
             mts.getAttribute(sess.id, u"id") &&
             mts.getAttribute(sess.content_ingest_method, u"contentIngestMethod") &&
             mts.getAttribute(sess.transmission_mode, u"transmissionMode");
    }

    return ok;
}
