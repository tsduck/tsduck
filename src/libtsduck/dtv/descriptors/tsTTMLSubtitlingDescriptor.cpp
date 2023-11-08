//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTTMLSubtitlingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TTML_subtitling_descriptor"
#define MY_CLASS ts::TTMLSubtitlingDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_TTML_SUBTITLING
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TTMLSubtitlingDescriptor::TTMLSubtitlingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TTMLSubtitlingDescriptor::clearContent()
{
    language_code.clear();
    subtitle_purpose = 0;
    TTS_suitability = 0;
    dvb_ttml_profile.clear();
    qualifier.reset();
    font_id.clear();
    service_name.clear();
    reserved_zero_future_use_bytes = 0;
}

ts::TTMLSubtitlingDescriptor::TTMLSubtitlingDescriptor(DuckContext& duck, const Descriptor& desc) :
    TTMLSubtitlingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::TTMLSubtitlingDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TTMLSubtitlingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putBits(subtitle_purpose, 6);
    buf.putBits(TTS_suitability, 2);
    buf.putBit(!font_id.empty());
    buf.putBit(qualifier.has_value());
    buf.putBits(0x00, 2);
    buf.putBits(dvb_ttml_profile.size(), 4);
    for (auto it : dvb_ttml_profile) {
        buf.putUInt8(it);
    }
    if (qualifier.has_value()) {
        buf.putUInt32(qualifier.value());
    }
    if (!font_id.empty()) {
        buf.putBits(font_id.size(), 8);
        for (auto it : font_id) {
            buf.putBit(0x00);
            buf.putBits(it, 7);
        }
    }
    buf.putStringWithByteLength(service_name);
    for (size_t i = 0; i < reserved_zero_future_use_bytes; i++) {
        buf.putUInt8(0x00);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TTMLSubtitlingDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getBits(subtitle_purpose, 6);
    buf.getBits(TTS_suitability, 2);
    bool essential_font_usage_flag = buf.getBool();
    bool qualifier_present_flag = buf.getBool();
    buf.skipBits(2);
    uint8_t dvb_ttml_profile_count;
    buf.getBits(dvb_ttml_profile_count, 4);
    for (uint8_t i = 0; i < dvb_ttml_profile_count; i++) {
        dvb_ttml_profile.push_back(buf.getUInt8());
    }
    if (qualifier_present_flag) {
        qualifier = buf.getUInt32();
    }
    if (essential_font_usage_flag) {
        uint8_t t_font_id, font_count = buf.getUInt8();
        for (uint8_t i = 0; i < font_count; i++) {
            buf.skipBits(1);
            buf.getBits(t_font_id, 7);
            font_id.push_back(t_font_id);
        }
    }
    buf.getStringWithByteLength(service_name);
    reserved_zero_future_use_bytes = buf.remainingReadBytes();
    buf.skipBytes(reserved_zero_future_use_bytes);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

ts::UString ts::TTMLSubtitlingDescriptor::TTML_qualifier(uint32_t quali) {
    // EN 303 560 claise 5.2.1.3
    // note that the number of reserved bits is 18, not 26
    ts::UString res(u"size=");
    switch ((quali & 0xF0000000) >> 28) {
        case 0x0: res += u"default"; break;
        case 0x1: res += u"small"; break;
        case 0x2: res += u"medium"; break;
        case 0x3: res += u"large"; break;
        default: res += u"reserved";
    }
    res += u", cadence=";
    switch ((quali & 0x0F000000) >> 24) {
        case 0x0: res += u"default"; break;
        case 0x1: res += u"slow"; break;
        case 0x2: res += u"medium"; break;
        case 0x3: res += u"fast"; break;
        default: res += u"reserved";
    }
    res += u", ";
    res += ((quali & 0x00800000) ? u"monochrome" : u"coloured");

    if (quali & 0x00400000) {
        res += u", enhanced contrast";
    }
    res += u", position=";
    switch ((quali & 0x003C0000) >> 18) {
        case 0x0: res += u"default"; break;
        case 0x1: res += u"slow"; break;
        case 0x2: res += u"medium"; break;
        case 0x3: res += u"fast"; break;
        default: res += u"reserved";
    }
    return res;
}


ts::UString ts::TTMLSubtitlingDescriptor::TTML_subtitle_purpose(uint8_t purpose) {
    // EN 303 560 table 2
    ts::UString res(ts::UString::Format(u"0x%X (", {purpose}));
    switch (purpose) {
        case 0x00: res += u"same-lang-dialogue"; break;
        case 0x01: res += u"other-lang-dialogue"; break;
        case 0x02: res += u"all-dialogue"; break;
        case 0x10: res += u"hard-of-hearing"; break;
        case 0x11: res += u"other-lang-dialogue-with-hard-of-hearing"; break;
        case 0x12: res += u"all-dialogue-with-hard-of-hearing"; break;
        case 0x30: res += u"audio-description"; break;
        case 0x31: res += u"content-related-commentary"; break;
        default: res += u"reserved for future use";
    }
    res += u")";
    return res;
}

ts::UString ts::TTMLSubtitlingDescriptor::TTML_suitability(uint8_t suitability) {
    // EN 303 560 table 3
    ts::UString res(ts::UString::Format(u"0x%X (", {suitability}));
    switch (suitability) {
        case 0x0: res += u"unknown"; break;
        case 0x1: res += u"suitable"; break;
        case 0x2: res += u"not suitable"; break;
        default: res += u"reserved";
    }
    res += u")";
    return res;
}

void ts::TTMLSubtitlingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << "ISO 639 language code: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Subtitle purpose: " << TTML_subtitle_purpose(buf.getBits<uint8_t>(6));
        disp << ", TTS suitability: " << TTML_suitability(buf.getBits<uint8_t>(2)) << std::endl;
        bool essential_font_usage_flag = buf.getBool();
        bool qualifier_present_flag = buf.getBool();
        buf.skipReservedBits(2, 0);
        uint8_t i, dvb_ttml_profile_count;
        buf.getBits(dvb_ttml_profile_count, 4);
        if (dvb_ttml_profile_count > 0) {
            std::vector<uint8_t> subtitle_profiles;
            for (i = 0; i < dvb_ttml_profile_count; i++)
                subtitle_profiles.push_back(buf.getUInt8());
            disp.displayVector(u"DVB TTML profile:", subtitle_profiles, margin);
        }
        if (qualifier_present_flag) {
            const uint32_t qualifier = buf.getUInt32();
            disp << margin << "Qualifier: (" << UString::Hexa(qualifier) << ") " << TTML_qualifier(qualifier) << std::endl;
        }
        if (essential_font_usage_flag) {
            std::vector<uint8_t> essential_fonts;
            uint8_t font_count = buf.getUInt8();
            for (i = 0; i < font_count; i++) {
                buf.skipReservedBits(1, 0);
                essential_fonts.push_back(buf.getBits<uint8_t>(7));
            }
            disp.displayVector(u"Essential font IDs:", essential_fonts, margin);
        }
        UString service_name = buf.getStringWithByteLength();
        if (!service_name.empty()) {
            disp << margin << "Service Name: " << service_name << std::endl;
        }
        disp.displayPrivateData(u"reserved_zero_future_use", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TTMLSubtitlingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
     root->setAttribute(u"ISO_639_language_code", language_code);
     root->setIntAttribute(u"subtitle_purpose", subtitle_purpose);
     root->setIntAttribute(u"TTS_suitability", TTS_suitability);
     for (auto it : dvb_ttml_profile) {
         root->addElement(u"dvb_ttml_profile")->setIntAttribute(u"value", it, true);
     }
     root->setOptionalIntAttribute(u"qualifier", qualifier, true);
     for (auto it : font_id) {
         root->addElement(u"font_id")->setIntAttribute(u"value", it, true);
     }
     root->setAttribute(u"service_name", service_name, true);
     root->setIntAttribute(u"reserved_zero_future_count", reserved_zero_future_use_bytes);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TTMLSubtitlingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    uint8_t tmp;
    bool ok =
        element->getAttribute(language_code, u"ISO_639_language_code", true) &&
        element->getIntAttribute(subtitle_purpose, u"subtitle_purpose", true, 0x00, 0x00, 0x31) &&
        element->getIntAttribute(TTS_suitability, u"TTS_suitability", true, 0x00, 0x00, 0x02) &&
        element->getOptionalIntAttribute(qualifier, u"qualifier") &&
        element->getAttribute(service_name, u"service_name") &&
        element->getIntAttribute(reserved_zero_future_use_bytes, u"reserved_zero_future_count");

    if (ok && ((subtitle_purpose >= 0x03 && subtitle_purpose <= 0x0F) || (subtitle_purpose >= 0x13 && subtitle_purpose <= 0x2F) || (subtitle_purpose >= 0x32))) {
        element->report().error(u"value 0x%X in <%s>, line %d, is reserved.", { subtitle_purpose, element->name(), element->lineNumber() });
        ok = false;
    }
    ok &= element->getChildren(children, u"dvb_ttml_profile", 0x00, 0x0F);
    for (auto child : children) {
        ok &= child->getIntAttribute(tmp, u"value", true, 0, 0x00, 0x02);
        dvb_ttml_profile.push_back(tmp);
    }
    ok &= element->getChildren(children, u"font_id", 0x00, 0xFF);
    for (auto child : children) {
        ok &= child->getIntAttribute(tmp, u"value", true, 0, 0, 0x7F);
        font_id.push_back(tmp);
    }
    return ok;
}
