//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
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

#include "tsMPEG4TextDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#include <algorithm>

#define MY_XML_NAME u"MPEG-4_text_descriptor"
#define MY_CLASS ts::MPEG4TextDescriptor
#define MY_DID ts::DID_MPEG4_TEXT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

// ISO/IEC 14496-17 Table 1
const std::vector<uint8_t> ts::MPEG4TextDescriptor::allowed_textFormat_values{
    0x01,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7 ,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE
};

// ISO/IEC 14496-17 Table 5
const std::vector<uint8_t> ts::MPEG4TextDescriptor::allowed_3GPPBaseFormat_values {
    0x10
};

// ISO/IEC 14496-17 Table 6
const std::vector<uint8_t> ts::MPEG4TextDescriptor::allowed_profileLevel_values {
    0x10
};

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG4TextDescriptor::MPEG4TextDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    textFormat(0),
    textConfigLength(0),
    ThreeGPPBaseFormat(0),
    profileLevel(0),
    durationClock(0),
    sampleDescriptionFlags(0),
    layer(0),
    text_track_width(0),
    text_track_height(0),
    Compatible_3GPPFormat(),
    scene_width(),
    scene_height(),
    horizontal_scene_offset(),
    vertical_scene_offset(),
    Sample_index_and_description()
{
}

void ts::MPEG4TextDescriptor::clearContent()
{
    textFormat = 0;
    textConfigLength = 0;
    ThreeGPPBaseFormat = 0;
    profileLevel = 0;
    durationClock = 0;
    sampleDescriptionFlags = 0;
    layer = 0;
    text_track_width = 0;
    text_track_height = 0;
    Compatible_3GPPFormat.clear();
    scene_width.clear();
    scene_height.clear();
    horizontal_scene_offset.clear();
    vertical_scene_offset.clear();
    Sample_index_and_description.clear();
}

ts::MPEG4TextDescriptor::MPEG4TextDescriptor(DuckContext& duck, const Descriptor& desc) :
        MPEG4TextDescriptor()
{
    deserialize(duck, desc);
}

ts::Sample_index_and_description_type::Sample_index_and_description_type() :
    sample_index(0),
    SampleDescription()
{
}

