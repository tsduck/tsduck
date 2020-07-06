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

#include "tsHierarchyDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"hierarchy_descriptor"
#define MY_CLASS ts::HierarchyDescriptor
#define MY_DID ts::DID_HIERARCHY
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HierarchyDescriptor::HierarchyDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    temporal_scalability(false),
    spatial_scalability(false),
    quality_scalability(false),
    hierarchy_type(0),
    hierarchy_layer_index(0),
    tref_present(false),
    hierarchy_embedded_layer_index(0),
    hierarchy_channel(0)
{
}

void ts::HierarchyDescriptor::clearContent()
{
    temporal_scalability = false;
    spatial_scalability = false;
    quality_scalability = false;
    hierarchy_type = 0;
    hierarchy_layer_index = 0;
    tref_present = false;
    hierarchy_embedded_layer_index = 0;
    hierarchy_channel = 0;
}

ts::HierarchyDescriptor::HierarchyDescriptor(DuckContext& duck, const Descriptor& desc) :
    HierarchyDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(0x80 |
                     (temporal_scalability ? 0x40 : 0x00) |
                     (spatial_scalability ? 0x20 : 0x00) |
                     (quality_scalability ? 0x10 : 0x00) |
                     (hierarchy_type & 0x0F));
    bbp->appendUInt8(0xC0 | (hierarchy_layer_index & 0x3F));
    bbp->appendUInt8((tref_present ? 0x80 : 0x00) |
                     0x40 |
                     (hierarchy_embedded_layer_index & 0x3F));
    bbp->appendUInt8(0xC0 | (hierarchy_channel & 0x3F));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 4;

    if (_is_valid) {
        temporal_scalability = (data[0] & 0x40) != 0;
        spatial_scalability = (data[0] & 0x20) != 0;
        quality_scalability = (data[0] & 0x10) != 0;
        hierarchy_type = data[0] & 0x0F;
        hierarchy_layer_index = data[1] & 0x3F;
        tref_present = (data[2] & 0x80) != 0;
        hierarchy_embedded_layer_index = data[2] & 0x3F;
        hierarchy_channel = data[3] & 0x3F;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        strm << margin << "Temporal scalability: " << UString::TrueFalse((data[0] & 0x40) != 0) << std::endl
             << margin << "Spatial scalability: " << UString::TrueFalse((data[0] & 0x20) != 0) << std::endl
             << margin << "Quality scalability: " << UString::TrueFalse((data[0] & 0x10) != 0) << std::endl
             << margin << "Hierarchy type: " << NameFromSection(u"HierarchyType", data[0] & 0x0F, names::BOTH_FIRST) << std::endl
             << margin << UString::Format(u"Hierarchy layer index: %d", {data[1] & 0x3F}) << std::endl
             << margin << "Tref present: " << UString::TrueFalse((data[2] & 0x80) != 0) << std::endl
             << margin << UString::Format(u"Hierarchy embedded layer index: %d", {data[2] & 0x3F}) << std::endl
             << margin << UString::Format(u"Hierarchy channel: %d", {data[3] & 0x3F}) << std::endl;
        data += 4; size -= 4;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"temporal_scalability", temporal_scalability);
    root->setBoolAttribute(u"spatial_scalability", spatial_scalability);
    root->setBoolAttribute(u"quality_scalability", quality_scalability);
    root->setIntAttribute(u"hierarchy_type", hierarchy_type);
    root->setIntAttribute(u"hierarchy_layer_index", hierarchy_layer_index);
    root->setBoolAttribute(u"tref_present", tref_present);
    root->setIntAttribute(u"hierarchy_embedded_layer_index", hierarchy_embedded_layer_index);
    root->setIntAttribute(u"hierarchy_channel", hierarchy_channel);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HierarchyDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(temporal_scalability, u"temporal_scalability", true) &&
           element->getBoolAttribute(spatial_scalability, u"spatial_scalability", true) &&
           element->getBoolAttribute(quality_scalability, u"quality_scalability", true) &&
           element->getIntAttribute<uint8_t>(hierarchy_type, u"hierarchy_type", true, 0x00, 0x00, 0x0F) &&
           element->getIntAttribute<uint8_t>(hierarchy_layer_index, u"hierarchy_layer_index", true, 0x00, 0x00, 0x3F) &&
           element->getBoolAttribute(tref_present, u"tref_present", true) &&
           element->getIntAttribute<uint8_t>(hierarchy_embedded_layer_index, u"hierarchy_embedded_layer_index", true, 0x00, 0x00, 0x3F) &&
           element->getIntAttribute<uint8_t>(hierarchy_channel, u"hierarchy_channel", true, 0x00, 0x00, 0x3F);
}
