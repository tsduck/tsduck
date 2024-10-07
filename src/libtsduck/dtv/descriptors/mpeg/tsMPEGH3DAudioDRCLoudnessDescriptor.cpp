//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioDRCLoudnessDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_drc_loudness_descriptor"
#define MY_CLASS ts::MPEGH3DAudioDRCLoudnessDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MPH3D_DRCLOUD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioDRCLoudnessDescriptor::MPEGH3DAudioDRCLoudnessDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::clearContent()
{
    drcInstructionsUniDrc.clear();
    loudnessInfo.clear();
    downmixId.clear();
    reserved.clear();
}

ts::MPEGH3DAudioDRCLoudnessDescriptor::MPEGH3DAudioDRCLoudnessDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioDRCLoudnessDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioDRCLoudnessDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool mpegh3daDrcAndLoudnessInfoPresent = !drcInstructionsUniDrc.empty() || !loudnessInfo.empty() || !downmixId.empty();
    buf.putReserved(7);
    buf.putBit(mpegh3daDrcAndLoudnessInfoPresent);

    if (mpegh3daDrcAndLoudnessInfoPresent) {
        buf.putReserved(2);
        buf.putBits(drcInstructionsUniDrc.size(), 6);
        buf.putReserved(2);
        buf.putBits(loudnessInfo.size(), 6);
        buf.putReserved(3);
        buf.putBits(downmixId.size(), 5);

        for (const auto& drc : drcInstructionsUniDrc) {
            drc.serialize(buf);
        }
        for (const auto& ldi : loudnessInfo) {
            ldi.serialize(buf);
        }
        for (const auto& dmi : downmixId) {
            dmi.serialize(buf);
        }
    }
    buf.putBytes(reserved);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipReservedBits(7);
    const bool mpegh3daDrcAndLoudnessInfoPresent = buf.getBool();

    if (mpegh3daDrcAndLoudnessInfoPresent) {
        buf.skipReservedBits(2);
        drcInstructionsUniDrc.resize(buf.getBits<size_t>(6));
        buf.skipReservedBits(2);
        loudnessInfo.resize(buf.getBits<size_t>(6));
        buf.skipReservedBits(3);
        downmixId.resize(buf.getBits<size_t>(5));

        for (auto& drc : drcInstructionsUniDrc) {
            drc.deserialize(buf);
        }
        for (auto& ldi : loudnessInfo) {
            ldi.deserialize(buf);
        }
        for (auto& dmi : downmixId) {
            dmi.deserialize(buf);
        }
    }
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    buf.skipReservedBits(7);
    const bool mpegh3daDrcAndLoudnessInfoPresent = buf.getBool();

    if (mpegh3daDrcAndLoudnessInfoPresent) {
        buf.skipReservedBits(2);
        const size_t drcInstructionsUniDrcCount = buf.getBits<size_t>(6);
        buf.skipReservedBits(2);
        const size_t loudnessInfoCount = buf.getBits<size_t>(6);
        buf.skipReservedBits(3);
        const size_t downmixIdCount = buf.getBits<size_t>(5);

        disp << margin << "Number of DRC instructions: " << drcInstructionsUniDrcCount << std::endl;
        for (size_t i = 0; !buf.readError() && i < drcInstructionsUniDrcCount; ++i) {
            disp << margin << "- DRC instructions #" << i << std::endl;
            DRCInstructions::Display(disp, buf, margin + u"  ");
        }

        disp << margin << "Number of loudness info: " << loudnessInfoCount << std::endl;
        for (size_t i = 0; !buf.readError() && i < loudnessInfoCount; ++i) {
            disp << margin << "- Loudness info #" << i << std::endl;
            LoudnessInfo::Display(disp, buf, margin + u"  ");
        }

        disp << margin << "Number of downmix id: " << downmixIdCount << std::endl;
        for (size_t i = 0; !buf.readError() && i < downmixIdCount; ++i) {
            disp << margin << "- Downmix id #" << i << std::endl;
            DownmixId::Display(disp, buf, margin + u"  ");
        }
    }
    disp.displayPrivateData(u"reserved", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& drc : drcInstructionsUniDrc) {
        drc.toXML(root->addElement(u"drcInstructionsUniDrc"));
    }
    for (const auto& ldi : loudnessInfo) {
        ldi.toXML(root->addElement(u"loudnessInfo"));
    }
    for (const auto& dmi : downmixId) {
        dmi.toXML(root->addElement(u"downmixId"));
    }
    root->addHexaTextChild(u"reserved", reserved, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioDRCLoudnessDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdrc, xldi, xdmi;
    bool ok = element->getChildren(xdrc, u"drcInstructionsUniDrc", 0, 0x3F) &&
              element->getChildren(xldi, u"loudnessInfo", 0, 0x3F) &&
              element->getChildren(xdmi, u"downmixId", 0, 0x1F) &&
              element->getHexaTextChild(reserved, u"reserved", false);

    drcInstructionsUniDrc.resize(xdrc.size());
    for (size_t i = 0; ok && i < drcInstructionsUniDrc.size(); ++i) {
        ok = drcInstructionsUniDrc[i].fromXML(xdrc[i]);
    }

    loudnessInfo.resize(xldi.size());
    for (size_t i = 0; ok && i < loudnessInfo.size(); ++i) {
        ok = loudnessInfo[i].fromXML(xldi[i]);
    }

    downmixId.resize(xdmi.size());
    for (size_t i = 0; ok && i < downmixId.size(); ++i) {
        ok = downmixId[i].fromXML(xdmi[i]);
    }

    return ok;
}



