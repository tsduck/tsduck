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

#include "tsNodeRelationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"node_relation_descriptor"
#define MY_CLASS ts::NodeRelationDescriptor
#define MY_DID ts::DID_ISDB_NODE_RELATION
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NodeRelationDescriptor::NodeRelationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    reference_type(0),
    information_provider_id(),
    event_relation_id(),
    reference_node_id(0),
    reference_number(0)
{
}

void ts::NodeRelationDescriptor::clearContent()
{
    reference_type = 0;
    information_provider_id.clear();
    event_relation_id.clear();
    reference_node_id = 0;
    reference_number = 0;
}

ts::NodeRelationDescriptor::NodeRelationDescriptor(DuckContext& duck, const Descriptor& desc) :
    NodeRelationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    const bool has_external = information_provider_id.set() && event_relation_id.set();
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(reference_type << 4) | (has_external ? 0x0F : 0x07));
    if (has_external) {
        bbp->appendUInt16(information_provider_id.value());
        bbp->appendUInt16(event_relation_id.value());
    }
    bbp->appendUInt16(reference_node_id);
    bbp->appendUInt8(reference_number);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4;

    information_provider_id.clear();
    event_relation_id.clear();

    if (_is_valid) {
        reference_type = (data[0] >> 4) & 0x0F;
        const bool has_external = (data[0] & 0x08) != 0;
        data++; size--;
        _is_valid = size == size_t(has_external ? 7 : 3);
        if (_is_valid) {
            if (has_external) {
                information_provider_id = GetUInt16(data);
                event_relation_id = GetUInt16(data + 2);
                data += 4; size -= 4;
            }
            reference_node_id = GetUInt16(data);
            reference_number = data[2];
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"Reference type: %d", {(data[0] >> 4) & 0x0F}) << std::endl;
        const bool has_external = (data[0] & 0x08) != 0;
        data++; size--;

        if (size >= size_t(has_external ? 7 : 3)) {
            if (has_external) {
                strm << margin << UString::Format(u"Information provider id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"Event relation id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl;
                data += 4; size -= 4;
            }
            strm << margin << UString::Format(u"Reference node id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << UString::Format(u"Reference number: 0x%X (%d)", {data[2], data[2]}) << std::endl;
            data += 3; size -= 3;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"reference_type", reference_type);
    root->setOptionalIntAttribute(u"information_provider_id", information_provider_id, true);
    root->setOptionalIntAttribute(u"event_relation_id", event_relation_id, true);
    root->setIntAttribute(u"reference_node_id", reference_node_id, true);
    root->setIntAttribute(u"reference_number", reference_number, true);
}

/*
    <node_relation_descriptor
        reference_type="uint4, default=0"
        information_provider_id="uint16, optional"
        event_relation_id="uint16, optional"
        reference_node_id="uint16, required"
        reference_number="uint8, required"/>
*/

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NodeRelationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute<uint8_t>(reference_type, u"reference_type", false, 0, 0, 15) &&
        element->getOptionalIntAttribute<uint16_t>(information_provider_id, u"information_provider_id", true) &&
        element->getOptionalIntAttribute<uint16_t>(event_relation_id, u"event_relation_id", true) &&
        element->getIntAttribute<uint16_t>(reference_node_id, u"reference_node_id", true) &&
        element->getIntAttribute<uint8_t>(reference_number, u"reference_number", true);

    if (ok && (information_provider_id.set() + event_relation_id.set()) == 1) {
        element->report().error(u"in <%s> line %d, attributes 'information_provider_id' and 'event_relation_id' must be both present or both absent", {element->name(), element->lineNumber()});
        ok = false;
    }
    return ok;
}
