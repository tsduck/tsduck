//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"splice_audio_descriptor"
#define MY_CLASS    ts::SpliceAudioDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_SPLICE_AUDIO, ts::Standards::SCTE, ts::TID_SCTE35_SIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceAudioDescriptor::SpliceAudioDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SpliceAudioDescriptor::clearContent()
{
    identifier = SPLICE_ID_CUEI;
    audio.clear();
}

ts::SpliceAudioDescriptor::SpliceAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    SpliceAudioDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceAudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(identifier);
    buf.putBits(audio.size(), 4);
    buf.putReserved(4);
    for (const auto& a : audio) {
        buf.putUInt8(a.component_tag);
        buf.putLanguageCode(a.ISO_code);
        buf.putBits(a.Bit_Stream_Mode, 3);
        buf.putBits(a.Num_Channels, 4);
        buf.putBit(a.Full_Srvc_Audio);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceAudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    identifier = buf.getUInt32();
    audio.resize(buf.getBits<size_t>(4));
    buf.skipReservedBits(4);
    for (auto& a : audio) {
        a.component_tag = buf.getUInt8();
        buf.getLanguageCode(a.ISO_code);
        buf.getBits(a.Bit_Stream_Mode, 3);
        buf.getBits(a.Num_Channels, 4);
        a.Full_Srvc_Audio = buf.getBool();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        // Sometimes, the identifiers are made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Identifier: 0x%08X", buf, 4, margin);
        const size_t audio_count = buf.getBits<size_t>(4);
        buf.skipReservedBits(4);
        disp << margin << "Audio count: " << audio_count << std::endl;
        for (size_t i = 0; i < audio_count; ++i) {
            disp << margin << UString::Format(u"- Component tag: %n", buf.getUInt8());
            disp << ", language: " << buf.getLanguageCode() << std::endl;
            disp << margin << UString::Format(u"  Bit stream mode: %n", buf.getBits<uint8_t>(3));
            disp << UString::Format(u", num channels: %d", buf.getBits<uint8_t>(4));
            disp << ", full service: " << UString::TrueFalse(buf.getBool()) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceAudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    for (const auto& a : audio) {
        xml::Element* e = root->addElement(u"audio");
        e->setIntAttribute(u"component_tag", a.component_tag);
        e->setAttribute(u"ISO_code", a.ISO_code);
        e->setIntAttribute(u"Bit_Stream_Mode", a.Bit_Stream_Mode);
        e->setIntAttribute(u"Num_Channels", a.Num_Channels);
        e->setBoolAttribute(u"Full_Srvc_Audio", a.Full_Srvc_Audio);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceAudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getIntAttribute(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
              element->getChildren(children, u"audio", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Audio a;
        ok = children[i]->getIntAttribute(a.component_tag, u"component_tag", true) &&
             children[i]->getAttribute(a.ISO_code, u"ISO_code", true, u"", 3, 3) &&
             children[i]->getIntAttribute(a.Bit_Stream_Mode, u"Bit_Stream_Mode", true, 0, 0, 0x07) &&
             children[i]->getIntAttribute(a.Num_Channels, u"Num_Channels", true, 0, 0, 0x0F) &&
             children[i]->getBoolAttribute(a.Full_Srvc_Audio, u"Full_Srvc_Audio", true);
        audio.push_back(a);
    }
    return ok;
}