//----------------------------------------------------------------------------
// Dynamic Range Control instructions class
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::serialize(PSIBuffer& buf) const
{
    buf.putReserved(6);
    buf.putBits(drcInstructionsType, 2);
    if (drcInstructionsType == 2) {
        buf.putReserved(1);
        buf.putBits(mae_groupID.value_or(0), 7);
    }
    else if (drcInstructionsType == 3) {
        buf.putReserved(3);
        buf.putBits(mae_groupPresetID.value_or(0), 5);
    }
    buf.putReserved(2);
    buf.putBits(drcSetId, 6);
    buf.putReserved(1);
    buf.putBits(downmixId, 7);
    buf.putReserved(3);
    if (additionalDownmixId.size() > 7) {
        buf.setUserError();
        return;
    }
    buf.putBits(additionalDownmixId.size(), 3);
    buf.putBit(bsLimiterPeakTarget.has_value());
    buf.putBit(bsDrcSetTargetLoudnessValueUpper.has_value());
    for (auto id : additionalDownmixId) {
        buf.putReserved(1);
        buf.putBits(id, 7);
    }
    buf.putUInt16(drcSetEffect);
    buf.putBits(bsLimiterPeakTarget, 8);
    if (bsDrcSetTargetLoudnessValueUpper.has_value()) {
        buf.putReserved(1);
        buf.putBits(bsDrcSetTargetLoudnessValueUpper, 6);
        buf.putBit(bsDrcSetTargetLoudnessValueLower.has_value());
        if (bsDrcSetTargetLoudnessValueLower.has_value()) {
            buf.putReserved(2);
            buf.putBits(bsDrcSetTargetLoudnessValueLower, 6);
        }
    }
    buf.putReserved(1);
    buf.putBits(dependsOnDrcSet, 6);
    buf.putBit(dependsOnDrcSet != 0 || noIndependentUse);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::clear()
{
    mae_groupID.reset();
    mae_groupPresetID.reset();
    additionalDownmixId.clear();
    bsLimiterPeakTarget.reset();
    bsDrcSetTargetLoudnessValueUpper.reset();
    bsDrcSetTargetLoudnessValueLower.reset();
    noIndependentUse = false;
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::deserialize(PSIBuffer& buf)
{
    clear();

    buf.skipReservedBits(6);
    buf.getBits(drcInstructionsType, 2);
    if (drcInstructionsType == 2) {
        buf.skipReservedBits(1);
        buf.getBits(mae_groupID, 7);
    }
    else if (drcInstructionsType == 3) {
        buf.skipReservedBits(3);
        buf.getBits(mae_groupPresetID, 5);
    }
    buf.skipReservedBits(2);
    buf.getBits(drcSetId, 6);
    buf.skipReservedBits(1);
    buf.getBits(downmixId, 7);
    buf.skipReservedBits(3);
    const size_t additionalDownmixIdCount = buf.getBits<size_t>(3);
    const bool limiterPeakTargetPresent = buf.getBool();
    const bool drcSetTargetLoudnessPresent = buf.getBool();
    for (size_t i = 0; i < additionalDownmixIdCount; ++i) {
        buf.skipReservedBits(1);
        additionalDownmixId.push_back(buf.getBits<uint8_t>(7));
    }
    drcSetEffect = buf.getUInt16();
    if (limiterPeakTargetPresent) {
        bsLimiterPeakTarget = buf.getUInt8();
    }
    if (drcSetTargetLoudnessPresent) {
        buf.skipReservedBits(1);
        buf.getBits(bsDrcSetTargetLoudnessValueUpper, 6);
        const bool drcSetTargetLoudnessValueLowerPresent = buf.getBool();
        if (drcSetTargetLoudnessValueLowerPresent) {
            buf.skipReservedBits(2);
            buf.getBits(bsDrcSetTargetLoudnessValueLower, 6);
        }
    }
    buf.skipReservedBits(1);
    buf.getBits(dependsOnDrcSet, 6);
    if (dependsOnDrcSet == 0) {
        noIndependentUse = buf.getBool();
    }
    else {
        buf.skipReservedBits(1);
    }
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"drcInstructionsType", drcInstructionsType);
    root->setOptionalIntAttribute(u"mae_groupID", mae_groupID, true);
    root->setOptionalIntAttribute(u"mae_groupPresetID", mae_groupPresetID, true);
    root->setIntAttribute(u"drcSetId", drcSetId, true);
    root->setIntAttribute(u"downmixId", downmixId, true);
    for (auto id : additionalDownmixId) {
        root->addElement(u"additionalDownmixId")->setIntAttribute(u"value", id, true);
    }
    root->setIntAttribute(u"drcSetEffect", drcSetEffect, true);
    root->setOptionalIntAttribute(u"bsLimiterPeakTarget", bsLimiterPeakTarget, true);
    root->setOptionalIntAttribute(u"bsDrcSetTargetLoudnessValueUpper", bsDrcSetTargetLoudnessValueUpper, true);
    root->setOptionalIntAttribute(u"bsDrcSetTargetLoudnessValueLower", bsDrcSetTargetLoudnessValueLower, true);
    root->setIntAttribute(u"dependsOnDrcSet", dependsOnDrcSet, true);
    if (dependsOnDrcSet == 0) {
        root->setBoolAttribute(u"noIndependentUse", noIndependentUse);
    }
}

bool ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::fromXML(const xml::Element* element)
{
    clear();

    xml::ElementVector xid;
    bool ok =
        element->getIntAttribute(drcInstructionsType, u"drcInstructionsType", true, 0, 0, 3) &&
        element->getConditionalIntAttribute(mae_groupID, u"mae_groupID", drcInstructionsType == 2, 0, 0x7F) &&
        element->getConditionalIntAttribute(mae_groupPresetID, u"mae_groupPresetID", drcInstructionsType == 3, 0, 0x1F) &&
        element->getIntAttribute(drcSetId, u"drcSetId", true, 0, 0, 0x3F) &&
        element->getIntAttribute(downmixId, u"downmixId", true, 0, 0, 0x7F) &&
        element->getIntAttribute(drcSetEffect, u"drcSetEffect", true) &&
        element->getOptionalIntAttribute(bsLimiterPeakTarget, u"bsLimiterPeakTarget") &&
        element->getOptionalIntAttribute(bsDrcSetTargetLoudnessValueUpper, u"bsDrcSetTargetLoudnessValueUpper", 0, 0x3F) &&
        (!bsDrcSetTargetLoudnessValueUpper.has_value() ||
         element->getOptionalIntAttribute(bsDrcSetTargetLoudnessValueLower, u"bsDrcSetTargetLoudnessValueLower", 0, 0x3F)) &&
        element->getIntAttribute(dependsOnDrcSet, u"dependsOnDrcSet", true, 0, 0, 0x3F) &&
        element->getBoolAttribute(noIndependentUse, u"noIndependentUse", dependsOnDrcSet == 0) &&
        element->getChildren(xid, u"additionalDownmixId", 0, 7);

    for (auto it = xid.begin(); ok && it != xid.end(); ++it) {
        uint8_t value = 0;
        ok = (*it)->getIntAttribute(value, u"value", true, 0, 0, 0x7F);
        additionalDownmixId.push_back(value);
    }
    return ok;
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DRCInstructions::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(3)) {
        return;
    }
    buf.skipReservedBits(6);
    const uint16_t drcInstructionsType = buf.getBits<uint16_t>(2);
    disp << margin << "DRC instructions type: " << drcInstructionsType << std::endl;
    if (drcInstructionsType == 2) {
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"MAE group id: %n", buf.getBits<uint8_t>(7)) << std::endl;
    }
    else if (drcInstructionsType == 3) {
        buf.skipReservedBits(3);
        disp << margin << UString::Format(u"MAE group preset id: %n", buf.getBits<uint8_t>(5)) << std::endl;
    }
    if (!buf.canReadBytes(3)) {
        return;
    }
    buf.skipReservedBits(2);
    disp << margin << UString::Format(u"DRC set id: %n", buf.getBits<uint8_t>(6)) << std::endl;
    buf.skipReservedBits(1);
    disp << margin << UString::Format(u"Downmix id: %n", buf.getBits<uint8_t>(7)) << std::endl;
    buf.skipReservedBits(3);
    const size_t additionalDownmixIdCount = buf.getBits<size_t>(3);
    const bool limiterPeakTargetPresent = buf.getBool();
    const bool drcSetTargetLoudnessPresent = buf.getBool();
    for (size_t i = 0; buf.canRead() && i < additionalDownmixIdCount; ++i) {
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"Additional downmix id: %n", buf.getBits<uint8_t>(7)) << std::endl;
    }
    if (!buf.canReadBytes(2)) {
        return;
    }
    disp << margin << UString::Format(u"DRC set effect: %n", buf.getUInt16()) << std::endl;
    if (limiterPeakTargetPresent && buf.canRead()) {
        disp << margin << UString::Format(u"Limiter peak target: %n", buf.getUInt8()) << std::endl;
    }
    if (!buf.canReadBytes(2)) {
        return;
    }
    if (drcSetTargetLoudnessPresent) {
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"DRC set target loudness value upper: %n", buf.getBits<uint8_t>(6)) << std::endl;
        const bool drcSetTargetLoudnessValueLowerPresent = buf.getBool();
        if (drcSetTargetLoudnessValueLowerPresent) {
            buf.skipReservedBits(2);
            disp << margin << UString::Format(u"DRC set target loudness value lower: %n", buf.getBits<uint8_t>(6)) << std::endl;
        }
    }
    if (buf.canRead()) {
        buf.skipReservedBits(1);
        const uint8_t dependsOnDrcSet = buf.getBits<uint8_t>(6);
        disp << margin << UString::Format(u"Depends on DRC set: %n", dependsOnDrcSet) << std::endl;
        if (dependsOnDrcSet == 0) {
            disp << margin << UString::Format(u"No independent use: %s", buf.getBool()) << std::endl;
        }
        else {
            buf.skipReservedBits(1);
        }
    }
}


