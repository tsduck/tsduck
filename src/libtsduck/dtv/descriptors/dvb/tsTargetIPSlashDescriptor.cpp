//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetIPSlashDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_IP_slash_descriptor"
#define MY_CLASS    ts::TargetIPSlashDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_IP_SLASH, ts::Standards::DVB, ts::TID_INT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPSlashDescriptor::TargetIPSlashDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::TargetIPSlashDescriptor::clearContent()
{
    addresses.clear();
}

ts::TargetIPSlashDescriptor::TargetIPSlashDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetIPSlashDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPSlashDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& addr : addresses) {
        if (addr.IPv4_addr.generation() == IP::v4) {
            buf.putUInt32(addr.IPv4_addr.address4());
            buf.putUInt8(addr.IPv4_slash_mask);
        }
        else {
            buf.setUserError();
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPSlashDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Address addr;
        addr.IPv4_addr = IPAddress(buf.getUInt32());
        addr.IPv4_slash_mask = buf.getUInt8();
        addresses.push_back(addr);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPSlashDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(5)) {
        disp << margin << "Address/mask: " << IPAddress(buf.getUInt32());
        disp << "/" << int(buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPSlashDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : addresses) {
        xml::Element* e = root->addElement(u"address");
        e->setIPAttribute(u"IPv4_addr", it.IPv4_addr);
        e->setIntAttribute(u"IPv4_slash_mask", it.IPv4_slash_mask);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetIPSlashDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Address addr;
        ok = children[i]->getIPAttribute(addr.IPv4_addr, u"IPv4_addr", true) &&
             children[i]->getIntAttribute(addr.IPv4_slash_mask, u"IPv4_slash_mask", true);
        addresses.push_back(addr);
    }
    return ok;
}
