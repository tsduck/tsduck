//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG4TextDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsAlgorithm.h"

#define MY_XML_NAME u"MPEG4_text_descriptor"
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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEG4TextDescriptor::clearContent()
{
    textFormat = 0;
    ThreeGPPBaseFormat = 0;
    profileLevel = 0;
    durationClock = 0;
    sampleDescriptionFlags = 0;
    layer = 0;
    text_track_width = 0;
    text_track_height = 0;
    Compatible_3GPPFormat.clear();
    scene_width.reset();
    scene_height.reset();
    horizontal_scene_offset.reset();
    vertical_scene_offset.reset();
    Sample_index_and_description.clear();
}

ts::MPEG4TextDescriptor::MPEG4TextDescriptor(DuckContext& duck, const Descriptor& desc) :
        MPEG4TextDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(textFormat);
    buf.pushWriteSequenceWithLeadingLength(16); // textConfigLength
    buf.putUInt8(ThreeGPPBaseFormat);
    buf.putUInt8(profileLevel);
    buf.putUInt24(durationClock);
    const bool contains_list_of_compatible_3GPPFormats_flag = !Compatible_3GPPFormat.empty();
    buf.putBits(contains_list_of_compatible_3GPPFormats_flag, 1);
    buf.putBits(sampleDescriptionFlags, 2);
    const bool SampleDescription_carriage_flag = !Sample_index_and_description.empty();
    buf.putBits(SampleDescription_carriage_flag, 1);
    const bool positioning_information_flag = scene_width.has_value() || scene_height.has_value() || horizontal_scene_offset.has_value() || vertical_scene_offset.has_value();
    buf.putBits(positioning_information_flag, 1);
    buf.putBits(0xFF, 3);
    buf.putUInt8(layer);
    buf.putUInt16(text_track_width);
    buf.putUInt16(text_track_height);
    if (contains_list_of_compatible_3GPPFormats_flag) {
        buf.putUInt8(uint8_t(Compatible_3GPPFormat.size()));
        buf.putBytes(Compatible_3GPPFormat);
    }
    if (SampleDescription_carriage_flag) {
        buf.putUInt8(uint8_t(Sample_index_and_description.size()));
        for (const auto& it : Sample_index_and_description) {
            buf.putUInt8(it.sample_index);
            buf.putUInt8(it.SampleDescription.textFormat);
            buf.putUInt16(uint16_t(it.SampleDescription.formatSpecificTextConfig.size()));
            buf.putBytes(it.SampleDescription.formatSpecificTextConfig);
        }
    }
    if (positioning_information_flag) {
        buf.putUInt16(scene_width.value_or(0));
        buf.putUInt16(scene_height.value_or(0));
        buf.putUInt16(horizontal_scene_offset.value_or(0));
        buf.putUInt16(vertical_scene_offset.value_or(0));
    }
    buf.popState(); // update textConfigLength
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::deserializePayload(PSIBuffer& buf)
{
    textFormat = buf.getUInt8();
    buf.pushReadSizeFromLength(16); // textConfigLength
    ThreeGPPBaseFormat = buf.getUInt8();
    profileLevel = buf.getUInt8();
    durationClock = buf.getUInt24();
    const bool contains_list_of_compatible_3GPPFormats_flag = buf.getBool();
    buf.getBits(sampleDescriptionFlags, 2);
    const bool SampleDescription_carriage_flag = buf.getBool();
    const bool positioning_information_flag = buf.getBool();
    buf.skipBits(3);
    layer = buf.getUInt8();
    text_track_width = buf.getUInt16();
    text_track_height = buf.getUInt16();
    if (contains_list_of_compatible_3GPPFormats_flag) {
        const uint8_t number_of_formats = buf.getUInt8();
        buf.getBytes(Compatible_3GPPFormat, number_of_formats);
    }
    if (SampleDescription_carriage_flag) {
        const uint8_t number_of_SampleDescriptions = buf.getUInt8();
        for (uint8_t i = 0; i < number_of_SampleDescriptions; i++) {
            Sample_index_and_description_type new_sample;
            new_sample.sample_index = buf.getUInt8();
            new_sample.SampleDescription.textFormat = buf.getUInt8();
            const uint16_t textConfigLength = buf.getUInt16();
            buf.getBytes(new_sample.SampleDescription.formatSpecificTextConfig, textConfigLength);
            Sample_index_and_description.push_back(new_sample);
        }
    }
    if (positioning_information_flag) {
        scene_width = buf.getUInt16();
        scene_height = buf.getUInt16();
        horizontal_scene_offset = buf.getUInt16();
        vertical_scene_offset = buf.getUInt16();
    }
    buf.popState(); // end of textConfigLength
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

ts::UString ts::MPEG4TextDescriptor::TimedText_TS26245(ByteBlock formatSpecificTextConfig)
{
    // TODO - format the paramater according to 3GPP TS 26.245
    return UString::Dump(formatSpecificTextConfig, UString::SINGLE_LINE);
}

void ts::MPEG4TextDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        disp << margin << "Text format: " << DataName(MY_XML_NAME, u"textFormat", buf.getUInt8(), NamesFlags::VALUE);
        buf.pushReadSizeFromLength(16); // textConfigLength
        disp << ", config length: " << buf.remainingReadBytes() << std::endl;
        disp << margin << "3GPP base format: " << DataName(MY_XML_NAME, u"ThreeGPPBaseFormat", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", level: " << DataName(MY_XML_NAME, u"profileLevel", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", clock frequency: " << UString::Decimal(buf.getUInt24()) << " Hz" << std::endl;
        const bool contains_list_of_compatible_3GPPFormats_flag = buf.getBool();
        disp << margin << "Sample description: " << DataName(MY_XML_NAME, u"sampleDescriptionFlags", buf.getBits<uint8_t>(2), NamesFlags::VALUE) << std::endl;
        const bool SampleDescription_carriage_flag = buf.getBool();
        const bool positioning_information_flag = buf.getBool();
        buf.skipBits(3);  // ISO/IEC 14496-17 is not explicit on the value of reserved buts
        disp << margin << "Layer: " << int(buf.getUInt8());
        disp << ", text track width=" << buf.getUInt16();
        disp << ", height=" << buf.getUInt16() << std::endl;
        if (contains_list_of_compatible_3GPPFormats_flag) {
            const uint8_t number_of_formats = buf.getUInt8();
            std::vector<uint8_t> Compatible3GPPFormats;
            for (uint8_t i = 0; i < number_of_formats; i++) {
                Compatible3GPPFormats.push_back(buf.getUInt8());
            }
            disp.displayVector(u"Compatible 3GPP formats:", Compatible3GPPFormats, margin);
        }
        if (SampleDescription_carriage_flag) {
            const uint8_t number_of_SampleDescriptions = buf.getUInt8();
            for (uint8_t i = 0; i < number_of_SampleDescriptions; i++) {
                disp << margin << UString::Format(u"Sample description[%d]: index=0x%X", {i, buf.getUInt8()});
                const uint8_t textFormat = buf.getUInt8();
                disp << ", format: " << DataName(MY_XML_NAME, u"textFormat", textFormat, NamesFlags::VALUE);
                const uint16_t textConfigLength = buf.getUInt16();
                disp << ", length: " << textConfigLength << std::endl;
                if (textConfigLength > 0) {
                    if (textFormat == 0x01) {
                        // Need to extract string first because of issue with MSVC in C++20 mode.
                        const UString line(TimedText_TS26245(buf.getBytes(textConfigLength)));
                        disp << margin << line;
                    }
                    else {
                        disp << margin << UString::Dump(buf.getBytes(textConfigLength), UString::SINGLE_LINE);
                    }
                    disp << std::endl;
                }
            }
        }
        if (positioning_information_flag) {
            disp << margin << "Scene width=" << buf.getUInt16();
            disp << ", height=" << buf.getUInt16();
            disp << ", Scene offset horizontal=" << buf.getUInt16();
            disp << ", vertical=" << buf.getUInt16() << std::endl;
        }
        buf.popState(); // end of textConfigLength
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG4TextDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"textFormat", textFormat);
    root->setIntAttribute(u"ThreeGPPBaseFormat", ThreeGPPBaseFormat, true);
    root->setIntAttribute(u"profileLevel", profileLevel, true);
    root->setIntAttribute(u"durationClock", durationClock);
    root->setIntAttribute(u"sampleDescriptionFlags", sampleDescriptionFlags);
    root->setIntAttribute(u"layer", layer, true);
    root->setIntAttribute(u"text_track_width", text_track_width);
    root->setIntAttribute(u"text_track_height", text_track_height);
    root->setOptionalIntAttribute(u"scene_width", scene_width);
    root->setOptionalIntAttribute(u"scene_height", scene_height);
    root->setOptionalIntAttribute(u"horizontal_scene_offset", horizontal_scene_offset);
    root->setOptionalIntAttribute(u"vertical_scene_offset", vertical_scene_offset);
    for (auto it : Compatible_3GPPFormat) {
        root->addElement(u"Compatible_3GPPFormat")->setIntAttribute(u"value", it);
    }
    for (const auto& it : Sample_index_and_description) {
        xml::Element* e = root->addElement(u"Sample_index_and_description");
        e->setIntAttribute(u"sample_index", it.sample_index);
        e->setIntAttribute(u"textFormat", it.SampleDescription.textFormat);
        e->addHexaText(it.SampleDescription.formatSpecificTextConfig, true);
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
        element->getIntAttribute(ThreeGPPBaseFormat, u"ThreeGPPBaseFormat", true) &&
        element->getIntAttribute(profileLevel, u"profileLevel", true) &&
        element->getIntAttribute(durationClock, u"durationClock", true, 0, 0, 0x00FFFFFF) &&
        element->getIntAttribute(sampleDescriptionFlags, u"sampleDescriptionFlags", true, 0, 0, 3) &&
        element->getIntAttribute(layer, u"layer", true) &&
        element->getIntAttribute(text_track_width, u"text_track_width", true) &&
        element->getIntAttribute(text_track_height, u"text_track_height", true) &&
        element->getOptionalIntAttribute(scene_width, u"scene_width") &&
        element->getOptionalIntAttribute(scene_height, u"scene_height") &&
        element->getOptionalIntAttribute(horizontal_scene_offset, u"horizontal_scene_offset") &&
        element->getOptionalIntAttribute(vertical_scene_offset, u"vertical_scene_offset") &&
        element->getChildren(Compatible_3GPPFormat_children, u"Compatible_3GPPFormat") &&
        element->getChildren(Sample_index_and_description_children, u"Sample_index_and_description");

    if (!Contains(allowed_3GPPBaseFormat_values, ThreeGPPBaseFormat)) {
        element->report().error(u"line %d: in <%s>, attribute 'ThreeGPPBaseFormat' has a reserved value (0x%X)", {element->lineNumber(), element->name(), ThreeGPPBaseFormat});
        ok = false;
    }
    if (!Contains(allowed_profileLevel_values, profileLevel)) {
        element->report().error(u"line %d: in <%s>, attribute 'profileLevel' has a reserved value (%d)", {element->lineNumber(), element->name(), profileLevel});
        ok = false;
    }
    const uint8_t num_optionals = scene_width.has_value() + scene_height.has_value() + horizontal_scene_offset.has_value() + vertical_scene_offset.has_value();
    if (ok && (num_optionals > 0 && num_optionals < 4)) {
        element->report().error(u"line %d: in <%s>, attributes 'scene_width', 'scene_height', 'horizontal_scene_offset' and 'vertical_scene_offset' must all be present or all omitted", {element->lineNumber(), element->name()});
        ok = false;
    }
    for (auto it : Compatible_3GPPFormat_children) {
        uint8_t value = 0;
        ok &= it->getIntAttribute(value, u"value", true);
        if (!Contains(allowed_3GPPBaseFormat_values, value)) {
            element->report().error(u"line %d: in <%s>, element 'Compatible_3GPPFormat' has a reserved value (0x%X)", {element->lineNumber(), element->name(), value});
            ok = false;
        }
        Compatible_3GPPFormat.push_back(value);
    }
    for (auto it : Sample_index_and_description_children) {
        Sample_index_and_description_type sample;
        ok = ok &&
             it->getIntAttribute(sample.sample_index, u"sample_index", true) &&
             it->getIntAttribute(sample.SampleDescription.textFormat, u"textFormat") &&
             it->getHexaText(sample.SampleDescription.formatSpecificTextConfig);
        if (ok && !Contains(allowed_textFormat_values, sample.SampleDescription.textFormat)) {
            element->report().error(u"line %d: in <%s>, attribute 'textFormat' has a reserved value (0x%X)", {element->lineNumber(), element->name(), sample.SampleDescription.textFormat});
            ok = false;
        }
        Sample_index_and_description.push_back(sample);
    }
    return ok;
}
