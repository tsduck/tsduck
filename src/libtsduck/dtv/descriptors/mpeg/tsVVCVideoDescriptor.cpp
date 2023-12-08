//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"VVC_video_descriptor"
#define MY_CLASS ts::VVCVideoDescriptor
#define MY_DID ts::DID_VVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VVCVideoDescriptor::VVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::VVCVideoDescriptor::clearContent()
{
    profile_idc = 0;
    tier = false;
    sub_profile_idc.clear();
    progressive_source = false;
    interlaced_source = false;
    non_packed_constraint = false;
    frame_only_constraint = false;
    level_idc = 0;
    VVC_still_present = false;
    VVC_24hr_picture_present = false;
    HDR_WCG_idc = 3;
    video_properties_tag = 0;
    temporal_id_min.reset();
    temporal_id_max.reset();
}

ts::VVCVideoDescriptor::VVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    VVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(profile_idc, 7);
    buf.putBit(tier);
    buf.putBits(sub_profile_idc.size(), 8);
    for (auto it : sub_profile_idc) {
        buf.putUInt32(it);
    }
    buf.putBit(progressive_source);
    buf.putBit(interlaced_source);
    buf.putBit(non_packed_constraint);
    buf.putBit(frame_only_constraint);
    buf.putBits(0x00, 4);
    buf.putUInt8(level_idc);
    const bool temporal_layer_subset_flag = temporal_id_min.has_value() && temporal_id_max.has_value();
    buf.putBit(temporal_layer_subset_flag);
    buf.putBit(VVC_still_present);
    buf.putBit(VVC_24hr_picture_present);
    buf.putBits(0xFF, 5);
    buf.putBits(HDR_WCG_idc, 2);
    buf.putBits(0xFF, 2);
    buf.putBits(video_properties_tag, 4);
    if (temporal_layer_subset_flag) {
        buf.putBits(0xFF, 5);
        buf.putBits(temporal_id_min.value(), 3);
        buf.putBits(0xFF, 5);
        buf.putBits(temporal_id_max.value(), 3);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(profile_idc, 7);
    tier = buf.getBool();
    uint8_t num_sub_profiles = buf.getUInt8();
    for (uint8_t i = 0; i < num_sub_profiles; i++) {
        sub_profile_idc.push_back(buf.getUInt32());
    }
    progressive_source = buf.getBool();
    interlaced_source = buf.getBool();
    non_packed_constraint = buf.getBool();
    frame_only_constraint = buf.getBool();
    buf.skipBits(4);
    level_idc = buf.getUInt8();
    const bool temporal_layer_subset_flag = buf.getBool();
    VVC_still_present = buf.getBool();
    VVC_24hr_picture_present = buf.getBool();
    buf.skipBits(5);
    buf.getBits(HDR_WCG_idc, 2);
    buf.skipBits(2);
    buf.getBits(video_properties_tag, 4);
    if (temporal_layer_subset_flag) {
        buf.skipBits(5);
        buf.getBits(temporal_id_min, 3);
        buf.skipBits(5);
        buf.getBits(temporal_id_max, 3);
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Profile IDC: " << DataName(MY_XML_NAME, u"profile_idc", buf.getBits<uint8_t>(7), NamesFlags::VALUE);
        disp << ", tier: " << DataName(MY_XML_NAME, u"tier", buf.getBool()) << std::endl;
        const size_t num_sub_profiles = buf.getUInt8();
        if (num_sub_profiles > 0) {
            disp << margin << "Sub profile IDC:";
            for (size_t i = 0; i < num_sub_profiles; i++) {
                disp << " " << UString::Hexa(buf.getUInt32());
                if ((i + 1) % 6 == 0) {
                    disp << std::endl;
                    if (i != (num_sub_profiles - 1)) {
                        disp << margin << "                 ";
                    }
                }
            }
            disp << std::endl;
        }
        disp << margin << "Progressive source: " << UString::TrueFalse(buf.getBool());
        disp << ", interlaced source: " << UString::TrueFalse(buf.getBool());
        disp << ", non packed: " << UString::TrueFalse(buf.getBool());
        disp << ", frame only: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(4, 0);
        disp << margin << "Level IDC: " << DataName(MY_XML_NAME, u"level_idc", buf.getUInt8(), NamesFlags::VALUE);
        const bool temporal = buf.getBool();
        disp << ", still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(5);
        const uint16_t hdr_wcg_idc = buf.getBits<uint16_t>(2);
        disp << margin << "HDR WCG idc: " << DataName(MY_XML_NAME, u"hdr_wcg_idc", hdr_wcg_idc, NamesFlags::VALUE | NamesFlags::DECIMAL);
        buf.skipReservedBits(2);
        const uint16_t vprop = buf.getBits<uint16_t>(4);
        disp << ", video properties: " << DataName(MY_XML_NAME, u"video_properties", (hdr_wcg_idc << 8) | vprop) << " (" << vprop << ")" << std::endl;
        if (temporal && buf.canReadBytes(2)) {
            buf.skipReservedBits(5);
            disp << margin << "Temporal id min: " << buf.getBits<uint16_t>(3);
            buf.skipReservedBits(5);
            disp << ", max: " << buf.getBits<uint16_t>(3) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setBoolAttribute(u"tier_flag", tier);
    for (auto it : sub_profile_idc) {
        root->addElement(u"sub_profile_idc")->setIntAttribute(u"value", it, true);
    }
    root->setBoolAttribute(u"progressive_source_flag", progressive_source);
    root->setBoolAttribute(u"interlaced_source_flag", interlaced_source);
    root->setBoolAttribute(u"non_packed_constraint_flag", non_packed_constraint);
    root->setBoolAttribute(u"frame_only_constraint_flag", frame_only_constraint);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"VVC_still_present_flag", VVC_still_present);
    root->setBoolAttribute(u"VVC_24hr_picture_present_flag", VVC_24hr_picture_present);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setIntAttribute(u"video_properties_tag", video_properties_tag);
    root->setOptionalIntAttribute(u"temporal_id_min", temporal_id_min);
    root->setOptionalIntAttribute(u"temporal_id_max", temporal_id_max);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(profile_idc, u"profile_idc", true, 0, 0x00, 0x7F) &&
        element->getBoolAttribute(tier, u"tier_flag", true) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        element->getIntAttribute(level_idc, u"level_idc", true) &&
        element->getBoolAttribute(VVC_still_present, u"VVC_still_present_flag", true) &&
        element->getBoolAttribute(VVC_24hr_picture_present, u"VVC_24hr_picture_present_flag", true) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", true, 3, 0, 3) &&
        element->getIntAttribute(video_properties_tag, u"video_properties_tag", true, 0, 0, 15) &&
        element->getOptionalIntAttribute(temporal_id_min, u"temporal_id_min", 0, 7) &&
        element->getOptionalIntAttribute(temporal_id_max, u"temporal_id_max", 0, 7) &&
        element->getChildren(children, u"sub_profile_idc");
    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint32_t value = 0;
        ok = children[i]->getIntAttribute(value, u"value", true);
        sub_profile_idc.push_back(value);
    }
    if (ok && temporal_id_min.has_value() + temporal_id_max.has_value() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", { element->lineNumber(), element->name() });
        ok = false;
    }
    return ok;
}
