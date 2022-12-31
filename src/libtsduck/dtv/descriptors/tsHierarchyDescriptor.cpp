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

#include "tsHierarchyDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    no_view_scalability_flag(false),
    no_temporal_scalability(false),
    no_spatial_scalability(false),
    no_quality_scalability(false),
    hierarchy_type(0),
    hierarchy_layer_index(0),
    tref_present(false),
    hierarchy_embedded_layer_index(0),
    hierarchy_channel(0)
{
}

void ts::HierarchyDescriptor::clearContent()
{
    no_view_scalability_flag = false;
    no_temporal_scalability = false;
    no_spatial_scalability = false;
    no_quality_scalability = false;
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

void ts::HierarchyDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(no_view_scalability_flag);
    buf.putBit(no_temporal_scalability);
    buf.putBit(no_spatial_scalability);
    buf.putBit(no_quality_scalability);
    buf.putBits(hierarchy_type, 4);
    buf.putBits(0xFF, 2);
    buf.putBits(hierarchy_layer_index, 6);
    buf.putBit(tref_present);
    buf.putBit(1);
    buf.putBits(hierarchy_embedded_layer_index, 6);
    buf.putBits(0xFF, 2);
    buf.putBits(hierarchy_channel, 6);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::deserializePayload(PSIBuffer& buf)
{
    no_view_scalability_flag = buf.getBool();
    no_temporal_scalability = buf.getBool();
    no_spatial_scalability = buf.getBool();
    no_quality_scalability = buf.getBool();
    buf.getBits(hierarchy_type, 4);
    buf.skipReservedBits(2);
    buf.getBits(hierarchy_layer_index, 6);
    tref_present = buf.getBool();
    buf.skipReservedBits(1);
    buf.getBits(hierarchy_embedded_layer_index, 6);
    buf.skipReservedBits(2);
    buf.getBits(hierarchy_channel, 6);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "No view scalability: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "No temporal scalability: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "No spatial scalability: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "No quality scalability: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Hierarchy type: " << DataName(MY_XML_NAME, u"HierarchyType", buf.getBits<uint8_t>(4), NamesFlags::BOTH_FIRST) << std::endl;
        buf.skipReservedBits(2);
        disp << margin << UString::Format(u"Hierarchy layer index: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
        disp << margin << "Tref present: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"Hierarchy embedded layer index: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
        buf.skipReservedBits(2);
        disp << margin << UString::Format(u"Hierarchy channel: %d", {buf.getBits<uint8_t>(6)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HierarchyDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"no_view_scalability_flag", no_view_scalability_flag);
    root->setBoolAttribute(u"no_temporal_scalability", no_temporal_scalability);
    root->setBoolAttribute(u"no_spatial_scalability", no_spatial_scalability);
    root->setBoolAttribute(u"no_quality_scalability", no_quality_scalability);
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
    // Attributes "no_temporal_scalability", "no_spatial_scalability" and "no_quality_scalability"
    // were previously named without "no_". The name change was made in ISO 13818-1 and integrated
    // in TSDuck. The field "no_view_scalability_flag" is new (never existed without "no_").
    // For compatibility, all those flags are now optional with default value being true.

    bool temporal_scalability = false;
    bool spatial_scalability = false;
    bool quality_scalability = false;

    return element->getBoolAttribute(no_view_scalability_flag, u"no_view_scalability_flag", false, true) &&
           element->getBoolAttribute(temporal_scalability, u"temporal_scalability", false, true) &&
           element->getBoolAttribute(no_temporal_scalability, u"no_temporal_scalability", false, temporal_scalability) &&
           element->getBoolAttribute(spatial_scalability, u"spatial_scalability", false, true) &&
           element->getBoolAttribute(no_spatial_scalability, u"no_spatial_scalability", false, spatial_scalability) &&
           element->getBoolAttribute(quality_scalability, u"quality_scalability", false, true) &&
           element->getBoolAttribute(no_quality_scalability, u"no_quality_scalability", false, quality_scalability) &&
           element->getIntAttribute(hierarchy_type, u"hierarchy_type", true, 0x00, 0x00, 0x0F) &&
           element->getIntAttribute(hierarchy_layer_index, u"hierarchy_layer_index", true, 0x00, 0x00, 0x3F) &&
           element->getBoolAttribute(tref_present, u"tref_present", true) &&
           element->getIntAttribute(hierarchy_embedded_layer_index, u"hierarchy_embedded_layer_index", true, 0x00, 0x00, 0x3F) &&
           element->getIntAttribute(hierarchy_channel, u"hierarchy_channel", true, 0x00, 0x00, 0x3F);
}
