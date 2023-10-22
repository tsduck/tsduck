//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDIILocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DII_location_descriptor"
#define MY_CLASS ts::DIILocationDescriptor
#define MY_DID ts::DID_AIT_DII_LOCATION
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DIILocationDescriptor::DIILocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DIILocationDescriptor::DIILocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DIILocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DIILocationDescriptor::clearContent()
{
    transport_protocol_label = 0;
    entries.clear();
}

ts::DIILocationDescriptor::Entry::Entry(uint16_t id, uint16_t tag) :
    DII_identification(id),
    association_tag(tag)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(transport_protocol_label);
    for (const auto& it : entries) {
        buf.putBit(1);
        buf.putBits(it.DII_identification, 15);
        buf.putUInt16(it.association_tag);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    transport_protocol_label = buf.getUInt8();
    while (buf.canRead()) {
        Entry e;
        buf.skipBits(1);
        buf.getBits(e.DII_identification, 15);
        e.association_tag = buf.getUInt16();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Transport protocol label: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        while (buf.canReadBytes(4)) {
            buf.skipBits(1);
            disp << margin << UString::Format(u"DII id: 0x%X (%<d)", {buf.getBits<uint16_t>(15)});
            disp << UString::Format(u", tag: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_protocol_label", transport_protocol_label, true);
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"module");
        e->setIntAttribute(u"DII_identification", it.DII_identification, true);
        e->setIntAttribute(u"association_tag", it.association_tag, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DIILocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(transport_protocol_label, u"transport_protocol_label", true) &&
        element->getChildren(children, u"module", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.DII_identification, u"DII_identification", true, 0, 0x0000, 0x7FFF) &&
             children[i]->getIntAttribute(entry.association_tag, u"association_tag", true);
        entries.push_back(entry);
    }
    return ok;
}
