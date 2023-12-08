//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetIPSourceSlashDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_IP_source_slash_descriptor"
#define MY_CLASS ts::TargetIPSourceSlashDescriptor
#define MY_DID ts::DID_INT_IP_SRC_SLASH
#define MY_TID ts::TID_INT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPSourceSlashDescriptor::TargetIPSourceSlashDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TargetIPSourceSlashDescriptor::TargetIPSourceSlashDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetIPSourceSlashDescriptor()
{
    deserialize(duck, desc);
}

void ts::TargetIPSourceSlashDescriptor::clearContent()
{
    addresses.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPSourceSlashDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : addresses) {
        buf.putUInt32(it.IPv4_source_addr.address());
        buf.putUInt8(it.IPv4_source_slash_mask);
        buf.putUInt32(it.IPv4_dest_addr.address());
        buf.putUInt8(it.IPv4_dest_slash_mask);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPSourceSlashDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Address addr;
        addr.IPv4_source_addr = IPv4Address(buf.getUInt32());
        addr.IPv4_source_slash_mask = buf.getUInt8();
        addr.IPv4_dest_addr = IPv4Address(buf.getUInt32());
        addr.IPv4_dest_slash_mask = buf.getUInt8();
        addresses.push_back(addr);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPSourceSlashDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(10)) {
        disp << margin << "- Source:      " << IPv4Address(buf.getUInt32());
        disp << "/" << int(buf.getUInt8()) << std::endl;
        disp << margin << "  Destination: " << IPv4Address(buf.getUInt32());
        disp << "/" << int(buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPSourceSlashDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : addresses) {
        xml::Element* e = root->addElement(u"address");
        e->setIPAttribute(u"IPv4_source_addr", it.IPv4_source_addr);
        e->setIntAttribute(u"IPv4_source_slash_mask", it.IPv4_source_slash_mask);
        e->setIPAttribute(u"IPv4_dest_addr", it.IPv4_dest_addr);
        e->setIntAttribute(u"IPv4_dest_slash_mask", it.IPv4_dest_slash_mask);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPSourceSlashDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Address addr;
        ok = children[i]->getIPAttribute(addr.IPv4_source_addr, u"IPv4_source_addr", true) &&
             children[i]->getIntAttribute(addr.IPv4_source_slash_mask, u"IPv4_source_slash_mask", true) &&
             children[i]->getIPAttribute(addr.IPv4_dest_addr, u"IPv4_dest_addr", true) &&
             children[i]->getIntAttribute(addr.IPv4_dest_slash_mask, u"IPv4_dest_slash_mask", true);
        addresses.push_back(addr);
    }
    return ok;
}
