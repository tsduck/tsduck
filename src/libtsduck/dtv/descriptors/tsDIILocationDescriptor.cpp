//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsDIILocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    transport_protocol_label(0),
    entries()
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(transport_protocol_label);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        buf.putBit(1);
        buf.putBits(it->DII_identification, 15);
        buf.putUInt16(it->association_tag);
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

void ts::DIILocationDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 1) {
        disp << margin << UString::Format(u"Transport protocol label: 0x%X (%d)", {data[0], data[0]}) << std::endl;
        data++; size--;
        while (size >= 4) {
            const uint16_t id = GetUInt16(data) & 0x7FFF;
            const uint16_t tag = GetUInt16(data + 2);
            data += 4; size -= 4;
            disp << margin << UString::Format(u"DII id: 0x%X (%d), tag: 0x%X (%d)", {id, id, tag, tag}) << std::endl;
        }
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DIILocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_protocol_label", transport_protocol_label, true);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"module");
        e->setIntAttribute(u"DII_identification", it->DII_identification, true);
        e->setIntAttribute(u"association_tag", it->association_tag, true);
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
