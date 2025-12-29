//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#define MY_CLASS    ts::TargetIPv6AddressDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_IPV6_ADDR, ts::Standards::DVB, ts::TID_INT, ts::TID_UNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPv6AddressDescriptor::TargetIPv6AddressDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
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
    if (IPv6_addr_mask.generation() == IP::v6) {
        buf.putBytes(IPv6_addr_mask.address6());
    }
    else {
        buf.setUserError();
    }
    for (const auto& addr : IPv6_addr) {
        if (addr.generation() == IP::v6) {
            buf.putBytes(addr.address6());
        }
        else {
            buf.setUserError();
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::deserializePayload(PSIBuffer& buf)
{
    IPv6_addr_mask.setAddress(buf.getBytes(IPAddress::BYTES6));
    while (buf.canRead()) {
        IPv6_addr.push_back(IPAddress(buf.getBytes(IPAddress::BYTES6)));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    const char* header = "Address mask: ";
    while (buf.canReadBytes(16)) {
        disp << margin << header << IPAddress(buf.getBytes(IPAddress::BYTES6)) << std::endl;
        header = "Address: ";
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPv6AddressDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIPAttribute(u"IPv6_addr_mask", IPv6_addr_mask);
    for (const auto& it : IPv6_addr) {
        root->addElement(u"address")->setIPAttribute(u"IPv6_addr", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPv6AddressDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIPAttribute(IPv6_addr_mask, u"IPv6_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        IPAddress addr;
        ok = children[i]->getIPAttribute(addr, u"IPv6_addr", true);
        IPv6_addr.push_back(addr);
    }
    return ok;
}
