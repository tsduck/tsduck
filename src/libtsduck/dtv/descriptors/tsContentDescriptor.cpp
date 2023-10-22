//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"content_descriptor"
#define MY_CLASS ts::ContentDescriptor
#define MY_DID ts::DID_CONTENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentDescriptor::ContentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ContentDescriptor::clearContent()
{
    entries.clear();
}

ts::ContentDescriptor::ContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putBits(it.content_nibble_level_1, 4);
        buf.putBits(it.content_nibble_level_2, 4);
        buf.putBits(it.user_nibble_1, 4);
        buf.putBits(it.user_nibble_2, 4);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        entries.push_back(Entry(buf.getUInt16()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(2)) {
        disp << margin << "Content: " << names::Content(disp.duck(), buf.getUInt8(), NamesFlags::FIRST);
        disp << UString::Format(u" / User: 0x%X", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"content");
        e->setIntAttribute(u"content_nibble_level_1", it.content_nibble_level_1);
        e->setIntAttribute(u"content_nibble_level_2", it.content_nibble_level_2);
        e->setIntAttribute(u"user_byte", uint8_t((it.user_nibble_1 << 4) | it.user_nibble_2), true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"content", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        uint8_t user = 0;
        ok = children[i]->getIntAttribute(entry.content_nibble_level_1, u"content_nibble_level_1", true, 0, 0x00, 0x0F) &&
             children[i]->getIntAttribute(entry.content_nibble_level_2, u"content_nibble_level_2", true, 0, 0x00, 0x0F) &&
             children[i]->getIntAttribute(user, u"user_byte", true, 0, 0x00, 0xFF);
        entry.user_nibble_1 = (user >> 4) & 0x0F;
        entry.user_nibble_2 = user & 0x0F;
        entries.push_back(entry);
    }
    return ok;
}
