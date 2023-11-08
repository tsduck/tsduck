//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"HEVC_video_descriptor"
#define MY_CLASS ts::HEVCVideoDescriptor
#define MY_DID ts::DID_HEVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCVideoDescriptor::HEVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::HEVCVideoDescriptor::clearContent()
{
    profile_space = 0;
    tier = false;
    profile_idc = 0;
    profile_compatibility_indication = 0;
    progressive_source = false;
    interlaced_source = false;
    non_packed_constraint = false;
    frame_only_constraint = false;
    copied_44bits = 0;
    level_idc = 0;
    HEVC_still_present = false;
    HEVC_24hr_picture_present = false;
    sub_pic_hrd_params_not_present = true;
    HDR_WCG_idc = 3;
    temporal_id_min.reset();
    temporal_id_max.reset();
}

ts::HEVCVideoDescriptor::HEVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(profile_space, 2);
    buf.putBit(tier);
    buf.putBits(profile_idc, 5);
    buf.putUInt32(profile_compatibility_indication);
    buf.putBit(progressive_source);
    buf.putBit(interlaced_source);
    buf.putBit(non_packed_constraint);
    buf.putBit(frame_only_constraint);
    buf.putBits(copied_44bits, 44);
    buf.putUInt8(level_idc);
    const bool temporal = temporal_id_min.has_value() && temporal_id_max.has_value();
    buf.putBit(temporal);
    buf.putBit(HEVC_still_present);
    buf.putBit(HEVC_24hr_picture_present);
    buf.putBit(sub_pic_hrd_params_not_present);
    buf.putBits(0xFF, 2);
    buf.putBits(HDR_WCG_idc, 2);
    if (temporal) {
        buf.putBits(temporal_id_min.value(), 3);
        buf.putBits(0xFF, 5);
        buf.putBits(temporal_id_max.value(), 3);
        buf.putBits(0xFF, 5);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(profile_space, 2);
    tier = buf.getBool();
    buf.getBits(profile_idc, 5);
    profile_compatibility_indication = buf.getUInt32();
    progressive_source = buf.getBool();
    interlaced_source = buf.getBool();
    non_packed_constraint = buf.getBool();
    frame_only_constraint = buf.getBool();
    buf.getBits(copied_44bits, 44);
    level_idc = buf.getUInt8();
    const bool temporal = buf.getBool();
    HEVC_still_present = buf.getBool();
    HEVC_24hr_picture_present = buf.getBool();
    sub_pic_hrd_params_not_present = buf.getBool();
    buf.skipBits(2);
    buf.getBits(HDR_WCG_idc, 2);
    if (temporal) {
        buf.getBits(temporal_id_min, 3);
        buf.skipBits(5);
        buf.getBits(temporal_id_max, 3);
        buf.skipBits(5);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(13)) {
        disp << margin << "Profile space: " << buf.getBits<uint16_t>(2);
        disp << ", tier: " << UString::TrueFalse(buf.getBool());
        disp << ", profile IDC: " << buf.getBits<uint16_t>(5) << std::endl;
        disp << margin << "Profile compatibility: " << UString::Hexa(buf.getUInt32()) << std::endl;
        disp << margin << "Progressive source: " << UString::TrueFalse(buf.getBool());
        disp << ", interlaced source: " << UString::TrueFalse(buf.getBool());
        disp << ", non packed: " << UString::TrueFalse(buf.getBool());
        disp << ", frame only: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Copied 44 bits: " << UString::Hexa(buf.getBits<uint64_t>(44), 11) << std::endl;
        disp << margin << "Level IDC: " << int(buf.getUInt8());
        const bool temporal = buf.getBool();
        disp << ", still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "No sub-pic HRD params: " << UString::TrueFalse(buf.getBool());
        buf.skipBits(2);
        disp << ", HDR WCG idc: " << buf.getBits<uint16_t>(2) << std::endl;

        if (temporal && buf.canReadBytes(2)) {
            disp << margin << "Temporal id min: " << buf.getBits<uint16_t>(3);
            buf.skipBits(5);
            disp << ", max: " << buf.getBits<uint16_t>(3) << std::endl;
            buf.skipBits(5);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_space", profile_space, true);
    root->setBoolAttribute(u"tier_flag", tier);
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setIntAttribute(u"profile_compatibility_indication", profile_compatibility_indication, true);
    root->setBoolAttribute(u"progressive_source_flag", progressive_source);
    root->setBoolAttribute(u"interlaced_source_flag", interlaced_source);
    root->setBoolAttribute(u"non_packed_constraint_flag", non_packed_constraint);
    root->setBoolAttribute(u"frame_only_constraint_flag", frame_only_constraint);
    root->setIntAttribute(u"copied_44bits", copied_44bits, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"HEVC_still_present_flag", HEVC_still_present);
    root->setBoolAttribute(u"HEVC_24hr_picture_present_flag", HEVC_24hr_picture_present);
    root->setBoolAttribute(u"sub_pic_hrd_params_not_present", sub_pic_hrd_params_not_present);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setOptionalIntAttribute(u"temporal_id_min", temporal_id_min);
    root->setOptionalIntAttribute(u"temporal_id_max", temporal_id_max);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HEVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(profile_space, u"profile_space", true, 0, 0x00, 0x03) &&
        element->getBoolAttribute(tier, u"tier_flag", true) &&
        element->getIntAttribute(profile_idc, u"profile_idc", true, 0, 0x00, 0x1F) &&
        element->getIntAttribute(profile_compatibility_indication, u"profile_compatibility_indication", true) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        // copied_44bits and reserved_zero_44bits are synonyms
        element->getIntAttribute(copied_44bits, u"copied_44bits", false, 0, 0, 0x00000FFFFFFFFFFF) &&
        element->getIntAttribute(copied_44bits, u"reserved_zero_44bits", false, copied_44bits, 0, 0x00000FFFFFFFFFFF) &&
        element->getIntAttribute(level_idc, u"level_idc", true) &&
        element->getBoolAttribute(HEVC_still_present, u"HEVC_still_present_flag", true) &&
        element->getBoolAttribute(HEVC_24hr_picture_present, u"HEVC_24hr_picture_present_flag", true) &&
        element->getBoolAttribute(sub_pic_hrd_params_not_present, u"sub_pic_hrd_params_not_present", false, true) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", false, 3, 0, 3) &&
        element->getOptionalIntAttribute(temporal_id_min, u"temporal_id_min", 0x00, 0x07) &&
        element->getOptionalIntAttribute(temporal_id_max, u"temporal_id_max", 0x00, 0x07);

    if (ok && temporal_id_min.has_value() + temporal_id_max.has_value() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", {element->lineNumber(), element->name()});
        ok = false;
    }
    return ok;
}
