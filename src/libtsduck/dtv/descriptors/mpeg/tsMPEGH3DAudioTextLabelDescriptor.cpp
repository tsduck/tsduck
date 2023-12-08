//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioTextLabelDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_text_label_descriptor"
#define MY_CLASS ts::MPEGH3DAudioTextLabelDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MPH3D_TEXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioTextLabelDescriptor::MPEGH3DAudioTextLabelDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEGH3DAudioTextLabelDescriptor::clearContent()
{
    _3dAudioSceneInfoID = 0;
    description_languages.clear();
    numReservedBytes.reset();
}

ts::MPEGH3DAudioTextLabelDescriptor::MPEGH3DAudioTextLabelDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioTextLabelDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioTextLabelDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioTextLabelDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(_3dAudioSceneInfoID);
    buf.putBits(0xFF, 4);
    buf.putBits(description_languages.size(), 4);
    for (auto desclanguage : description_languages) {
        buf.putLanguageCode(desclanguage.descriptionLanguage);
        buf.putBits(0xFF, 1);
        buf.putBits(desclanguage.group_descriptions.size(), 7);
        for (auto groupDesc : desclanguage.group_descriptions) {
            buf.putBits(0xFF, 1);
            buf.putBits(groupDesc.mae_descriptionGroupID, 7);
            buf.putStringWithByteLength(groupDesc.groupDescriptionData);
        }
        buf.putBits(0xFF, 3);
        buf.putBits(desclanguage.switch_group_descriptions.size(), 5);
        for (auto switchGroup : desclanguage.switch_group_descriptions) {
            buf.putBits(0xFF, 3);
            buf.putBits(switchGroup.mae_descriptionSwitchGroupID, 5);
            buf.putStringWithByteLength(switchGroup.switchGroupDescriptionData);
        }
        buf.putBits(0xFF, 3);
        buf.putBits(desclanguage.group_preset_descriptions.size(), 5);
        for (auto groupPreset : desclanguage.group_preset_descriptions) {
            buf.putBits(0xFF, 3);
            buf.putBits(groupPreset.mae_descriptionGroupPresetID, 5);
            buf.putStringWithByteLength(groupPreset.groupDescriptionPresetData);
        }
    }
    if (numReservedBytes.has_value()) {
        for (size_t i = 0; i < numReservedBytes.value(); i++) {
            buf.putUInt8(0xFF);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioTextLabelDescriptor::deserializePayload(PSIBuffer& buf)
{
    _3dAudioSceneInfoID = buf.getUInt8();
    buf.skipBits(4);
    uint8_t numDescLanguages = buf.getBits<uint8_t>(4);
    for (uint8_t i = 0; i < numDescLanguages; i++) {
        descriptionLanguage_type newDescriptionLanguage;
        newDescriptionLanguage.descriptionLanguage = buf.getLanguageCode();
        buf.skipBits(1);
        uint8_t numGroupDescriptions = buf.getBits<uint8_t>(7);
        for (uint8_t n = 0; n < numGroupDescriptions; n++) {
            groupDescription_type newGroupDescription;
            buf.skipBits(1);
            newGroupDescription.mae_descriptionGroupID = buf.getBits<uint8_t>(7);
            newGroupDescription.groupDescriptionData = buf.getStringWithByteLength();
            newDescriptionLanguage.group_descriptions.push_back(newGroupDescription);
        }
        buf.skipBits(3);
        uint8_t numSwitchGroupDescriptions = buf.getBits<uint8_t>(5);
        for (uint8_t n = 0; n < numSwitchGroupDescriptions; n++) {
            switchGroupDescription_type newSwitchGroupDescription;
            buf.skipBits(3);
            newSwitchGroupDescription.mae_descriptionSwitchGroupID = buf.getBits<uint8_t>(5);
            newSwitchGroupDescription.switchGroupDescriptionData = buf.getStringWithByteLength();
            newDescriptionLanguage.switch_group_descriptions.push_back(newSwitchGroupDescription);
        }
        buf.skipBits(3);
        uint8_t numGroupPresetsDescriptions = buf.getBits<uint8_t>(5);
        for (uint8_t n = 0; n < numGroupPresetsDescriptions; n++) {
            groupPresetsDescription_type newGroupPresetsDescription;
            buf.skipBits(3);
            newGroupPresetsDescription.mae_descriptionGroupPresetID = buf.getBits<uint8_t>(5);
            newGroupPresetsDescription.groupDescriptionPresetData = buf.getStringWithByteLength();
            newDescriptionLanguage.group_preset_descriptions.push_back(newGroupPresetsDescription);
        }
        description_languages.push_back(newDescriptionLanguage);
    }
    ByteBlock reserved = buf.getBytes();
    numReservedBytes = reserved.size();
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioTextLabelDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "3D Audio Scene Id: " << int(buf.getUInt8()) << std::endl;
        buf.skipReservedBits(4);
        uint8_t numDescLanguages = buf.getBits<uint8_t>(4);
        for (uint8_t i = 0; i < numDescLanguages; i++) {
            disp << margin << "Description Language: " << buf.getLanguageCode() << std::endl;
            buf.skipReservedBits(1);
            uint8_t numGroupDescriptions = buf.getBits<uint8_t>(7);
            for (uint8_t n = 0; n < numGroupDescriptions; n++) {
                buf.skipReservedBits(1);
                disp << margin << UString::Format(u" Group Description [%d] id: %d - ", { n, buf.getBits<uint8_t>(7) });
                const UString groupDescriptionData(buf.getStringWithByteLength());
                disp << "\"" << groupDescriptionData << "\"" << std::endl;
            }
            buf.skipReservedBits(3);
            uint8_t numSwitchGroupDescriptions = buf.getBits<uint8_t>(5);
            for (uint8_t n = 0; n < numSwitchGroupDescriptions; n++) {
                buf.skipReservedBits(3);
                disp << margin << UString::Format(u" Switch Group Description [%d] id: %d - ", { n, buf.getBits<uint8_t>(5) });
                const UString switchGroupDescriptionData(buf.getStringWithByteLength());
                disp << "\"" << switchGroupDescriptionData << "\"" << std::endl;
            }
            buf.skipReservedBits(3);
            uint8_t numGroupPresetsDescriptions = buf.getBits<uint8_t>(5);
            for (uint8_t n = 0; n < numGroupPresetsDescriptions; n++) {
                buf.skipReservedBits(3);
                disp << margin << UString::Format(u" Group Preset Description [%d] id: %d - ", { n, buf.getBits<uint8_t>(5) });
                const UString groupPresetDescriptionData(buf.getStringWithByteLength());
                disp << "\"" << groupPresetDescriptionData << "\"" << std::endl;
            }
        }
        ByteBlock reserved = buf.getBytes();
        if (!reserved.empty()) {
            disp << margin << "reserved: " << UString::Dump(reserved, UString::SINGLE_LINE) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioTextLabelDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"_3dAudioSceneInfoID", _3dAudioSceneInfoID);
    for (auto desclanguage : description_languages) {
        ts::xml::Element* description = root->addElement(u"DescriptionLanguage");
        description->setAttribute(u"descriptionLanguage", desclanguage.descriptionLanguage);
        for (auto groupDesc : desclanguage.group_descriptions) {
            ts::xml::Element* groupDescription = description->addElement(u"GroupDescription");
            groupDescription->setIntAttribute(u"mae_descriptionGroupID", groupDesc.mae_descriptionGroupID);
            groupDescription->setAttribute(u"groupDescription", groupDesc.groupDescriptionData);
        }
        for (auto switchGroup : desclanguage.switch_group_descriptions) {
            ts::xml::Element* switchGroupDescription = description->addElement(u"SwitchGroupDescription");
            switchGroupDescription->setIntAttribute(u"mae_descriptionSwitchGroupID", switchGroup.mae_descriptionSwitchGroupID);
            switchGroupDescription->setAttribute(u"switchGroupDescription", switchGroup.switchGroupDescriptionData);
        }
        for (auto groupPreset : desclanguage.group_preset_descriptions) {
            ts::xml::Element* groupPresetDescription = description->addElement(u"GroupPresetDescription");
            groupPresetDescription->setIntAttribute(u"mae_descriptionGroupPresetID", groupPreset.mae_descriptionGroupPresetID);
            groupPresetDescription->setAttribute(u"groupPresetDescription", groupPreset.groupDescriptionPresetData);
        }
    }
    root->setOptionalIntAttribute(u"numReservedBytes", numReservedBytes);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioTextLabelDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector desclanguages;
    bool ok =
        element->getIntAttribute(_3dAudioSceneInfoID, u"_3dAudioSceneInfoID", true, 0, 0x00, 0xFF) &&
        element->getOptionalIntAttribute(numReservedBytes, u"numReservedBytes", 0, 0xFFFF) &&
        element->getChildren(desclanguages, u"DescriptionLanguage", 0, 0x0F);

    for (size_t i = 0; ok && i < desclanguages.size(); ++i) {
        descriptionLanguage_type newDescriptionLanguage;
        xml::ElementVector groupDescriptions, switchGroupDescriptions, groupPresetDescriptions;
        ok &= desclanguages[i]->getAttribute(newDescriptionLanguage.descriptionLanguage, u"descriptionLanguage", true, u"***", 3, 3) &&
            desclanguages[i]->getChildren(groupDescriptions, u"GroupDescription", 0, 0x7F) &&
            desclanguages[i]->getChildren(switchGroupDescriptions, u"SwitchGroupDescription", 0, 0x1F) &&
            desclanguages[i]->getChildren(groupPresetDescriptions, u"GroupPresetDescription", 0, 0x1F);

        for (size_t n = 0; ok && n < groupDescriptions.size(); ++n) {
            groupDescription_type newGroupDescription;
            ok &= groupDescriptions[n]->getIntAttribute(newGroupDescription.mae_descriptionGroupID, u"mae_descriptionGroupID", true, 0, 0x00, 0x7F) &&
                groupDescriptions[n]->getAttribute(newGroupDescription.groupDescriptionData, u"groupDescription");
            newDescriptionLanguage.group_descriptions.push_back(newGroupDescription);
        }
        for (size_t n = 0; ok && n < switchGroupDescriptions.size(); ++n) {
            switchGroupDescription_type newSwitchGroupDescription;
            ok &= switchGroupDescriptions[n]->getIntAttribute(newSwitchGroupDescription.mae_descriptionSwitchGroupID, u"mae_descriptionSwitchGroupID", true, 0, 0x00, 0x1F) &&
                switchGroupDescriptions[n]->getAttribute(newSwitchGroupDescription.switchGroupDescriptionData, u"switchGroupDescription");
            newDescriptionLanguage.switch_group_descriptions.push_back(newSwitchGroupDescription);
        }
        for (size_t n = 0; ok && n < groupPresetDescriptions.size(); ++n) {
            groupPresetsDescription_type newGroupPresetsDescription;
            ok &= groupPresetDescriptions[n]->getIntAttribute(newGroupPresetsDescription.mae_descriptionGroupPresetID, u"mae_descriptionGroupPresetID", true, 0, 0x00, 0x1F) &&
                groupPresetDescriptions[n]->getAttribute(newGroupPresetsDescription.groupDescriptionPresetData, u"groupPresetDescription");
            newDescriptionLanguage.group_preset_descriptions.push_back(newGroupPresetsDescription);
        }
        description_languages.push_back(newDescriptionLanguage);
    }
    return ok;
}