//----------------------------------------------------------------------------
// Loudness info class
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::serialize(PSIBuffer& buf) const
{
    buf.putReserved(6);
    buf.putBits(loudnessInfoType, 2);
    if (loudnessInfoType == 1 || loudnessInfoType == 2) {
        buf.putReserved(1);
        buf.putBits(mae_groupID.value_or(0), 7);
    }
    else if (loudnessInfoType == 3) {
        buf.putReserved(3);
        buf.putBits(mae_groupPresetID.value_or(0), 5);
    }
    if (loudnessInfo.size() > 255) {
        buf.setUserError();
    }
    else {
        buf.putUInt8(uint8_t(loudnessInfo.size()));
        buf.putBytes(loudnessInfo);
    }
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::clear()
{
    mae_groupID.reset();
    mae_groupPresetID.reset();
    loudnessInfo.clear();
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::deserialize(PSIBuffer& buf)
{
    clear();

    buf.skipReservedBits(6);
    buf.getBits(loudnessInfoType, 2);
    if (loudnessInfoType == 1 || loudnessInfoType == 2) {
        buf.skipReservedBits(1);
        buf.getBits(mae_groupID, 7);
    }
    else if (loudnessInfoType == 3) {
        buf.skipReservedBits(3);
        buf.getBits(mae_groupPresetID, 5);
    }
    const size_t loudnessInfo_size = buf.getUInt8();
    buf.getBytes(loudnessInfo, loudnessInfo_size);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"loudnessInfoType", loudnessInfoType);
    root->setOptionalIntAttribute(u"mae_groupID", mae_groupID, true);
    root->setOptionalIntAttribute(u"mae_groupPresetID", mae_groupPresetID, true);
    root->addHexaTextChild(u"loudnessInfo", loudnessInfo, true);
}

bool ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::fromXML(const xml::Element* element)
{
    clear();
    return element->getIntAttribute(loudnessInfoType, u"loudnessInfoType", true, 0, 0, 3) &&
           element->getConditionalIntAttribute(mae_groupID, u"mae_groupID", loudnessInfoType == 1 || loudnessInfoType == 2, 0, 0x7F) &&
           element->getConditionalIntAttribute(mae_groupPresetID, u"mae_groupPresetID", loudnessInfoType == 3, 0, 0x1F) &&
           element->getHexaTextChild(loudnessInfo, u"loudnessInfo", false, 0, 255);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::LoudnessInfo::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(2)) {
        return;
    }
    buf.skipReservedBits(6);
    const uint16_t loudnessInfoType = buf.getBits<uint16_t>(2);
    disp << margin << "Loudness info type: " << loudnessInfoType << std::endl;
    if (loudnessInfoType == 1 || loudnessInfoType == 2) {
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"MAE group id: %n", buf.getBits<uint8_t>(7)) << std::endl;
    }
    else if (loudnessInfoType == 3) {
        buf.skipReservedBits(3);
        disp << margin << UString::Format(u"MAE group preset id: %n", buf.getBits<uint8_t>(5)) << std::endl;
    }
    if (!buf.canReadBytes(1)) {
        return;
    }
    const size_t loudnessInfo_size = buf.getUInt8();
    disp.displayPrivateData(u"loudnessInfo()", buf, loudnessInfo_size, margin);
}


