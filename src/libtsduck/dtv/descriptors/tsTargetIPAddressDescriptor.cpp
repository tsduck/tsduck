//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#define MY_CLASS ts::TargetIPAddressDescriptor
#define MY_DID ts::DID_INT_IP_ADDR
#define MY_STD ts::Standards::DVB

// Table-specific descriptor which is allowed in two distinct tables.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_INT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPAddressDescriptor::TargetIPAddressDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
    buf.putUInt32(IPv4_addr_mask.address());
    for (const auto& it : IPv4_addr) {
        buf.putUInt32(it.address());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::deserializePayload(PSIBuffer& buf)
{
    IPv4_addr_mask.setAddress(buf.getUInt32());
    while (buf.canRead()) {
        IPv4_addr.push_back(IPv4Address(buf.getUInt32()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    const char* header = "Address mask: ";
    while (buf.canReadBytes(4)) {
        disp << margin << header << IPv4Address(buf.getUInt32()) << std::endl;
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
    xml::ElementVector children;
    bool ok =
        element->getIPAttribute(IPv4_addr_mask, u"IPv4_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        IPv4Address addr;
        ok = children[i]->getIPAttribute(addr, u"IPv4_addr", true);
        IPv4_addr.push_back(addr);
    }
    return ok;
}
