//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNodeRelationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::NodeRelationDescriptor::clearContent()
{
    reference_type = 0;
    information_provider_id.reset();
    event_relation_id.reset();
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

void ts::NodeRelationDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool has_external = information_provider_id.has_value() && event_relation_id.has_value();
    buf.putBits(reference_type, 4);
    buf.putBit(has_external);
    buf.putBits(0xFF, 3);
    if (has_external) {
        buf.putUInt16(information_provider_id.value());
        buf.putUInt16(event_relation_id.value());
    }
    buf.putUInt16(reference_node_id);
    buf.putUInt8(reference_number);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(reference_type, 4);
    const bool has_external = buf.getBool();
    buf.skipBits(3);
    if (has_external) {
        information_provider_id = buf.getUInt16();
        event_relation_id = buf.getUInt16();
    }
    reference_node_id = buf.getUInt16();
    reference_number = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NodeRelationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Reference type: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        const bool has_external = buf.getBool();
        buf.skipBits(3);
        if (has_external && buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"Information provider id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"Event relation id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        if (buf.canReadBytes(3)) {
            disp << margin << UString::Format(u"Reference node id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"Reference number: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
    }
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
        element->getIntAttribute(reference_type, u"reference_type", false, 0, 0, 15) &&
        element->getOptionalIntAttribute(information_provider_id, u"information_provider_id", true) &&
        element->getOptionalIntAttribute(event_relation_id, u"event_relation_id", true) &&
        element->getIntAttribute(reference_node_id, u"reference_node_id", true) &&
        element->getIntAttribute(reference_number, u"reference_number", true);

    if (ok && (information_provider_id.has_value() + event_relation_id.has_value()) == 1) {
        element->report().error(u"in <%s> line %d, attributes 'information_provider_id' and 'event_relation_id' must be both present or both absent", {element->name(), element->lineNumber()});
        ok = false;
    }
    return ok;
}
