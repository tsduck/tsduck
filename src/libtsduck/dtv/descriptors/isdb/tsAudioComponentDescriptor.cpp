//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAudioComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"audio_component_descriptor"
#define MY_CLASS ts::AudioComponentDescriptor
#define MY_DID ts::DID_ISDB_AUDIO_COMP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AudioComponentDescriptor::AudioComponentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::AudioComponentDescriptor::AudioComponentDescriptor(DuckContext& duck, const Descriptor& desc) :
    AudioComponentDescriptor()
{
    deserialize(duck, desc);
}

void ts::AudioComponentDescriptor::clearContent()
{
    stream_content = 2;  // audio content
    component_type = 0;
    component_tag = 0;
    stream_type = 0;
    simulcast_group_tag = 0xFF; // no simulcast
    main_component = true;
    quality_indicator = 0;
    sampling_rate = 0;
    ISO_639_language_code.clear();
    ISO_639_language_code_2.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AudioComponentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 4);
    buf.putBits(stream_content, 4);
    buf.putUInt8(component_type);
    buf.putUInt8(component_tag);
    buf.putUInt8(stream_type);
    buf.putUInt8(simulcast_group_tag);
    buf.putBit(!ISO_639_language_code_2.empty());
    buf.putBit(main_component);
    buf.putBits(quality_indicator, 2);
    buf.putBits(sampling_rate, 3);
    buf.putBit(1);
    buf.putLanguageCode(ISO_639_language_code);
    if (!ISO_639_language_code_2.empty()) {
        buf.putLanguageCode(ISO_639_language_code_2);
    }
    buf.putString(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AudioComponentDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(4);
    buf.getBits(stream_content, 4);
    component_type = buf.getUInt8();
    component_tag = buf.getUInt8();
    stream_type = buf.getUInt8();
    simulcast_group_tag = buf.getUInt8();
    const bool multi = buf.getBool();
    main_component = buf.getBool();
    buf.getBits(quality_indicator, 2);
    buf.getBits(sampling_rate, 3);
    buf.skipBits(1);
    buf.getLanguageCode(ISO_639_language_code);
    if (multi) {
        buf.getLanguageCode(ISO_639_language_code_2);
    }
    buf.getString(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AudioComponentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(9)) {
        buf.skipBits(4);
        disp << margin << UString::Format(u"Content type: 0x%X (%<d)", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp << margin << "Component type: " << DataName(MY_XML_NAME, u"component_type", buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << "Stream type: " << names::StreamType(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        const uint8_t group = buf.getUInt8();
        disp << margin << UString::Format(u"Simulcast group: 0x%X (%<d%s)", {group, group == 0xFF ? u", none" : u""}) << std::endl;
        const bool multi = buf.getBool();
        disp << margin << UString::Format(u"Main component: %s", {buf.getBool()}) << std::endl;
        disp << margin << "Quality indicator: " << DataName(MY_XML_NAME, u"Quality", buf.getBits<uint8_t>(2), NamesFlags::FIRST) << std::endl;
        disp << margin << "Sampling rate: " << DataName(MY_XML_NAME, u"Sampling", buf.getBits<uint8_t>(3), NamesFlags::FIRST) << std::endl;
        buf.skipBits(1);
        disp << margin << "Language code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        if (multi && buf.canReadBytes(3)) {
            disp << margin << "Language code 2: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        disp << margin << "Text: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AudioComponentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"stream_content", stream_content, true);
    root->setIntAttribute(u"component_type", component_type, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setIntAttribute(u"stream_type", stream_type, true);
    if (simulcast_group_tag != 0xFF) {
        root->setIntAttribute(u"simulcast_group_tag", simulcast_group_tag, true);
    }
    root->setBoolAttribute(u"main_component", main_component);
    root->setIntAttribute(u"quality_indicator", quality_indicator);
    root->setIntAttribute(u"sampling_rate", sampling_rate);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->setAttribute(u"ISO_639_language_code_2", ISO_639_language_code_2, true);
    root->setAttribute(u"text", text, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AudioComponentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(stream_content, u"stream_content", false, 0x02, 0x00, 0x0F) &&
            element->getIntAttribute(component_type, u"component_type", true) &&
            element->getIntAttribute(component_tag, u"component_tag", true) &&
            element->getIntAttribute(stream_type, u"stream_type", true) &&
            element->getIntAttribute(simulcast_group_tag, u"simulcast_group_tag", false, 0xFF) &&
            element->getBoolAttribute(main_component, u"main_component", false, true) &&
            element->getIntAttribute(quality_indicator, u"quality_indicator", true, 0, 0, 3) &&
            element->getIntAttribute(sampling_rate, u"sampling_rate", true, 0, 0, 7) &&
            element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, UString(), 3, 3) &&
            element->getAttribute(ISO_639_language_code_2, u"ISO_639_language_code_2", false, UString(), 3, 3) &&
            element->getAttribute(text, u"text");
}
