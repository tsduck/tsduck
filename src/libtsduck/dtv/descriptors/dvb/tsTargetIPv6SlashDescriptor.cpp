//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetIPv6SlashDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_IPv6_slash_descriptor"
#define MY_CLASS    ts::TargetIPv6SlashDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_IPV6_SLASH, ts::Standards::DVB, ts::TID_INT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPv6SlashDescriptor::TargetIPv6SlashDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::TargetIPv6SlashDescriptor::clearContent()
{
    addresses.clear();
}

ts::TargetIPv6SlashDescriptor::TargetIPv6SlashDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetIPv6SlashDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPv6SlashDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& addr : addresses) {
        if (addr.IPv6_addr.generation() == IP::v6) {
            buf.putBytes(addr.IPv6_addr.address6());
            buf.putUInt8(addr.IPv6_slash_mask);
        }
        else {
            buf.setUserError();
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPv6SlashDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Address addr;
        addr.IPv6_addr.setAddress(buf.getBytes(IPAddress::BYTES6));
        addr.IPv6_slash_mask = buf.getUInt8();
        addresses.push_back(addr);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPv6SlashDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(17)) {
        disp << margin << "Address/mask: " << IPAddress(buf.getBytes(IPAddress::BYTES6));
        disp << "/" << int(buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPv6SlashDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : addresses) {
        xml::Element* e = root->addElement(u"address");
        e->setIPAttribute(u"IPv6_addr", it.IPv6_addr);
        e->setIntAttribute(u"IPv6_slash_mask", it.IPv6_slash_mask);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPv6SlashDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Address addr;
        ok = children[i]->getIPAttribute(addr.IPv6_addr, u"IPv6_addr", true) &&
             children[i]->getIntAttribute(addr.IPv6_slash_mask, u"IPv6_slash_mask", true);
        addresses.push_back(addr);
    }
    return ok;
}