//----------------------------------------------------------------------------
// Downmix id class
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DownmixId::serialize(PSIBuffer& buf) const
{
    buf.putReserved(1);
    buf.putBits(downmixId, 7);
    buf.putBits(downmixType, 2);
    buf.putBits(CICPspeakerLayoutIdx, 6);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DownmixId::deserialize(PSIBuffer& buf)
{
    buf.skipReservedBits(1);
    buf.getBits(downmixId, 7);
    buf.getBits(downmixType, 2);
    buf.getBits(CICPspeakerLayoutIdx, 6);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DownmixId::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"downmixId", downmixId, true);
    root->setIntAttribute(u"downmixType", downmixType);
    root->setIntAttribute(u"CICPspeakerLayoutIdx", CICPspeakerLayoutIdx, true);
}

bool ts::MPEGH3DAudioDRCLoudnessDescriptor::DownmixId::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(downmixId, u"downmixId", true, 0, 0, 0x7F) &&
           element->getIntAttribute(downmixType, u"downmixType", true, 0, 0, 3) &&
           element->getIntAttribute(CICPspeakerLayoutIdx, u"CICPspeakerLayoutIdx", true, 0, 0, 0x3F);
}

void ts::MPEGH3DAudioDRCLoudnessDescriptor::DownmixId::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(2)) {
        buf.skipReservedBits(1);
        disp << margin << UString::Format(u"Downmix id: %n", buf.getBits<uint8_t>(7)) << std::endl;
        disp << margin << UString::Format(u"Downmix type: %d", buf.getBits<uint8_t>(2)) << std::endl;
        disp << margin << UString::Format(u"CICP speaker layout index: %n", buf.getBits<uint8_t>(6)) << std::endl;
    }
}
