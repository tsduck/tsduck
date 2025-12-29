//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastTransportProtocol.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::TransportProtocol::clear()
{
    protocol = FT_UNKNOWN;
    version = 0;
    protocol_identifier.clear();
    protocol_version.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::TransportProtocol::parseXML(const xml::Element* element, bool strict, const UString& child_name)
{
    clear();
    const xml::Element* e = element;
    if (element == nullptr || (!child_name.empty() && (e = element->findFirstChild(child_name, true)) == nullptr)) {
        return false;
    }

    bool ok = e->getAttribute(protocol_identifier, u"protocolIdentifier", strict) &&
              e->getAttribute(protocol_version, u"protocolVersion");

    if (ok) {
        // The field version is documented as string but usually contains an integer.
        // So try to interpret it as an integer but don't trigger an error if it fails.
        protocol_version.toInteger(version);
        // The protocol name may have a namespace or not.
        if (protocol_identifier.ends_with(u"FLUTE", CASE_INSENSITIVE)) {
            protocol = FT_FLUTE;
        }
        else if (protocol_identifier.ends_with(u"ROUTE", CASE_INSENSITIVE)) {
            protocol = FT_ROUTE;
        }
        else {
            protocol = FT_UNKNOWN;
        }
    }

    return ok;
}
