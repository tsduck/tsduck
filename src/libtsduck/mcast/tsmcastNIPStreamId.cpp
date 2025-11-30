//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPStreamId.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Clear the content of a DVB-NIP Actual Carrier Information.
//----------------------------------------------------------------------------

void ts::mcast::NIPStreamId::clear()
{
    network_id = carrier_id = link_id = service_id = 0;
}


//----------------------------------------------------------------------------
// Comparison operator for use as index in maps.
//----------------------------------------------------------------------------

bool ts::mcast::NIPStreamId::operator<(const NIPStreamId& other) const
{
    return ((uint64_t(network_id) << 48) | (uint64_t(carrier_id) << 32) | (uint64_t(link_id) << 16) | network_id) <
        ((uint64_t(other.network_id) << 48) | (uint64_t(other.carrier_id) << 32) | (uint64_t(other.link_id) << 16) | other.network_id);

}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::NIPStreamId::toString() const
{
    return UString::Format(u"network: %d, carrier: %d, link: %d, service: %d", network_id, carrier_id, link_id, service_id);
}


//----------------------------------------------------------------------------
// Read from a XML element.
//----------------------------------------------------------------------------

bool ts::mcast::NIPStreamId::parseXML(const xml::Element* element)
{
    return element != nullptr &&
        element->getIntChild(network_id, u"NIPNetworkID", true, 0, 1, 65280) &&
        element->getIntChild(carrier_id, u"NIPCarrierID", true) &&
        element->getIntChild(link_id, u"NIPLinkID", true) &&
        element->getIntChild(service_id, u"NIPServiceID", true);
}
