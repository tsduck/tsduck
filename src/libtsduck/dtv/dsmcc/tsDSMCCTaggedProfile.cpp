//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCTaggedProfile.h"
#include "tsNames.h"

//----------------------------------------------------------------------------
// TaggedProfile
//----------------------------------------------------------------------------

void ts::DSMCCTaggedProfile::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(profile_id_tag);
    buf.pushWriteSequenceWithLeadingLength(32);  // profile_data
    buf.putUInt8(profile_data_byte_order);

    if (profile_id_tag == DSMCC_TAG_BIOP) {  // TAG_BIOP (BIOP Profile Body)
        buf.putUInt8(uint8_t(lite_components.size()));
        for (const auto& lite_component : lite_components) {
            lite_component.serialize(buf);
        }
    }
    else if (profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
        if (profile_data.has_value()) {
            buf.putBytes(profile_data.value());
        }
    }
    else {  // UnknownComponent
        if (profile_data.has_value()) {
            buf.putBytes(profile_data.value());
        }
    }

    buf.popState();
}

void ts::DSMCCTaggedProfile::deserialize(PSIBuffer& buf)
{
    profile_id_tag = buf.getUInt32();

    buf.pushReadSizeFromLength(32);  //profile_data_length

    profile_data_byte_order = buf.getUInt8();

    if (profile_id_tag == DSMCC_TAG_BIOP) {  // TAG_BIOP (BIOP Profile Body)
        const uint8_t lite_component_count = buf.getUInt8();
        for (size_t i = 0; i < lite_component_count; i++) {
            lite_components.emplace_back().deserialize(buf);
        }
    }
    else {
        profile_data = buf.getBytes();
    }
    buf.popState();
}

void ts::DSMCCTaggedProfile::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    const uint32_t profile_id_tag = buf.getUInt32();

    buf.pushReadSizeFromLength(32);  //profile_data_length

    const uint8_t profile_data_byte_order = buf.getUInt8();

    disp << margin << "ProfileId Tag: " << NameFromSection(u"dtv", u"DSMCC_user_to_network_message.tag", profile_id_tag, NamesFlags::HEX_VALUE_NAME) << std::endl;
    disp << margin << UString::Format(u"Profile Data Byte Order: %n", profile_data_byte_order) << std::endl;

    if (profile_id_tag == DSMCC_TAG_BIOP) {  // TAG_BIOP (BIOP Profile Body)
                                             //
        const uint8_t lite_component_count = buf.getUInt8();

        disp << margin << UString::Format(u"Lite Component Count: %n", lite_component_count) << std::endl;

        for (size_t i = 0; i < lite_component_count; i++) {
            DSMCCLiteComponent::Display(disp, buf, margin);
        }
    }
    else if (profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
        disp.displayPrivateData(u"Lite Options Profile Body Data", buf, NPOS, margin);
    }
    else {
        disp.displayPrivateData(u"Unknown Profile Data", buf, NPOS, margin);
    }
    buf.popState();
}

void ts::DSMCCTaggedProfile::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* tagged_profile = parent->addElement(u"tagged_profile");
    tagged_profile->setIntAttribute(u"profile_id_tag", profile_id_tag, true);
    tagged_profile->setIntAttribute(u"profile_data_byte_order", profile_data_byte_order, true);

    if (profile_id_tag == DSMCC_TAG_BIOP) {
        xml::Element* biop_profile_body = tagged_profile->addElement(u"BIOP_profile_body");
        for (const auto& lite_component : lite_components) {
            lite_component.toXML(duck, biop_profile_body);
        }
    }
    else if (profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TAG_LITE_OPTIONS (Lite Options Profile Body)
        xml::Element* lite_options_profile = tagged_profile->addElement(u"Lite_options_profile_body");

        if (profile_data.has_value()) {
            lite_options_profile->addHexaTextChild(u"profile_data", profile_data.value(), true);
        }
    }
    else {
        xml::Element* unknown_profile = tagged_profile->addElement(u"Unknown_profile");

        if (profile_data.has_value()) {
            unknown_profile->addHexaTextChild(u"profile_data", profile_data.value(), true);
        }
    }
}

bool ts::DSMCCTaggedProfile::fromXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(profile_id_tag, u"profile_id_tag", true) &&
        element->getIntAttribute(profile_data_byte_order, u"profile_data_byte_order", true);
    if (!ok)
        return false;

    if (profile_id_tag == DSMCC_TAG_BIOP) {  // TAG_BIOP (BIOP Profile Body):w
        const xml::Element* biop_profile_body = element->findFirstChild(u"BIOP_profile_body", true);
        if (biop_profile_body != nullptr) {
            for (auto& lite_component : biop_profile_body->children(u"lite_component", &ok)) {
                ok = lite_components.emplace_back().fromXML(duck, &lite_component);
            }
        }
    }
    else if (profile_id_tag == DSMCC_TAG_LITE_OPTIONS) {  // TODO: TAG_LITE_OPTIONS (Lite Options Profile Body)
        const xml::Element* lite_options_profile_body = element->findFirstChild(u"Lite_options_profile_body");
        if (lite_options_profile_body == nullptr) {
            return false;
        }

        ByteBlock temp {};
        if (lite_options_profile_body->getHexaTextChild(temp, u"profile_data")) {
            profile_data = temp;
        }
    }
    else {  // Any other profile context
        const xml::Element* unknown_profile_body = element->findFirstChild(u"Unknown_profile_body");
        if (unknown_profile_body == nullptr) {
            return false;
        }

        ByteBlock temp {};
        if (unknown_profile_body->getHexaTextChild(temp, u"profile_data")) {
            profile_data = temp;
        }
    }
    return ok;
}
