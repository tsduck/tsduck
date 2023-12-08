//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetIPv6AddressDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_IPv6_address_descriptor"
#define MY_CLASS ts::TargetIPv6AddressDescriptor
#define MY_DID ts::DID_INT_IPV6_ADDR
#define MY_STD ts::Standards::DVB

// Table-specific descriptor which is allowed in two distinct tables.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_INT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPv6AddressDescriptor::TargetIPv6AddressDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TargetIPv6AddressDescriptor::clearContent()
{
    IPv6_addr_mask.clear();
    IPv6_addr.clear();
}

ts::TargetIPv6AddressDescriptor::TargetIPv6AddressDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetIPv6AddressDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(IPv6_addr_mask.toBytes());
    for (const auto& it : IPv6_addr) {
        buf.putBytes(it.toBytes());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::deserializePayload(PSIBuffer& buf)
{
    IPv6_addr_mask.setAddress(buf.getBytes(16));
    while (buf.canRead()) {
        IPv6_addr.push_back(IPv6Address(buf.getBytes(16)));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    const char* header = "Address mask: ";
    while (buf.canReadBytes(16)) {
        disp << margin << header << IPv6Address(buf.getBytes(16)) << std::endl;
        header = "Address: ";
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIPv6Attribute(u"IPv6_addr_mask", IPv6_addr_mask);
    for (const auto& it : IPv6_addr) {
        root->addElement(u"address")->setIPv6Attribute(u"IPv6_addr", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPv6AddressDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIPv6Attribute(IPv6_addr_mask, u"IPv6_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        IPv6Address addr;
        ok = children[i]->getIPv6Attribute(addr, u"IPv6_addr", true);
        IPv6_addr.push_back(addr);
    }
    return ok;
}
