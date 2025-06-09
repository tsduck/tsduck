//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsComponentDescriptor.h"
#include "tsDVBAC3Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"component_descriptor"
#define MY_CLASS    ts::ComponentDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_COMPONENT, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ComponentDescriptor::ComponentDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ComponentDescriptor::ComponentDescriptor(DuckContext& duck, const Descriptor& desc) :
    ComponentDescriptor()
{
    deserialize(duck, desc);
}

void ts::ComponentDescriptor::clearContent()
{
    stream_content_ext = 0;
    stream_content = 0;
    component_type = 0;
    component_tag = 0;
    language_code.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(stream_content_ext, 4);
    buf.putBits(stream_content, 4);
    buf.putUInt8(component_type);
    buf.putUInt8(component_tag);
    buf.putLanguageCode(language_code);
    buf.putString(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(stream_content_ext, 4);
    buf.getBits(stream_content, 4);
    component_type = buf.getUInt8();
    component_tag = buf.getUInt8();
    buf.getLanguageCode(language_code);
    buf.getString(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(6)) {
        const uint8_t stream_content_ext = buf.getBits<uint8_t>(4);
        const uint8_t stream_content = buf.getBits<uint8_t>(4);
        const uint8_t component_type = buf.getUInt8();
        disp << margin << "Content/type: " << ComponentTypeName(disp.duck(), stream_content, stream_content_ext, component_type, NamesFlags::VALUE_NAME) << std::endl;
        if (stream_content >= 0x1 && stream_content <= 0x8 && stream_content_ext != 0xF) {
            disp << margin << "  warning! stream_content_ext should be 0xF when stream_content is 0x1..0x8." << std::endl
                 << margin << "  (see note 1 in Table 26 of ETSI EN 300 468) " << std::endl;
        }
        if (stream_content_ext == 0xE && stream_content == 0xB) {
            DisplayNGAComponentFeatures(disp, margin + u"  ", component_type);
        }
        disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        if (buf.canRead()) {
            disp << margin << "Description: \"" << buf.getString() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display the feature flags of the NGA Component Descriptor value.
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::DisplayNGAComponentFeatures(TablesDisplay& disp, const UString& margin, uint8_t component_type)
{
    if (component_type & 0b01000000) {
        disp << margin << "content is pre-rendered for consumption with headphones" << std::endl;
    }
    std::vector<std::string> enables = {};
    if (component_type & 0b00100000) {
        enables.push_back("interactivity");
    }
    if (component_type & 0b00010000) {
        enables.push_back("dialogue enhancement");
    }
    if (!enables.empty()) {
        disp << margin << "content enables ";
        for (size_t i = 0; i < enables.size(); i++) {
            disp << ((i != 0 && i == enables.size() - 1) ? " and " : (i > 0 ? ", " : "")) << enables[i];
        }
        disp << std::endl;
    }
    std::vector<std::string> contains = {};
    if (component_type & 0b00001000) {
        contains.push_back("spoken subtitles");
    }
    if (component_type & 0b00000100) {
        contains.push_back("audio description");
    }
    if (!contains.empty()) {
        disp << margin << "content contains ";
        for (size_t i = 0; i < contains.size(); i++) {
            disp << ((i != 0 && i == contains.size() - 1) ? " and " : (i > 0 ? ", " : "")) << contains[i];
        }
        disp << std::endl;
    }
    disp << margin;
    switch (component_type & 0b00000011) {
        case 0:
            disp << "no preferred reproduction";
            break;
        case 1:
            disp << "stereo";
            break;
        case 2:
            disp << "two-dimensional";
            break;
        case 3:
            disp << "three dimensional";
            break;
        default:
            break;
    }
    disp << " channel layout" << std::endl;
}


//----------------------------------------------------------------------------
// Name of a Component Type.
//----------------------------------------------------------------------------

ts::UString ts::ComponentDescriptor::ComponentTypeName(const DuckContext& duck, uint8_t stream_content, uint8_t stream_content_ext, uint8_t component_type, NamesFlags flags, size_t bits)
{
    // There is a special case here. The binary layout of the 16 bits in the .names
    // file is based on table 26 (component_descriptor) in ETSI EN 300 468.
    //
    //   stream_content (4 bits) || stream_content_ext (4 bits) || component_type (8 bits).
    //
    // In the beginning, stream_content_ext did not exist and, as a reserved field, was 0xF.
    // Starting with stream_content > 8, stream_content_ext appeared and may have different
    // values. Logically, stream_content_ext is a subsection of stream_content but the memory
    // layout in a binary component_descriptor is:
    //
    //   stream_content_ext (4 bits) || stream_content (4 bits) || component_type (8 bits).

    // Stream content and extension use 4 bits.
    stream_content &= 0x0F;
    uint8_t lookup_stream_content_ext = (stream_content >= 1 && stream_content <= 8) ? 0x0F : (stream_content_ext & 0x0F);

    // Value to use for name lookup:
    const uint16_t nType = uint16_t((uint16_t(stream_content) << 12) | (uint16_t(lookup_stream_content_ext) << 8) | component_type);

    // Value to display, we use the real binary value for stream_content_ext
    const uint16_t dType = uint16_t((uint16_t(stream_content_ext) << 12) | (uint16_t(stream_content) << 8) | component_type);

    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return DataName(MY_XML_NAME, u"component_type.japan", nType, flags | NamesFlags::ALTERNATE, dType, bits);
    }
    else if (stream_content == 4) {
        return Names::Format(dType, AC3Descriptor::ComponentTypeName(uint8_t(nType)), flags, 16);
    }
    else {
        return DataName(MY_XML_NAME, u"component_type", nType, flags | NamesFlags::ALTERNATE, dType, bits);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"stream_content", stream_content, true);
    root->setIntAttribute(u"stream_content_ext", stream_content_ext, true);
    root->setIntAttribute(u"component_type", component_type, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setAttribute(u"language_code", language_code);
    root->setAttribute(u"text", text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ComponentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =  element->getIntAttribute(stream_content, u"stream_content", true, 0x00, 0x00, 0x0F) &&
               element->getIntAttribute(stream_content_ext, u"stream_content_ext", false, 0x0F, 0x00, 0x0F) &&
               element->getIntAttribute(component_type, u"component_type", true, 0x00, 0x00, 0xFF) &&
               element->getIntAttribute(component_tag, u"component_tag", false, 0x00, 0x00, 0xFF) &&
               element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
               element->getAttribute(text, u"text", false, u"", 0, MAX_DESCRIPTOR_SIZE - 8);
    if (ok && (stream_content >= 0x1 && stream_content <= 0x8 && stream_content_ext != 0xF)) {
        element->report().warning(u"stream_content_ext should be 0xF when stream_content is 0x1..0x8 (see note 1 in Table 26 of ETSI EN 300 468), in <%s> line %d", element->name(), element->lineNumber());
    }
    return ok;
}
