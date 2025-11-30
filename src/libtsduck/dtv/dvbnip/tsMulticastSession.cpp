//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMulticastSession.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::MulticastSession::clear()
{
    service_identifier.clear();
    transport_sessions.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::MulticastSession::parseXML(const xml::Element* element)
{
    if (element == nullptr) {
        return false;
    }

    bool ok = element->getAttribute(service_identifier, u"serviceIdentifier", false);

    for (const xml::Element* e = element->findFirstChild(u"MulticastTransportSession", true); ok && e != nullptr; e = e->findNextSibling(true)) {
        transport_sessions.emplace_back();
        auto& sess(transport_sessions.back());
        ok = e->getAttribute(sess.id, u"id") &&
             e->getAttribute(sess.service_class, u"serviceClass") &&
             e->getAttribute(sess.content_ingest_method, u"contentIngestMethod") &&
             e->getAttribute(sess.transmission_mode, u"transmissionMode") &&
             e->getAttribute(sess.transport_security, u"transportSecurity") &&
             sess.protocol.parseXML(e);

        for (const xml::Element* ep = e->findFirstChild(u"EndpointAddress"); ok && ep != nullptr; ep = ep->findNextSibling(true)) {
            sess.endpoints.emplace_back();
            ok = sess.endpoints.back().parseXML(ep);
        }
    }

    return ok;
}
