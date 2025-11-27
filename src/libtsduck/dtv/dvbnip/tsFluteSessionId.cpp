//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteSessionId.h"
#include "tsNIP.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Explicit constructor.
//----------------------------------------------------------------------------

ts::FluteSessionId::FluteSessionId(const IPAddress& source_, const IPSocketAddress& destination_, uint64_t tsi_) :
    source(source_),
    destination(destination_),
    tsi(tsi_)
{
}


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::FluteSessionId::clear()
{
    source.clear();
    destination.clear();
    tsi = 0;
}


//----------------------------------------------------------------------------
// Comparison operator for use as index in maps.
//----------------------------------------------------------------------------

bool ts::FluteSessionId::operator<(const FluteSessionId& other) const
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

bool ts::FluteSessionId::match(const FluteSessionId& other) const
{
    return (tsi == INVALID_TSI || other.tsi == INVALID_TSI || tsi == other.tsi) &&
           source.match(other.source) && destination.match(other.destination);
}


//----------------------------------------------------------------------------
// Check if this session is in the DVB-NIP Announcement Channel.
//----------------------------------------------------------------------------

bool ts::FluteSessionId::nipAnnouncementChannel() const
{
    return destination == NIPSignallingAddress4() || destination.sameMulticast6(NIPSignallingAddress6());
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::FluteSessionId::toString() const
{
    return UString::Format(u"source: %s, destination: %s, TSI: %d", source, destination, tsi);
}


//----------------------------------------------------------------------------
// Reinitialize the structure from a XML element.
//----------------------------------------------------------------------------

bool ts::FluteSessionId::parseXML(const xml::Element* element)
{
    clear();
    uint16_t port = 0;
    bool ok = element != nullptr &&
              element->getIPChild(source, u"NetworkSourceAddress", false) &&
              element->getIPChild(destination, u"NetworkDestinationGroupAddress", true) &&
              element->getIntChild(port, u"TransportDestinationPort", true) &&
              element->getIntChild(tsi, u"MediaTransportSessionIdentifier", true);
    destination.setPort(port);
    return ok;
}
