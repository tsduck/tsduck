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

#include "tsAudioComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    stream_content(2),  // audio content
    component_type(0),
    component_tag(0),
    stream_type(0),
    simulcast_group_tag(0xFF), // no simulcast
    main_component(true),
    quality_indicator(0),
    sampling_rate(0),
    ISO_639_language_code(),
    ISO_639_language_code_2(),
    text()
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
    stream_content = buf.getBits<uint8_t>(4);
    component_type = buf.getUInt8();
    component_tag = buf.getUInt8();
    stream_type = buf.getUInt8();
    simulcast_group_tag = buf.getUInt8();
    const bool multi = buf.getBit() != 0;
    main_component = buf.getBit() != 0;
    quality_indicator = buf.getBits<uint8_t>(2);
    sampling_rate = buf.getBits<uint8_t>(3);
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

void ts::AudioComponentDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 9) {
        const bool multi = (data[5] & 0x80) != 0;
        disp << margin << UString::Format(u"Content type: 0x%X (%d)", {data[0] & 0x0F, data[0] & 0x0F}) << std::endl
             << margin << "Component type: " << NameFromSection(u"ISDBAudioComponentType", data[1], names::FIRST) << std::endl
             << margin << UString::Format(u"Component tag: 0x%X (%d)", {data[2], data[2]}) << std::endl
             << margin << "Stream type: " << names::StreamType(data[3], names::FIRST) << std::endl
             << margin << UString::Format(u"Simulcast group: 0x%X (%d%s)", {data[4], data[4], data[4] == 0xFF ? u", none" : u""}) << std::endl
             << margin << UString::Format(u"Main component: %s", {(data[5] & 0x40) != 0}) << std::endl
             << margin << "Quality indicator: " << NameFromSection(u"ISDBAudioQuality", (data[5] >> 4) & 0x03, names::FIRST) << std::endl
             << margin << "Sampling rate: " << NameFromSection(u"ISDBAudioSampling", (data[5] >> 1) & 0x07, names::FIRST) << std::endl
             << margin << "Language code: \"" << DeserializeLanguageCode(data + 6) << "\"" << std::endl;
        data += 9; size -= 9;
        if (multi && size >= 3) {
            disp << margin << "Language code 2: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        disp << margin << "Text: \"" << disp.duck().decoded(data, size) << "\"" << std::endl;
    }
    else {
        disp.displayExtraData(data, size, margin);
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
    return  element->getIntAttribute<uint8_t>(stream_content, u"stream_content", false, 0x02, 0x00, 0x0F) &&
            element->getIntAttribute<uint8_t>(component_type, u"component_type", true) &&
            element->getIntAttribute<uint8_t>(component_tag, u"component_tag", true) &&
            element->getIntAttribute<uint8_t>(stream_type, u"stream_type", true) &&
            element->getIntAttribute<uint8_t>(simulcast_group_tag, u"simulcast_group_tag", false, 0xFF) &&
            element->getBoolAttribute(main_component, u"main_component", false, true) &&
            element->getIntAttribute<uint8_t>(quality_indicator, u"quality_indicator", true, 0, 0, 3) &&
            element->getIntAttribute<uint8_t>(sampling_rate, u"sampling_rate", true, 0, 0, 7) &&
            element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, UString(), 3, 3) &&
            element->getAttribute(ISO_639_language_code_2, u"ISO_639_language_code_2", false, UString(), 3, 3) &&
            element->getAttribute(text, u"text");
}
