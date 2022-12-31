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

#include "tsHEVCHierarchyExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"HEVC_hierarchy_extension_descriptor"
#define MY_CLASS ts::HEVCHierarchyExtensionDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_HEVC_HIER_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCHierarchyExtensionDescriptor::HEVCHierarchyExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    extension_dimension_bits(0),
    hierarchy_layer_index(0),
    temporal_id(0),
    nuh_layer_id(0),
    tref_present(false),
    hierarchy_channel(0),
    hierarchy_ext_embedded_layer_index()
{
}

ts::HEVCHierarchyExtensionDescriptor::HEVCHierarchyExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCHierarchyExtensionDescriptor()
{
    deserialize(duck, desc);
}

void ts::HEVCHierarchyExtensionDescriptor::clearContent()
{
    extension_dimension_bits = 0;
    hierarchy_layer_index = 0;
    temporal_id = 0;
    nuh_layer_id = 0;
    tref_present = false;
    hierarchy_channel = 0;
    hierarchy_ext_embedded_layer_index.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::HEVCHierarchyExtensionDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::HEVCHierarchyExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(extension_dimension_bits);
    buf.putBits(hierarchy_layer_index, 6);
    buf.putBits(temporal_id, 3);
    buf.putBits(nuh_layer_id, 6);
    buf.putBit(tref_present);
    buf.putBits(0xFF, 2);
    buf.putBits(hierarchy_ext_embedded_layer_index.size(), 6);
    buf.putBits(0xFF, 2);
    buf.putBits(hierarchy_channel, 6);
    for (const auto& it : hierarchy_ext_embedded_layer_index) {
        buf.putBits(0xFF, 2);
        buf.putBits(it, 6);
    }
}

void ts::HEVCHierarchyExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    extension_dimension_bits = buf.getUInt16();
    buf.getBits(hierarchy_layer_index, 6);
    buf.getBits(temporal_id, 3);
    buf.getBits(nuh_layer_id, 6);
    tref_present = buf.getBool();
    buf.skipBits(2);
    const size_t num_embedded_layers = buf.getBits<uint8_t>(6);
    buf.skipBits(2);
    buf.getBits(hierarchy_channel, 6);
    for (size_t i = 0; i < num_embedded_layers && !buf.error(); ++i) {
        buf.skipBits(2);
        hierarchy_ext_embedded_layer_index.push_back(buf.getBits<uint8_t>(6));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCHierarchyExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(6)) {
        const uint16_t bits = buf.getUInt16();
        disp << margin << UString::Format(u"Extension dimension bits: 0x%X", {bits}) << std::endl;
        for (size_t bit = 0; bit < 16; ++bit) {
            if ((bits & (0x8000 >> bit)) != 0) {
                disp << margin << "  Bit " << bit << ": " << DataName(MY_XML_NAME, u"ExtensionDimensionBits", bit) << std::endl;
            }
        }
        disp << margin << UString::Format(u"Hierarchy layer index: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
        disp << margin << UString::Format(u"Temporal id: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
        disp << margin << UString::Format(u"NUH layer id: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
        disp << margin << UString::Format(u"TREF present: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(2);
        const size_t num_embedded_layers = buf.getBits<uint8_t>(6);
        disp << margin << UString::Format(u"Number of embedded layers: %d", {num_embedded_layers}) << std::endl;
        buf.skipBits(2);
        disp << margin << UString::Format(u"Hierarchy channel: 0x%X (%<d)", {buf.getBits<uint8_t>(6)}) << std::endl;
        for (size_t i = 0; i < num_embedded_layers && buf.canReadBytes(1); ++i) {
            buf.skipBits(2);
            disp << margin << UString::Format(u"Hierarchy embeddedlayer index[%d]: 0x%X (%<d)", {i, buf.getBits<uint8_t>(6)}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::HEVCHierarchyExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"extension_dimension_bits", extension_dimension_bits, true);
    root->setIntAttribute(u"hierarchy_layer_index", hierarchy_layer_index, true);
    root->setIntAttribute(u"temporal_id", temporal_id, false);
    root->setIntAttribute(u"nuh_layer_id", nuh_layer_id, true);
    root->setBoolAttribute(u"tref_present", tref_present);
    root->setIntAttribute(u"hierarchy_channel", hierarchy_channel, true);
    for (const auto& it : hierarchy_ext_embedded_layer_index) {
        root->addElement(u"embedded_layer")->setIntAttribute(u"hierarchy_layer_index", it, true);
    }
}

bool ts::HEVCHierarchyExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xlayer;
    bool ok =
        element->getIntAttribute(extension_dimension_bits, u"extension_dimension_bits", true) &&
        element->getIntAttribute(hierarchy_layer_index, u"hierarchy_layer_index", true, 0, 0, 0x3F) &&
        element->getIntAttribute(temporal_id, u"temporal_id", true, 0, 0, 0x07) &&
        element->getIntAttribute(nuh_layer_id, u"nuh_layer_id", true, 0, 0, 0x3F) &&
        element->getBoolAttribute(tref_present, u"tref_present", true) &&
        element->getIntAttribute(hierarchy_channel, u"hierarchy_channel", true, 0, 0, 0x3F) &&
        element->getChildren(xlayer, u"embedded_layer", 0, 0x3F);

    for (auto it : xlayer) {
        uint8_t id = 0;
        ok = it->getIntAttribute(id, u"hierarchy_layer_index", true, 0, 0, 0x3F);
        hierarchy_ext_embedded_layer_index.push_back(id);
    }
    return ok;
}
