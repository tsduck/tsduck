//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetMACAddressDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_MAC_address_descriptor"
#define MY_CLASS ts::TargetMACAddressDescriptor
#define MY_DID ts::DID_INT_MAC_ADDR
#define MY_STD ts::Standards::DVB

// Table-specific descriptor which is allowed in two distinct tables.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_INT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetMACAddressDescriptor::TargetMACAddressDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TargetMACAddressDescriptor::TargetMACAddressDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetMACAddressDescriptor()
{
    deserialize(duck, desc);
}

void ts::TargetMACAddressDescriptor::clearContent()
{
    MAC_addr.clear();
    MAC_addr_mask.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt48(MAC_addr_mask.address());
    for (const auto& it : MAC_addr) {
        buf.putUInt48(it.address());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::deserializePayload(PSIBuffer& buf)
{
    MAC_addr_mask.setAddress(buf.getUInt48());
    while (buf.canRead()) {
        MAC_addr.push_back(MACAddress(buf.getUInt48()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    const char* header = "Address mask: ";
    while (buf.canReadBytes(6)) {
        disp << margin << header << MACAddress(buf.getUInt48()) << std::endl;
        header = "Address: ";
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setMACAttribute(u"MAC_addr_mask", MAC_addr_mask);
    for (const auto& it : MAC_addr) {
        root->addElement(u"address")->setMACAttribute(u"MAC_addr", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetMACAddressDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getMACAttribute(MAC_addr_mask, u"MAC_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        MACAddress addr;
        ok = children[i]->getMACAttribute(addr, u"MAC_addr", true);
        MAC_addr.push_back(addr);
    }
    return ok;
}
