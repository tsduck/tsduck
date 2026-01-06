//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetIPAddressDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_IP_address_descriptor"
#define MY_CLASS    ts::TargetIPAddressDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_IP_ADDR, ts::Standards::DVB, ts::TID_INT, ts::TID_UNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPAddressDescriptor::TargetIPAddressDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::TargetIPAddressDescriptor::clearContent()
{
    IPv4_addr_mask.clear();
    IPv4_addr.clear();
}

ts::TargetIPAddressDescriptor::TargetIPAddressDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetIPAddressDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (IPv4_addr_mask.generation() == IP::v4) {
        buf.putUInt32(IPv4_addr_mask.address4());
    }
    else {
        buf.setUserError();
    }
    for (const auto& addr : IPv4_addr) {
        if (addr.generation() == IP::v4) {
            buf.putUInt32(addr.address4());
        }
        else {
            buf.setUserError();
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::deserializePayload(PSIBuffer& buf)
{
    IPv4_addr_mask.setAddress4(buf.getUInt32());
    while (buf.canRead()) {
        IPv4_addr.push_back(IPAddress(buf.getUInt32()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    const char* header = "Address mask: ";
    while (buf.canReadBytes(4)) {
        disp << margin << header << IPAddress(buf.getUInt32()) << std::endl;
        header = "Address: ";
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIPAttribute(u"IPv4_addr_mask", IPv4_addr_mask);
    for (const auto& it : IPv4_addr) {
        root->addElement(u"address")->setIPAttribute(u"IPv4_addr", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPAddressDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIPAttribute(IPv4_addr_mask, u"IPv4_addr_mask", true);
    for (auto& child : element->children(u"address", &ok, 0, MAX_ENTRIES)) {
        ok = child.getIPAttribute(IPv4_addr.emplace_back(), u"IPv4_addr", true);
    }
    return ok;
}
