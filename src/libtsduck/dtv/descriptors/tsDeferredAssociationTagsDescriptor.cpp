//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDeferredAssociationTagsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"deferred_association_tags_descriptor"
#define MY_CLASS ts::DeferredAssociationTagsDescriptor
#define MY_DID ts::DID_DEFERRED_ASSOC_TAGS
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DeferredAssociationTagsDescriptor::DeferredAssociationTagsDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::DeferredAssociationTagsDescriptor::clearContent()
{
    association_tags.clear();
    transport_stream_id = 0;
    program_number = 0;
    private_data.clear();
}

ts::DeferredAssociationTagsDescriptor::DeferredAssociationTagsDescriptor(DuckContext& duck, const Descriptor& desc) :
    DeferredAssociationTagsDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DeferredAssociationTagsDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.pushWriteSequenceWithLeadingLength(8); // association_tags_loop_length
    for (auto it : association_tags) {
        buf.putUInt16(it);
    }
    buf.popState(); // update association_tags_loop_length
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(program_number);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DeferredAssociationTagsDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(8); // association_tags_loop_length
    while (buf.canRead()) {
        association_tags.push_back(buf.getUInt16());
    }
    buf.popState(); // update association_tags_loop_length
    transport_stream_id = buf.getUInt16();
    program_number = buf.getUInt16();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DeferredAssociationTagsDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    buf.pushReadSizeFromLength(8); // association_tags_loop_length
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Association tag: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    }
    buf.popState(); // update association_tags_loop_length
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DeferredAssociationTagsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"program_number", program_number, true);
    for (auto it : association_tags) {
        root->addElement(u"association")->setIntAttribute(u"tag", it, true);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DeferredAssociationTagsDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(program_number, u"program_number", true) &&
        element->getChildren(children, u"association") &&
        element->getHexaTextChild(private_data, u"private_data", false);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint16_t tag = 0;
        ok = children[i]->getIntAttribute(tag, u"tag", true);
        association_tags.push_back(tag);
    }
    return ok;
}
