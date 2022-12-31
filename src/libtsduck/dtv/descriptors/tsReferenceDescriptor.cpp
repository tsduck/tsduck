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

#include "tsReferenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"reference_descriptor"
#define MY_CLASS ts::ReferenceDescriptor
#define MY_DID ts::DID_ISDB_REFERENCE
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ReferenceDescriptor::ReferenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    information_provider_id(0),
    event_relation_id(0),
    references()
{
}

void ts::ReferenceDescriptor::clearContent()
{
    information_provider_id = 0;
    event_relation_id = 0;
    references.clear();
}

ts::ReferenceDescriptor::ReferenceDescriptor(DuckContext& duck, const Descriptor& desc) :
    ReferenceDescriptor()
{
    deserialize(duck, desc);
}

ts::ReferenceDescriptor::Reference::Reference() :
    reference_node_id(0),
    reference_number(0),
    last_reference_number(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(information_provider_id);
    buf.putUInt16(event_relation_id);
    for (const auto& it : references) {
        buf.putUInt16(it.reference_node_id);
        buf.putUInt8(it.reference_number);
        buf.putUInt8(it.last_reference_number);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    information_provider_id = buf.getUInt16();
    event_relation_id = buf.getUInt16();
    while (buf.canRead()) {
        Reference ref;
        ref.reference_node_id = buf.getUInt16();
        ref.reference_number = buf.getUInt8();
        ref.last_reference_number = buf.getUInt8();
        references.push_back(ref);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Information provider id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Event relation id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        while (buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"- Reference node id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Reference number: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            disp << margin << UString::Format(u"  Last reference number: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"information_provider_id", information_provider_id, true);
    root->setIntAttribute(u"event_relation_id", event_relation_id, true);
    for (const auto& it : references) {
        xml::Element* e = root->addElement(u"reference");
        e->setIntAttribute(u"reference_node_id", it.reference_node_id, true);
        e->setIntAttribute(u"reference_number", it.reference_number, true);
        e->setIntAttribute(u"last_reference_number", it.last_reference_number, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ReferenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xref;
    bool ok =
        element->getIntAttribute(information_provider_id, u"information_provider_id", true) &&
        element->getIntAttribute(event_relation_id, u"event_relation_id", true) &&
        element->getChildren(xref, u"reference");

    for (auto it = xref.begin(); ok && it != xref.end(); ++it) {
        Reference ref;
        ok = (*it)->getIntAttribute(ref.reference_node_id, u"reference_node_id", true) &&
             (*it)->getIntAttribute(ref.reference_number, u"reference_number", true) &&
             (*it)->getIntAttribute(ref.last_reference_number, u"last_reference_number", true);
        references.push_back(ref);
    }
    return ok;
}
