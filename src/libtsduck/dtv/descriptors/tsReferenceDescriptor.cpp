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

#include "tsReferenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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

void ts::ReferenceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(information_provider_id);
    bbp->appendUInt16(event_relation_id);
    for (auto it = references.begin(); it != references.end(); ++it) {
        bbp->appendUInt16(it->reference_node_id);
        bbp->appendUInt8(it->reference_number);
        bbp->appendUInt8(it->last_reference_number);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && size % 4 == 0;

    references.clear();

    if (_is_valid) {
        information_provider_id = GetUInt16(data);
        event_relation_id = GetUInt16(data + 2);
        data += 4; size -= 4;

        while (_is_valid && size >= 4) {
            Reference ref;
            ref.reference_node_id = GetUInt16(data);
            ref.reference_number = data[2];
            ref.last_reference_number = data[3];
            data += 4; size -= 4;
            references.push_back(ref);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"Information provider id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << UString::Format(u"Event relation id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl;
        data += 4; size -= 4;

        while (size >= 4) {
            strm << margin << UString::Format(u"- Reference node id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << UString::Format(u"  Reference number: 0x%X (%d)", {data[2], data[2]}) << std::endl
                 << margin << UString::Format(u"  Last reference number: 0x%X (%d)", {data[3], data[3]}) << std::endl;
            data += 4; size -= 4;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ReferenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"information_provider_id", information_provider_id, true);
    root->setIntAttribute(u"event_relation_id", event_relation_id, true);
    for (auto it = references.begin(); it != references.end(); ++it) {
        xml::Element* e = root->addElement(u"reference");
        e->setIntAttribute(u"reference_node_id", it->reference_node_id, true);
        e->setIntAttribute(u"reference_number", it->reference_number, true);
        e->setIntAttribute(u"last_reference_number", it->last_reference_number, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ReferenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xref;
    bool ok =
        element->getIntAttribute<uint16_t>(information_provider_id, u"information_provider_id", true) &&
        element->getIntAttribute<uint16_t>(event_relation_id, u"event_relation_id", true) &&
        element->getChildren(xref, u"reference");

    for (auto it = xref.begin(); ok && it != xref.end(); ++it) {
        Reference ref;
        ok = (*it)->getIntAttribute<uint16_t>(ref.reference_node_id, u"reference_node_id", true) &&
             (*it)->getIntAttribute<uint8_t>(ref.reference_number, u"reference_number", true) &&
             (*it)->getIntAttribute<uint8_t>(ref.last_reference_number, u"last_reference_number", true);
        references.push_back(ref);
    }
    return ok;
}
