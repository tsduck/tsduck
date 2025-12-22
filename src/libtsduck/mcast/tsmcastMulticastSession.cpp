//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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

    bool ok = element->getAttribute(service_identifier, u"serviceIdentifier", false);

    for (auto& mts : element->children(u"MulticastTransportSession", &ok)) {
        transport_sessions.emplace_back();
        auto& sess(transport_sessions.back());
        ok = mts.getAttribute(sess.id, u"id") &&
             mts.getAttribute(sess.service_class, u"serviceClass") &&
             mts.getAttribute(sess.content_ingest_method, u"contentIngestMethod") &&
             mts.getAttribute(sess.transmission_mode, u"transmissionMode") &&
             mts.getAttribute(sess.transport_security, u"transportSecurity") &&
             sess.protocol.parseXML(&mts, strict);

        for (auto& ep : mts.children(u"EndpointAddress", &ok, strict ? 1 : 0)) {
            sess.endpoints.emplace_back();
            ok = sess.endpoints.back().parseXML(&ep, strict);
        }
    }

    return ok;
}
