//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsDTGServiceAttributeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dtg_service_attribute_descriptor"
#define MY_CLASS ts::DTGServiceAttributeDescriptor
#define MY_DID ts::DID_OFCOM_SERVICE_ATTR
#define MY_PDS ts::PDS_OFCOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGServiceAttributeDescriptor::DTGServiceAttributeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
}

ts::DTGServiceAttributeDescriptor::DTGServiceAttributeDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTGServiceAttributeDescriptor()
{
    deserialize(duck, desc);
}

ts::DTGServiceAttributeDescriptor::Entry::Entry(uint16_t id, bool numeric, bool visible) :
    service_id(id),
    numeric_selection(numeric),
    visible_service(visible)
{
}

void ts::DTGServiceAttributeDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTGServiceAttributeDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.service_id);
        buf.putBits(0xFF, 6);
        buf.putBit(it.numeric_selection);
        buf.putBit(it.visible_service);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTGServiceAttributeDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.service_id = buf.getUInt16();
        buf.skipBits(6);
        e.numeric_selection = buf.getBool();
        e.visible_service = buf.getBool();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTGServiceAttributeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Service Id: %5d (0x%<X)", {buf.getUInt16()});
        buf.skipBits(6);
        disp << UString::Format(u", numeric selection: %s", {buf.getBool()});
        disp << UString::Format(u", visible: %s", {buf.getBool()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTGServiceAttributeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setBoolAttribute(u"numeric_selection", it.numeric_selection);
        e->setBoolAttribute(u"visible_service", it.visible_service);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DTGServiceAttributeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xservice;
    bool ok = element->getChildren(xservice, u"service", 0, MAX_ENTRIES);

    for (auto it = xservice.begin(); ok && it != xservice.end(); ++it) {
        Entry entry;
        ok = (*it)->getIntAttribute(entry.service_id, u"service_id", true) &&
             (*it)->getBoolAttribute(entry.numeric_selection, u"numeric_selection", true) &&
             (*it)->getBoolAttribute(entry.visible_service, u"visible_service", true);
        entries.push_back(entry);
    }
    return ok;
}
