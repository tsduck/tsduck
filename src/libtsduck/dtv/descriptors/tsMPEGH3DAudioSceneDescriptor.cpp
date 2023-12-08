//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioSceneDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_scene_descriptor"
#define MY_CLASS ts::MPEGH3DAudioSceneDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MPH3D_SCENE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioSceneDescriptor::MPEGH3DAudioSceneDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEGH3DAudioSceneDescriptor::clearContent()
{
    _3dAudioSceneID = 0;
    interactivityGroups.clear();
    switchGroups.clear();
    presetGroups.clear();
}

ts::MPEGH3DAudioSceneDescriptor::MPEGH3DAudioSceneDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioSceneDescriptor()
{
    deserialize(duck, desc);
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::MH3D_InteractivityInfo_type()
{
    mae_groupID = 0;
    mae_allowOnOff = false;
    mae_defaultOnOff = false;
    mae_contentKind = 0;
    positionInteractivity.reset();
    gainInteractivity.reset();
    mae_contentLanguage.reset();
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::PositionInteractivityType()
{
    mae_interactivityMinAzOffset = 0;
    mae_interactivityMaxAzOffset = 0;
    mae_interactivityMinElOffset = 0;
    mae_interactivityMaxElOffset = 0;
    mae_interactivityMinDistOffset = 0;
    mae_interactivityMaxDistOffset = 0;
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::GainInteractivityType()
{
    mae_interactivityMinGain = 0;
    mae_interactivityMaxGain = 0;
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::MH3D_SwitchGroup_type()
{
    mae_switchGroupID = 0;
    mae_switchGroupAllowOnOff = false;
    mae_switchGroupDefaultOnOff = false;
    mae_switchGroupMemberID.clear();
    mae_switchGroupDefaultGroupID = 0;
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::MH3D_PresetGroup_type() {
    mae_groupPresetID = 0;
    mae_groupPresetKind = 0;
    groupPresetConditions.clear();
}

ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::GroupPresetConditions_type() {
    mae_groupPresetGroupID = 0;  //!< 7 bits.
    mae_groupPresetDisableGainInteractivity.reset();
    mae_groupPresetDisablePositionInteractivity.reset();
    mae_groupPresetGain.reset();
    mae_groupPresetAzOffset.reset();
    mae_groupPresetElOffset.reset();
    mae_groupPresetDistFactor.reset();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioSceneDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioSceneDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(!interactivityGroups.empty());
    buf.putBit(!switchGroups.empty());
    buf.putBit(!presetGroups.empty());
    buf.putReserved(5);
    buf.putUInt8(_3dAudioSceneID);

    if (!interactivityGroups.empty()) {
        buf.putBit(1);
        buf.putBits(interactivityGroups.size(), 7);
        for (auto ig : interactivityGroups) {
            ig.serialize(buf);
        }
    }
    if (!switchGroups.empty()) {
        buf.putReserved(3);
        buf.putBits(switchGroups.size(), 5);
        for (auto sg : switchGroups) {
            sg.serialize(buf);
        }
    }
    if (!presetGroups.empty()) {
        buf.putReserved(3);
        buf.putBits(presetGroups.size(), 5);
        for (auto pg : presetGroups) {
            pg.serialize(buf);
        }
    }
    buf.putBytes(reserved);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::serialize(PSIBuffer& buf) const
{
    buf.putReserved(1);
    buf.putBits(mae_groupID, 7);
    buf.putReserved(3);
    buf.putBit(mae_allowOnOff);
    buf.putBit(mae_defaultOnOff);
    buf.putBit(positionInteractivity.has_value());
    buf.putBit(gainInteractivity.has_value());
    buf.putBit(mae_contentLanguage.has_value());
    buf.putReserved(4);
    buf.putBits(mae_contentKind, 4);
    if (positionInteractivity.has_value()) {
        positionInteractivity.value().serialize(buf);
    }
    if (gainInteractivity.has_value()) {
        gainInteractivity.value().serialize(buf);
    }
    if (mae_contentLanguage.has_value()) {
         buf.putLanguageCode(mae_contentLanguage.value());
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::serialize(PSIBuffer& buf) const
{
    buf.putReserved(1);
    buf.putBits(mae_interactivityMinAzOffset, 7);
    buf.putReserved(1);
    buf.putBits(mae_interactivityMaxAzOffset, 7);
    buf.putReserved(3);
    buf.putBits(mae_interactivityMinElOffset, 5);
    buf.putReserved(3);
    buf.putBits(mae_interactivityMaxElOffset, 5);
    buf.putBits(mae_interactivityMinDistOffset, 4);
    buf.putBits(mae_interactivityMaxDistOffset, 4);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::serialize(PSIBuffer& buf) const
{
    buf.putReserved(2);
    buf.putBits(mae_interactivityMinGain, 6);
    buf.putReserved(3);
    buf.putBits(mae_interactivityMaxGain, 5);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::serialize(PSIBuffer& buf) const
{
    buf.putReserved(1);
    buf.putBits(mae_switchGroupID, 5);
    buf.putBit(mae_switchGroupAllowOnOff);
    buf.putBit(mae_switchGroupDefaultOnOff);
    buf.putReserved(3);
    buf.putBits(mae_switchGroupMemberID.size()-1, 5);
    for (auto gm : mae_switchGroupMemberID) {
        buf.putReserved(1);
        buf.putBits(gm, 7);
    }
    buf.putReserved(1);
    buf.putBits(mae_switchGroupDefaultGroupID, 7);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::serialize(PSIBuffer& buf) const
{
    buf.putReserved(3);
    buf.putBits(mae_groupPresetID, 5);
    buf.putReserved(3);
    buf.putBits(mae_groupPresetKind, 5);
    buf.putReserved(4);
    buf.putBits(groupPresetConditions.size()-1, 4);
    for (auto pg : groupPresetConditions) {
        pg.serialize(buf);
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(mae_groupPresetGroupID, 7);
    const bool mae_groupPresetConditionOnOff = mae_groupPresetDisableGainInteractivity.has_value() &&
                                               mae_groupPresetDisablePositionInteractivity.has_value();
    buf.putBit(mae_groupPresetConditionOnOff);
    if (mae_groupPresetConditionOnOff) {
        buf.putReserved(4);
        buf.putBit(mae_groupPresetDisableGainInteractivity.value());
        buf.putBit(mae_groupPresetGain.has_value());
        buf.putBit(mae_groupPresetDisablePositionInteractivity.value());
        const bool mae_groupPresetGPosition = mae_groupPresetAzOffset.has_value() &&
                                              mae_groupPresetElOffset.has_value() &&
                                              mae_groupPresetDistFactor.has_value();
        buf.putBit(mae_groupPresetGPosition);
        if (mae_groupPresetGain.has_value()) {
            buf.putUInt8(mae_groupPresetGain.value());
        }
        if (mae_groupPresetGPosition) {
            buf.putUInt8(mae_groupPresetAzOffset.value());
            buf.putReserved(2);
            buf.putBits(mae_groupPresetElOffset.value(), 6);
            buf.putReserved(4);
            buf.putBits(mae_groupPresetDistFactor.value(), 4);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioSceneDescriptor::deserializePayload(PSIBuffer& buf)
{
    const bool groupDefinitionPresent = buf.getBool();
    const bool switchGroupDefinitionPresent = buf.getBool();
    const bool presetGroupDefinitionPresent = buf.getBool();
    buf.skipBits(5);
    _3dAudioSceneID = buf.getUInt8();
    if (groupDefinitionPresent) {
        buf.skipBits(1);
        const uint8_t numGroups = buf.getBits<uint8_t>(7);
        for (auto i = 0; i < numGroups; i++) {
            MH3D_InteractivityInfo_type newInteractivityGroup(buf);
            interactivityGroups.push_back(newInteractivityGroup);
        }
    }
    if (switchGroupDefinitionPresent) {
        buf.skipBits(3);
        const uint8_t numSwitchGroups = buf.getBits<uint8_t>(5);
        for (auto i = 0; i < numSwitchGroups; i++) {
            MH3D_SwitchGroup_type newSwitchGroup(buf);
            switchGroups.push_back(newSwitchGroup);
        }
    }
    if (presetGroupDefinitionPresent) {
        buf.skipBits(3);
        const uint8_t numPresetGroups = buf.getBits<uint8_t>(5);
        for (auto i = 0; i < numPresetGroups; i++) {
            MH3D_PresetGroup_type newPresetGroup(buf);
            presetGroups.push_back(newPresetGroup);
        }
    }
    buf.getBytes(reserved);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::deserialize(PSIBuffer& buf)
{
    buf.skipBits(1);
    buf.getBits(mae_groupID, 7);
    buf.skipBits(3);
    mae_allowOnOff = buf.getBool();
    mae_defaultOnOff = buf.getBool();
    const bool mae_allowPositionInteractivity = buf.getBool();
    const bool mae_allowGainInteractivity = buf.getBool();
    const bool mae_hasContentLanguage = buf.getBool();
    buf.skipBits(4);
    buf.getBits(mae_contentKind, 4);
    if (mae_allowPositionInteractivity) {
        PositionInteractivityType newPositionInteractivity(buf);
        positionInteractivity = newPositionInteractivity;
    }
    if (mae_allowGainInteractivity) {
        GainInteractivityType newGainInteractivity(buf);
        gainInteractivity = newGainInteractivity;
    }
    if (mae_hasContentLanguage) {
        const UString lang(buf.getLanguageCode());
        mae_contentLanguage = lang;
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::deserialize(PSIBuffer& buf)
{
    buf.skipBits(1);
    buf.getBits(mae_interactivityMinAzOffset, 7);
    buf.skipBits(1);
    buf.getBits(mae_interactivityMaxAzOffset, 7);
    buf.skipBits(3);
    buf.getBits(mae_interactivityMinElOffset, 5);
    buf.skipBits(3);
    buf.getBits(mae_interactivityMaxElOffset, 5);
    buf.getBits(mae_interactivityMinDistOffset, 4);
    buf.getBits(mae_interactivityMaxDistOffset, 4);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::deserialize(PSIBuffer& buf)
{
    buf.skipBits(2);
    buf.getBits(mae_interactivityMinGain, 6);
    buf.skipBits(3);
    buf.getBits(mae_interactivityMaxGain, 5);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::deserialize(PSIBuffer& buf)
{
    buf.skipBits(1);
    buf.getBits(mae_switchGroupID, 5);
    mae_switchGroupAllowOnOff = buf.getBool();
    mae_switchGroupDefaultOnOff = buf.getBool();
    buf.skipBits(3);
    const uint8_t mae_bsSwitchGroupNumMembers = buf.getBits<uint8_t>(5);
    for (auto i = 0; i < mae_bsSwitchGroupNumMembers + 1; i++) {
        buf.skipBits(1);
        const uint8_t memberID = buf.getBits<uint8_t>(7);
        mae_switchGroupMemberID.push_back(memberID);
    }
    buf.skipBits(1);
    buf.getBits(mae_switchGroupDefaultGroupID, 7);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::deserialize(PSIBuffer& buf)
{
    buf.skipBits(3);
    buf.getBits(mae_groupPresetID, 5);
    buf.skipBits(3);
    buf.getBits(mae_groupPresetKind, 5);
    buf.skipBits(4);
    const uint8_t mae_numGroupPresetConditions = buf.getBits<uint8_t>(4);
    for (auto j = 0; j < mae_numGroupPresetConditions + 1; j++) {
        GroupPresetConditions_type newConditions(buf);
        groupPresetConditions.push_back(newConditions);
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::deserialize(PSIBuffer& buf)
{
    buf.getBits(mae_groupPresetGroupID, 7);
    const bool mae_groupPresetConditionOnOff = buf.getBool();
    if (mae_groupPresetConditionOnOff) {
        buf.skipBits(4);
        mae_groupPresetDisableGainInteractivity = buf.getBool();
        const bool mae_groupPresetGainFlag = buf.getBool();
        mae_groupPresetDisablePositionInteractivity = buf.getBool();
        const bool mae_groupPresetPositionFlag = buf.getBool();
        if (mae_groupPresetGainFlag) {
            mae_groupPresetGain = buf.getUInt8();
        }
        if (mae_groupPresetPositionFlag) {
            mae_groupPresetAzOffset = buf.getUInt8();
            buf.skipBits(2);
            buf.getBits(mae_groupPresetElOffset, 6);
            buf.skipBits(4);
            buf.getBits(mae_groupPresetDistFactor, 4);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioSceneDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const bool groupDefinitionPresent = buf.getBool();
        const bool switchGroupDefinitionPresent = buf.getBool();
        const bool presetGroupDefinitionPresent = buf.getBool();
        buf.skipReservedBits(5);
        disp << margin << "3D-audio scene info ID: " << int(buf.getUInt8()) << std::endl;
        if (groupDefinitionPresent) {
            buf.skipReservedBits(1);
            const uint8_t numGroups = buf.getBits<uint8_t>(7);
            for (uint8_t i = 0; i < numGroups; i++) {
                MH3D_InteractivityInfo_type tempIG;
                tempIG.display(disp, buf, margin, i);
            }
        }
        if (switchGroupDefinitionPresent) {
            buf.skipReservedBits(3);
            const uint8_t numSwitchGroups = buf.getBits<uint8_t>(5);
            for (uint8_t i = 0; i < numSwitchGroups; i++) {
                MH3D_SwitchGroup_type tempSG;
                tempSG.display(disp, buf, margin, i);
            }
        }
        if (presetGroupDefinitionPresent) {
            buf.skipReservedBits(3);
            const uint8_t numPresetGroups = buf.getBits<uint8_t>(5);
            for (uint8_t i = 0; i < numPresetGroups; i++) {
                MH3D_PresetGroup_type tempPG;
                tempPG.display(disp, buf, margin, i);
            }
        }
        disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t groupNum)
{
    buf.skipReservedBits(1);
    disp << margin << "Interactivity Group (" << int(groupNum) << ") id: " << int(buf.getBits<uint8_t>(7));
    buf.skipReservedBits(3);
    disp << ", allow OnOff: " << UString::TrueFalse(buf.getBool());
    disp << ", default OnOff: " << UString::TrueFalse(buf.getBool());
    const bool mae_allowPositionInteractivity = buf.getBool();
    const bool mae_allowGainInteractivity = buf.getBool();
    const bool mae_hasContentLanguage = buf.getBool();
    buf.skipReservedBits(4);
    disp << ", content kind: " << DataName(MY_XML_NAME, u"mae_contentKind", buf.getBits<uint8_t>(4), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
    if (mae_allowPositionInteractivity) {
        PositionInteractivityType pi;
        pi.display(disp, buf, margin);
    }
    if (mae_allowGainInteractivity) {
        GainInteractivityType gi;
        gi.display(disp, buf, margin);
    }
    if (mae_hasContentLanguage) {
        disp << margin << "  Content Language: " << buf.getLanguageCode() << std::endl;
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    buf.skipReservedBits(1);
    disp << margin << UString::Format(u"  Azimuth Offset (min: %f", {-1.5 * buf.getBits<uint8_t>(7)});
    buf.skipReservedBits(1);
    disp << UString::Format(u", max: %f)", {+1.5 * buf.getBits<uint8_t>(7)}) << std::endl;
    buf.skipReservedBits(3);
    disp << margin << UString::Format(u"  Elevation Offset (min: %f", {-3.0 * buf.getBits<uint8_t>(5)});
    buf.skipReservedBits(3);
    disp << UString::Format(u", max: %f)", {+3.0 * buf.getBits<uint8_t>(5)}) << std::endl;
    disp << margin << UString::Format(u"  Distance Offset (min: %f", {pow(2, buf.getBits<uint8_t>(4) - 12)});
    disp << UString::Format(u", max: %f)", {pow(2, buf.getBits<uint8_t>(4) - 12)}) << std::endl;
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    buf.skipReservedBits(2);
    disp << margin << "  Interactivity gain (min: " << int(buf.getBits<uint8_t>(6) - 63);
    buf.skipReservedBits(3);
    disp << ", max: " << int(buf.getBits<uint8_t>(5)) << ")" << std::endl;
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t groupNum)
{
    buf.skipReservedBits(1);
    disp << margin << "Switch Group (" << int(groupNum) << ") id: " << int(buf.getBits<uint8_t>(5));
    disp << ", allow OnOff: " << UString::TrueFalse(buf.getBool());
    disp << ", default OnOff: " << UString::TrueFalse(buf.getBool()) << std::endl;
    buf.skipReservedBits(3);
    const uint8_t mae_bsSwitchGroupNumMembers = buf.getBits<uint8_t>(5);
    std::vector<uint8_t> group_members;
    for (auto i = 0; i < mae_bsSwitchGroupNumMembers + 1; i++) {
        buf.skipReservedBits(1);
        group_members.push_back(buf.getBits<uint8_t>(7));
    }
    disp.displayVector(u"  Group Member IDs: ", group_members, margin);
    buf.skipReservedBits(1);
    disp << margin << "  Default Group ID: " << int(buf.getBits<uint8_t>(7)) << std::endl;
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t groupNum)
{
    buf.skipReservedBits(3);
    disp << margin << "Preset Group (" << int(groupNum) << ") id: " << int(buf.getBits<uint8_t>(5));
    buf.skipReservedBits(3);
    disp << ", kind: " << DataName(MY_XML_NAME, u"mae_groupPresetKind", buf.getBits<uint8_t>(5), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
    buf.skipReservedBits(4);
    const uint8_t mae_numGroupPresetConditions = buf.getBits<uint8_t>(4);
    for (uint8_t j = 0; j < mae_numGroupPresetConditions + 1; j++) {
        GroupPresetConditions_type preset;
        preset.display(disp, buf, margin, j);
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t groupNum)
{
    disp << margin << "  Preset Condition (" << int(groupNum) << ") id: " << int(buf.getBits<uint8_t>(7));
    const bool mae_groupPresetConditionOnOff = buf.getBool();
    disp << " " << (mae_groupPresetConditionOnOff ? "[on]" : "[off]") << std::endl;
    if (mae_groupPresetConditionOnOff) {
        buf.skipReservedBits(4);
        disp << margin << "   Disable Gain Interactivity: " << UString::TrueFalse(buf.getBool());
        const bool mae_groupPresetGainFlag = buf.getBool();
        disp << ", Disable Position Interactivity: " << UString::TrueFalse(buf.getBool()) << std::endl;
        const bool mae_groupPresetPositionFlag = buf.getBool();
        if (mae_groupPresetGainFlag) {
            disp << margin << UString::Format(u"   Preset Gain: %f dB", {((0.5 * (buf.getUInt8() - 255)) + 32)}) << std::endl;
        }
        if (mae_groupPresetPositionFlag) {
            disp << margin << UString::Format(u"   Azimuth Offset: %f degrees", {1.5 * (buf.getUInt8() - 127)});
            buf.skipReservedBits(2);
            disp << UString::Format(u", Elevation Offset: %f degrees", {3 * (buf.getBits<uint8_t>(6) - 32)}) << std::endl;
            buf.skipReservedBits(4);
            disp << margin << UString::Format(u"   Distance Factor: %f", {pow(2, buf.getBits<uint8_t>(4) - 12)}) << std::endl;
        }
    }
}

//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioSceneDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sceneID", _3dAudioSceneID);
    for (auto ig : interactivityGroups) {
        ig.toXML(root->addElement(u"InteractivityGroup"));
    }
    for (auto sg : switchGroups) {
        sg.toXML(root->addElement(u"SwitchGroup"));
    }
    for (auto pg : presetGroups) {
        pg.toXML(root->addElement(u"PresetGroup"));
    }
    root->addHexaTextChild(u"reserved", reserved, true);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"groupID", mae_groupID);
    root->setBoolAttribute(u"allowOnOff", mae_allowOnOff);
    root->setBoolAttribute(u"defaultOnOff", mae_defaultOnOff);
    root->setIntAttribute(u"contentKind", mae_contentKind);
    if (mae_contentLanguage.has_value())
        root->setAttribute(u"contentLanguage", mae_contentLanguage.value());

    if (positionInteractivity.has_value()) {
        positionInteractivity.value().toXML(root->addElement(u"PositionInteractivity"));
    }
    if (gainInteractivity.has_value()) {
        gainInteractivity.value().toXML(root->addElement(u"GainInteractivity"));
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"azimuthOffsetMin", mae_interactivityMinAzOffset);
    root->setIntAttribute(u"azimuthOffsetMax", mae_interactivityMaxAzOffset);
    root->setIntAttribute(u"elevationOffsetMin", mae_interactivityMinElOffset);
    root->setIntAttribute(u"elevationOffsetMax", mae_interactivityMaxElOffset);
    root->setIntAttribute(u"distanceOffsetMin", mae_interactivityMinDistOffset);
    root->setIntAttribute(u"distanceOffsetMax", mae_interactivityMaxDistOffset);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"interactivityGainMin", mae_interactivityMinGain);
    root->setIntAttribute(u"interactivityGainMax", mae_interactivityMaxGain);
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"switchGroupID", mae_switchGroupID);
    root->setBoolAttribute(u"switchGroupAllowOnOff", mae_switchGroupAllowOnOff);
    root->setBoolAttribute(u"switchGroupDefaultOnOff", mae_switchGroupDefaultOnOff);
    ByteBlock group_members;
    for (auto member : mae_switchGroupMemberID) {
        group_members.push_back(member);
    }
    root->addHexaTextChild(u"SwitchGroupMembers", group_members);
    root->setIntAttribute(u"switchGroupDefaultGroupID", mae_switchGroupDefaultGroupID);
}


void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"groupPresetID", mae_groupPresetID);
    root->setIntAttribute(u"groupPresetKind", mae_groupPresetKind);
    for (auto pc : groupPresetConditions) {
        pc.toXML(root->addElement(u"PresetConditions"));
    }
}

void ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"groupPresetGroupID", mae_groupPresetGroupID);
    root->setOptionalBoolAttribute(u"groupPresetDisableGainInteractivity", mae_groupPresetDisableGainInteractivity);
    root->setOptionalBoolAttribute(u"groupPresetDisablePositionInteractivity", mae_groupPresetDisablePositionInteractivity);
    root->setOptionalIntAttribute(u"groupPresetGain", mae_groupPresetGain);
    root->setOptionalIntAttribute(u"groupPresetAzOffset", mae_groupPresetAzOffset);
    root->setOptionalIntAttribute(u"groupPresetElOffset", mae_groupPresetElOffset);
    root->setOptionalIntAttribute(u"groupPresetDistFactor", mae_groupPresetDistFactor);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioSceneDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    ts::xml::ElementVector interactivity_groups, switch_groups, preset_groups;
    bool ok = element->getIntAttribute(_3dAudioSceneID, u"sceneID", true) &&
              element->getChildren(interactivity_groups, u"InteractivityGroup", 0, 127) &&
              element->getChildren(switch_groups, u"SwitchGroup", 0, 31) &&
              element->getChildren(preset_groups, u"PresetGroup", 0, 31) &&
              element->getHexaTextChild(reserved, u"reserved", false);
    bool ig_ok = true;
    for (size_t i = 0; ok && i < interactivity_groups.size(); ++i) {
        MH3D_InteractivityInfo_type newIG;
        if (newIG.fromXML(interactivity_groups[i])) {
            interactivityGroups.push_back(newIG);
        }
        else {
            ig_ok = false;
        }
    }
    bool sg_ok = true;
    for (size_t i = 0; ok && i < switch_groups.size(); ++i) {
        MH3D_SwitchGroup_type newSG;
        if (newSG.fromXML(switch_groups[i])) {
            switchGroups.push_back(newSG);
        }
        else {
            sg_ok = false;
        }
    }
    bool pg_ok = true;
    for (size_t i = 0; ok && i < preset_groups.size(); ++i) {
        MH3D_PresetGroup_type newPG;
        if (newPG.fromXML(preset_groups[i])) {
            presetGroups.push_back(newPG);
        }
        else {
            pg_ok = false;
        }
    }
    return ok && ig_ok && sg_ok && pg_ok;
}


bool ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::fromXML(const xml::Element* element)
{
    ts::xml::ElementVector position_interactivity, gain_interactivity;
    bool ok = element->getIntAttribute(mae_groupID, u"groupID", true, 0, 0, 0x1f) &&
              element->getBoolAttribute(mae_allowOnOff, u"allowOnOff", true) &&
              element->getBoolAttribute(mae_defaultOnOff, u"defaultOnOff", true) &&
              element->getIntAttribute(mae_contentKind, u"contentKind", true, 0, 0, 0x0f) &&
              element->getChildren(position_interactivity, u"PositionInteractivity", 0, 1) &&
              element->getChildren(gain_interactivity, u"GainInteractivity", 0, 1) &&
              element->getOptionalAttribute(mae_contentLanguage, u"contentLanguage", 0, 3);
    if (ok) {
        if (!position_interactivity.empty()) {
            PositionInteractivityType pos;
            ok = pos.fromXML(position_interactivity[0]);
            if (ok) {
                positionInteractivity = pos;
            }
        }
        if (!gain_interactivity.empty()) {
            GainInteractivityType gain;
            ok = gain.fromXML(gain_interactivity[0]) && ok;
            if (ok) {
                gainInteractivity = gain;
            }
        }
    }
    return ok;
}

bool ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::PositionInteractivityType::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(mae_interactivityMinAzOffset, u"azimuthOffsetMin", true, 0, 0, 0x7f) &&
           element->getIntAttribute(mae_interactivityMaxAzOffset, u"azimuthOffsetMax", true, 0, 0, 0x7f) &&
           element->getIntAttribute(mae_interactivityMinElOffset, u"elevationOffsetMin", true, 0, 0, 0x1f) &&
           element->getIntAttribute(mae_interactivityMaxElOffset, u"elevationOffsetMax", true, 0, 0, 0x1f) &&
           element->getIntAttribute(mae_interactivityMinDistOffset, u"distanceOffsetMin", true, 0, 0, 0x0f) &&
           element->getIntAttribute(mae_interactivityMaxDistOffset, u"distanceOffsetMax", true, 0, 0, 0x0f);
}

bool ts::MPEGH3DAudioSceneDescriptor::MH3D_InteractivityInfo_type::GainInteractivityType::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(mae_interactivityMinGain, u"interactivityGainMin", true, 0, 0, 0x3f) &&
           element->getIntAttribute(mae_interactivityMaxGain, u"interactivityGainMax", true, 0, 0, 0x1f);
}

bool ts::MPEGH3DAudioSceneDescriptor::MH3D_SwitchGroup_type::fromXML(const xml::Element* element)
{
    ByteBlock group_members;
    bool ok = element->getIntAttribute(mae_switchGroupID, u"switchGroupID", true, 0, 0, 0x1f) &&
              element->getBoolAttribute(mae_switchGroupAllowOnOff, u"switchGroupAllowOnOff", true) &&
              element->getBoolAttribute(mae_switchGroupDefaultOnOff, u"switchGroupDefaultOnOff", true) &&
              element->getHexaTextChild(group_members, u"SwitchGroupMembers", true, 1, 32) &&
              element->getIntAttribute(mae_switchGroupDefaultGroupID, u"switchGroupDefaultGroupID", true, 0, 0, 0x7f);
    if (ok && !group_members.empty()) {
        for (auto member : group_members) {
            if (/* member >= 0 && */ member <= 127) {
                mae_switchGroupMemberID.push_back(member);
            }
            else {
                element->report().error(u"SwitchGroupMember identifiers can only be 7 bits (0-127) in <%s>, line %d", {element->name(), element->lineNumber()});
                ok = false;
            }
        }
    }
    return ok;
}

bool ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::fromXML(const xml::Element* element)
{
    ts::xml::ElementVector preset_conditions;
    bool ok = element->getIntAttribute(mae_groupPresetID, u"groupPresetID", true, 0, 0, 0x1f) &&
              element->getIntAttribute(mae_groupPresetKind, u"groupPresetKind", true, 0, 0, 0x1f) &&
              element->getChildren(preset_conditions, u"PresetConditions", 1, 16);

    bool conditions_ok = true;
    if (ok) {
        for (size_t i = 0; i < preset_conditions.size(); ++i) {
            GroupPresetConditions_type newConditions;
            if (newConditions.fromXML(preset_conditions[i])) {
                groupPresetConditions.push_back(newConditions);
            }
            else {
                conditions_ok = false;
            }
        }
    }
    return ok && conditions_ok;
}

bool ts::MPEGH3DAudioSceneDescriptor::MH3D_PresetGroup_type::GroupPresetConditions_type::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(mae_groupPresetGroupID, u"groupPresetGroupID", true, 0, 0, 0x7f);
    bool hasPresetDisableGainInteractivity = element->hasAttribute(u"groupPresetDisableGainInteractivity"),
         hasPresetDisablePositionInteractivity = element->hasAttribute(u"groupPresetDisablePositionInteractivity"),
         hasPresetGain = element->hasAttribute(u"groupPresetGain"),
         hasPresetAzOffset = element->hasAttribute(u"groupPresetAzOffset"),
         hasPresetElOffset = element->hasAttribute(u"groupPresetElOffset"),
         hasPresetDistFactor = element->hasAttribute(u"groupPresetDistFactor");

    if (!(hasPresetDisableGainInteractivity || hasPresetDisablePositionInteractivity || hasPresetGain ||
          hasPresetAzOffset || hasPresetElOffset || hasPresetDistFactor)) {
        // i.e. groupPresetConditionOnOff == false
        return ok;
    }

    if ((hasPresetAzOffset + hasPresetElOffset + hasPresetDistFactor != 0) && (hasPresetAzOffset + hasPresetElOffset + hasPresetDistFactor != 3)) {
        element->report().error(u"all or none of groupPresetAzOffset, groupPresetElOffset and groupPresetDistFactor must be specified in <%s>, line %d", {element->name(), element->lineNumber()});
        ok = false;
    }
    else if (!hasPresetDisableGainInteractivity && !hasPresetDisablePositionInteractivity) {
        element->report().error(u"groupPresetAzOffset, groupPresetElOffset and groupPresetDistFactor can only be specified with groupPresetDisableGainInteractivity and groupPresetDisablePositionInteractivity in <%s>, line %d", {element->name(), element->lineNumber()});
        ok = false;
    }

    if (hasPresetAzOffset || hasPresetElOffset || hasPresetDistFactor) {
        uint8_t az = 0, el = 0, dist = 0;
        ok = element->getIntAttribute(az, u"groupPresetAzOffset", true, 0, 0, 0xff) &&
             element->getIntAttribute(el, u"groupPresetElOffset", true, 0, 0, 0x3f) &&
             element->getIntAttribute(dist, u"groupPresetDistFactor", true, 0, 0, 0x0f);
        if (ok) {
            mae_groupPresetAzOffset = az;
            mae_groupPresetElOffset = el;
            mae_groupPresetDistFactor = dist;
        }
    }

    if (hasPresetDisableGainInteractivity + hasPresetDisablePositionInteractivity == 1) {
        element->report().error(u"both groupPresetDisableGainInteractivity and mae_groupPresetDisablePositionInteractivity must be specified in <%s>, line %d", {element->name(), element->lineNumber()});
        ok = false;
    }
    else {
        bool gain = false, posi = false;
        ok = element->getBoolAttribute(gain, u"groupPresetDisableGainInteractivity", true) &&
             element->getBoolAttribute(posi, u"groupPresetDisablePositionInteractivity", true);
        if (ok) {
            mae_groupPresetDisableGainInteractivity = gain;
            mae_groupPresetDisablePositionInteractivity = posi;
        }
    }
    if (hasPresetGain && !(hasPresetDisableGainInteractivity || hasPresetDisablePositionInteractivity)) {
        element->report().error(u"groupPresetGain must be specified with groupPresetDisableGainInteractivity and mae_groupPresetDisablePositionInteractivity  <%s>, line %d", {element->name(), element->lineNumber()});
        ok = false;
    }
    else if (hasPresetGain) {
        uint8_t preset_gain = 0;
        ok = element->getIntAttribute(preset_gain, u"groupPresetGain", true);
        if (ok) {
            mae_groupPresetGain = preset_gain;
        }
    }
    return ok;
}


