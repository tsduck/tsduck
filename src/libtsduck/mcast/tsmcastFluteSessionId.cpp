//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteSessionId.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Explicit constructor.
//----------------------------------------------------------------------------

ts::mcast::FluteSessionId::FluteSessionId(const IPAddress& source_, const IPSocketAddress& destination_, uint64_t tsi_) :
    source(source_),
    destination(destination_),
    tsi(tsi_)
{
}


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::mcast::FluteSessionId::clear()
{
    source.clear();
    destination.clear();
    tsi = 0;
}


//----------------------------------------------------------------------------
// Check if there is some valid session value.
//----------------------------------------------------------------------------

bool ts::mcast::FluteSessionId::isValid() const
{
    // Source address is not required.
    return destination.hasAddress() && destination.hasPort() && tsi != INVALID_TSI;
}


//----------------------------------------------------------------------------
// Comparison operator for use as index in maps.
//----------------------------------------------------------------------------

bool ts::mcast::FluteSessionId::operator<(const FluteSessionId& other) const
{
    if (tsi != other.tsi) {
        return tsi < other.tsi;
    }
    else if (source != other.source) {
        return source < other.source;
    }
    else {
        return destination < other.destination;
    }
}


//----------------------------------------------------------------------------
// Check if this session id "matches" another one.
//----------------------------------------------------------------------------

bool ts::mcast::FluteSessionId::match(const FluteSessionId& other) const
{
    return (tsi == INVALID_TSI || other.tsi == INVALID_TSI || tsi == other.tsi) &&
           source.match(other.source) && destination.match(other.destination);
}


//----------------------------------------------------------------------------
// Check if this session is in the DVB-NIP Announcement Channel.
//----------------------------------------------------------------------------

bool ts::mcast::FluteSessionId::nipAnnouncementChannel() const
{
    return destination == NIPSignallingAddress4() || destination.sameMulticast6(NIPSignallingAddress6());
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FluteSessionId::toString() const
{
    return UString::Format(u"source: %s, destination: %s, TSI: %d", source, destination, tsi);
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::FluteSessionId::parseXML(const xml::Element* element, bool strict)
{
    clear();
    uint16_t port = 0;
    bool ok = element != nullptr &&
              element->getIPChild(source, u"NetworkSourceAddress", false) &&
              element->getIPChild(destination, u"NetworkDestinationGroupAddress", strict) &&
              element->getIntChild(port, u"TransportDestinationPort", strict) &&
              element->getIntChild(tsi, u"MediaTransportSessionIdentifier", strict);
    destination.setPort(port);
    return ok;
}