ts::TextConfig_type::TextConfig_type() :
    textFormat(0),
    formatSpecificTextConfig()
{
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(textFormat);
    buf.putUInt16(textConfigLength);
    buf.putUInt8(ThreeGPPBaseFormat);
    buf.putUInt8(profileLevel);
    buf.putUInt24(durationClock);
    bool contains_list_of_compatible_3GPPFormats_flag = !Compatible_3GPPFormat.empty();
    buf.putBits(contains_list_of_compatible_3GPPFormats_flag, 1);
    buf.putBits(sampleDescriptionFlags, 2);
    bool SampleDescription_carriage_flag = !Sample_index_and_description.empty();
    buf.putBits(SampleDescription_carriage_flag, 1);
    bool positioning_information_flag = scene_width.set() || scene_height.set() || horizontal_scene_offset.set() || vertical_scene_offset.set();
    buf.putBits(positioning_information_flag, 1);
    buf.putBits(0xFF, 3);
    buf.putUInt8(layer);
    buf.putUInt16(text_track_width);
    buf.putUInt16(text_track_height);
    if (contains_list_of_compatible_3GPPFormats_flag) {
        buf.putUInt8(uint8_t(Compatible_3GPPFormat.size()));
        for (auto it : Compatible_3GPPFormat)
            buf.putUInt8(it);
    }
    if (SampleDescription_carriage_flag) {
        buf.putUInt8(uint8_t(Sample_index_and_description.size()));
        for (auto it : Sample_index_and_description) {
            buf.putUInt8(it.sample_index);
            buf.putUInt8(it.SampleDescription.textFormat);
            buf.putUInt16(uint16_t(it.SampleDescription.formatSpecificTextConfig.size()));
            buf.putBytes(it.SampleDescription.formatSpecificTextConfig);
        }
    }
    if (positioning_information_flag) {
        buf.putUInt16(scene_width.set() ? scene_width.value() : 0);
        buf.putUInt16(scene_height.set() ? scene_height.value() : 0);
        buf.putUInt16(horizontal_scene_offset.set() ? horizontal_scene_offset.value() : 0);
        buf.putUInt16(vertical_scene_offset.set() ? vertical_scene_offset.value() : 0);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::deserializePayload(PSIBuffer& buf)
{
    textFormat = buf.getUInt8();
    textConfigLength = buf.getUInt16();
    ThreeGPPBaseFormat = buf.getUInt8();
    profileLevel = buf.getUInt8();
    durationClock = buf.getUInt24();
    bool contains_list_of_compatible_3GPPFormats_flag = buf.getBool();
    buf.getBits(sampleDescriptionFlags, 2);
    bool SampleDescription_carriage_flag = buf.getBool();
    bool positioning_information_flag = buf.getBool();
    buf.skipBits(3);
    layer = buf.getUInt8();
    text_track_width = buf.getUInt16();
    text_track_height = buf.getUInt16();
    if (contains_list_of_compatible_3GPPFormats_flag) {
        uint8_t number_of_formats = buf.getUInt8();
        for (uint8_t i = 0; i < number_of_formats; i++)
            Compatible_3GPPFormat.push_back(buf.getUInt8());
    }
    if (SampleDescription_carriage_flag) {
        uint8_t number_of_SampleDescriptions = buf.getUInt8();;
        for (uint8_t i = 0; i < number_of_SampleDescriptions; i++) {
            Sample_index_and_description_type new_sample;
            new_sample.sample_index = buf.getUInt8();
            new_sample.SampleDescription.textFormat = buf.getUInt8();
            uint16_t _textConfigLength = buf.getUInt16();
            new_sample.SampleDescription.formatSpecificTextConfig = buf.getBytes(_textConfigLength);
            Sample_index_and_description.push_back(new_sample);
        }
    }
    if (positioning_information_flag) {
        scene_width = buf.getUInt16();
        scene_height = buf.getUInt16();
        horizontal_scene_offset = buf.getUInt16();
        vertical_scene_offset = buf.getUInt16();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

ts::UString ts::MPEG4TextDescriptor::TimedText_TS26245(ByteBlock formatSpecificTextConfig) 
{
    // TODO - format the paramater according to 3GPP TS 26.245
    ts::UString res(formatSpecificTextConfig);
    return res;
}

void ts::MPEG4TextDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    constexpr auto ITEMS_PER_LINE = 6;

    if (buf.canReadBytes(8)) {
        disp << margin << "Text format: " << DataName(MY_XML_NAME, u"textFormat", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", config length: " << buf.getUInt16() << std::endl;
        disp << margin << "3GPP base format: " << DataName(MY_XML_NAME, u"ThreeGPPBaseFormat", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", level: " << DataName(MY_XML_NAME, u"profileLevel", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", clock frequency: " << buf.getUInt24() << "Hz" << std::endl;
        bool contains_list_of_compatible_3GPPFormats_flag = buf.getBool();
        disp << margin << "Sample description: " << DataName(MY_XML_NAME, u"sampleDescriptionFlags", buf.getBits<uint8_t>(2), NamesFlags::VALUE) << std::endl;
        bool SampleDescription_carriage_flag = buf.getBool();
        bool positioning_information_flag = buf.getBool();
        buf.skipBits(3);
        disp << margin << "Layer: " << int(buf.getUInt8());
        disp << ", text track width=" << buf.getUInt16() << " height=" << buf.getUInt16() << std::endl;
        if (contains_list_of_compatible_3GPPFormats_flag) {
            uint8_t i, number_of_formats = buf.getUInt8();
            disp << margin << "Compatible 3GPP formats:";
            for (i = 0; i < number_of_formats; i++) {
                disp << " " << int(buf.getUInt8());
                if ((i + 1) % ITEMS_PER_LINE == 0) {
                    disp << std::endl;
                    if (i != (number_of_formats - 1))
                        disp << margin << "                        ";
                }
            }
            if (i % ITEMS_PER_LINE != 0)
                disp << std::endl;
        }
        if (SampleDescription_carriage_flag) {
            uint8_t i, number_of_SampleDescriptions = buf.getUInt8();
            for (i = 0; i < number_of_SampleDescriptions; i++) {
                disp << margin << UString::Format(u"Sample description[%d]: index=0x%X", { i, buf.getUInt8() });
                uint8_t textFormat = buf.getUInt8();
                disp << " format: " << DataName(MY_XML_NAME, u"textFormat", textFormat, NamesFlags::VALUE);
                uint16_t textConfigLength = buf.getUInt16();
                disp << " length: " << textConfigLength << std::endl;
                if (textFormat == 0x01)
                    disp << margin << TimedText_TS26245(buf.getBytes(textConfigLength));
                else disp << margin << buf.getBytes(textConfigLength);
                disp << std::endl;
            }
        }
        if (positioning_information_flag) {
            disp << margin << "Scene width=" << buf.getUInt16();
            disp << ", height=" << buf.getUInt16();
            disp << ", Scene offset horizontal=" << buf.getUInt16();
            disp << ", vertical=" << buf.getUInt16() << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"textFormat", textFormat);
    root->setIntAttribute(u"textConfigLength", textConfigLength);
    root->setIntAttribute(u"ThreeGPPBaseFormat", ThreeGPPBaseFormat);
    root->setIntAttribute(u"profileLevel", profileLevel);
    root->setIntAttribute(u"durationClock", durationClock);
    root->setIntAttribute(u"layer", layer);
    root->setIntAttribute(u"text_track_width", text_track_width);
    root->setIntAttribute(u"text_track_height", text_track_height);
    root->setOptionalIntAttribute(u"scene_width", scene_width);
    root->setOptionalIntAttribute(u"scene_height", scene_height);
    root->setOptionalIntAttribute(u"horizontal_scene_offset", horizontal_scene_offset);
    root->setOptionalIntAttribute(u"vertical_scene_offset", vertical_scene_offset);
    for (auto it : Compatible_3GPPFormat) {
        root->addElement(u"Compatible_3GPPFormat")->addText(UString::Format(u"%d", { it }));
    }
    for (auto it : Sample_index_and_description) {
        ts::xml::Element *newE = root->addElement(u"Sample_index_and_description");
        newE->setIntAttribute(u"sample_index", it.sample_index);
        newE->setIntAttribute(u"textFormat", it.SampleDescription.textFormat);
        newE->addHexaText(it.SampleDescription.formatSpecificTextConfig);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEG4TextDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector Compatible_3GPPFormat_children;
    xml::ElementVector Sample_index_and_description_children;
    bool ok =
        element->getIntAttribute(textFormat, u"textFormat", true) &&
        element->getIntAttribute(textConfigLength, u"textConfigLength", true) &&
        element->getIntAttribute(ThreeGPPBaseFormat, u"ThreeGPPBaseFormat", true) &&
        element->getIntAttribute(profileLevel, u"profileLevel", true) &&
        element->getIntAttribute(durationClock, u"durationClock", true) &&
        element->getIntAttribute(sampleDescriptionFlags, u"sampleDescriptionFlags", true) &&
        element->getOptionalIntAttribute(scene_width, u"scene_width") &&
        element->getOptionalIntAttribute(scene_height, u"scene_height") &&
        element->getOptionalIntAttribute(horizontal_scene_offset, u"horizontal_scene_offset") &&
        element->getOptionalIntAttribute(vertical_scene_offset, u"vertical_scene_offset") &&
        element->getChildren(Compatible_3GPPFormat_children, u"Compatible_3GPPFormat") &&
        element->getChildren(Sample_index_and_description_children, u"Sample_index_and_description");

    if (std::find(allowed_3GPPBaseFormat_values.begin(), allowed_3GPPBaseFormat_values.end(), ThreeGPPBaseFormat) == allowed_3GPPBaseFormat_values.end()) {
        element->report().error(u"line %d: in <%s>, attribute 'ThreeGPPBaseFormat' has a reserved value (0x%X)", { element->lineNumber(), element->name(), ThreeGPPBaseFormat });
        ok = false;
    }
    if (std::find(allowed_profileLevel_values.begin(), allowed_profileLevel_values.end(), profileLevel) == allowed_profileLevel_values.end()) {
        element->report().error(u"line %d: in <%s>, attribute 'profileLevel' has a reserved value (%d)", { element->lineNumber(), element->name(), profileLevel });
        ok = false;
    }
    uint8_t num_optionals = scene_width.set() + scene_height.set() + horizontal_scene_offset.set() + vertical_scene_offset.set();
    if (ok && (num_optionals > 0 && num_optionals < 4)) {
        element->report().error(u"line %d: in <%s>, attributes 'scene_width', 'scene_height', 'horizontal_scene_offset' and 'vertical_scene_offset' must all be present or all omitted", { element->lineNumber(), element->name() });
        ok = false;
    }
    for (auto it : Compatible_3GPPFormat_children) {
        uint8_t value;
        ok = it->value().UString::scan(u"%d", { &value });
        if (std::find(allowed_3GPPBaseFormat_values.begin(), allowed_3GPPBaseFormat_values.end(), value) == allowed_3GPPBaseFormat_values.end()) {
            element->report().error(u"line %d: in <%s>, element 'Compatible_3GPPFormat' has a reserved value (0x%X)", { element->lineNumber(), element->name(), value });
            ok = false;
        }
    }
    for (auto it : Sample_index_and_description_children) {
        uint8_t value;
        ok = it->getIntAttribute(value, u"textFormat");
        if (ok && std::find(allowed_textFormat_values.begin(), allowed_textFormat_values.end(), value) == allowed_textFormat_values.end()) {
            element->report().error(u"line %d: in <%s>, attribute 'textFormat' has a reserved value (0x%X)", { element->lineNumber(), element->name(), value });
            ok = false;
        }
    }
    return ok;
}
