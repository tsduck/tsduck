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
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::MulticastSession::~MulticastSession()
{
}


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
        const xml::Element* proto = nullptr;
        ok = e->getAttribute(sess.id, u"id") &&
             e->getAttribute(sess.service_class, u"serviceClass") &&
             e->getAttribute(sess.content_ingest_method, u"contentIngestMethod") &&
             e->getAttribute(sess.transmission_mode, u"transmissionMode") &&
             e->getAttribute(sess.transport_security, u"transportSecurity") &&
             (proto = e->findFirstChild(u"TransportProtocol")) != nullptr &&
             proto->getAttribute(sess.protocol_identifier, u"protocolIdentifier", true) &&
             proto->getAttribute(sess.protocol_version, u"protocolVersion");

        for (const xml::Element* ep = e->findFirstChild(u"EndpointAddress"); ok && ep != nullptr; ep = ep->findNextSibling(true)) {
            sess.endpoints.emplace_back();
            ok = sess.endpoints.back().parseXML(ep);
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Display the content of this structure.
//----------------------------------------------------------------------------

std::ostream& ts::MulticastSession::display(std::ostream& out, const UString& margin, int level) const
{
    out << margin << "serviceIdentifier: " << service_identifier << ", " << transport_sessions.size() << " transport sessions" << std::endl;
    int count = 0;
    for (const auto& sess : transport_sessions) {
        out << margin << "- Transport session #" << (++count) << std::endl
            << margin << "  id: " << sess.id << ", serviceClass: " << sess.service_class << std::endl
            << margin << "  contentIngestMethod: " << sess.content_ingest_method << std::endl
            << margin << "  transmissionMode: " << sess.transmission_mode << ", transportSecurity: " << sess.transport_security << std::endl
            << margin << "  protocolIdentifier: " << sess.protocol_identifier << ", protocolVersion: " << sess.protocol_version << std::endl
            << margin << "  EndpointAddress: " << sess.endpoints.size() << " elements" << std::endl;
        for (const auto& ep : sess.endpoints) {
            out << margin << "    " << ep << std::endl;
        }
    }
    return out;
}
