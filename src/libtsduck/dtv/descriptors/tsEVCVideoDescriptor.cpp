//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"EVC_video_descriptor"
#define MY_CLASS ts::EVCVideoDescriptor
#define MY_DID ts::DID_EVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EVCVideoDescriptor::EVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::EVCVideoDescriptor::clearContent()
{
    profile_idc = 0;
    level_idc = 0;
    toolset_idc_h = 0;
    toolset_idc_l = 0;
    progressive_source = false;
    interlaced_source = false;
    non_packed_constraint = false;
    frame_only_constraint = false;
    EVC_still_present = false;
    EVC_24hr_picture_present = false;
    HDR_WCG_idc = 3;
    video_properties_tag = 0;
    temporal_id_min.reset();
    temporal_id_max.reset();
}

ts::EVCVideoDescriptor::EVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    EVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_idc);
    buf.putUInt8(level_idc);
    buf.putUInt32(toolset_idc_h);
    buf.putUInt32(toolset_idc_l);
    buf.putBit(progressive_source);
    buf.putBit(interlaced_source);
    buf.putBit(non_packed_constraint);
    buf.putBit(frame_only_constraint);
    buf.putBits(0xFF, 1);
    const bool temporal_layer_subset_flag = temporal_id_min.has_value() && temporal_id_max.has_value();
    buf.putBit(temporal_layer_subset_flag);
    buf.putBit(EVC_still_present);
    buf.putBit(EVC_24hr_picture_present);
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

void ts::EVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_idc = buf.getUInt8();
    level_idc = buf.getUInt8();
    toolset_idc_h = buf.getUInt32();
    toolset_idc_l = buf.getUInt32();
    progressive_source = buf.getBool();
    interlaced_source = buf.getBool();
    non_packed_constraint = buf.getBool();
    frame_only_constraint = buf.getBool();
    buf.skipBits(1);
    const bool temporal_layer_subset_flag = buf.getBool();
    EVC_still_present = buf.getBool();
    EVC_24hr_picture_present = buf.getBool();
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

void ts::EVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(12)) {
        disp << margin << "Profile IDC: " << DataName(MY_XML_NAME, u"profile_idc", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", level IDC: "<< DataName(MY_XML_NAME, u"level_idc", buf.getUInt8(), NamesFlags::VALUE) << std::endl;
        disp << margin << "Toolset h: " << UString::Hexa(buf.getUInt32());
        disp << ", l: " << UString::Hexa(buf.getUInt32()) << std::endl;
        disp << margin << "Progressive source: " << UString::TrueFalse(buf.getBool());
        disp << ", interlaced source: " << UString::TrueFalse(buf.getBool());
        disp << ", non packed: " << UString::TrueFalse(buf.getBool());
        disp << ", frame only: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(1);
        const bool temporal = buf.getBool();
        disp << margin << "Still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
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

void ts::EVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setIntAttribute(u"toolset_idc_h", toolset_idc_h, true);
    root->setIntAttribute(u"toolset_idc_l", toolset_idc_l, true);
    root->setBoolAttribute(u"progressive_source_flag", progressive_source);
    root->setBoolAttribute(u"interlaced_source_flag", interlaced_source);
    root->setBoolAttribute(u"non_packed_constraint_flag", non_packed_constraint);
    root->setBoolAttribute(u"frame_only_constraint_flag", frame_only_constraint);
    root->setBoolAttribute(u"EVC_still_present_flag", EVC_still_present);
    root->setBoolAttribute(u"EVC_24hr_picture_present_flag", EVC_24hr_picture_present);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setIntAttribute(u"video_properties_tag", video_properties_tag);
    root->setOptionalIntAttribute(u"temporal_id_min", temporal_id_min);
    root->setOptionalIntAttribute(u"temporal_id_max", temporal_id_max);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(profile_idc, u"profile_idc", true) &&
        element->getIntAttribute(level_idc, u"level_idc", true) &&
        element->getIntAttribute(toolset_idc_h, u"toolset_idc_h", true) &&
        element->getIntAttribute(toolset_idc_l, u"toolset_idc_l", true) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        element->getBoolAttribute(EVC_still_present, u"EVC_still_present_flag", true) &&
        element->getBoolAttribute(EVC_24hr_picture_present, u"EVC_24hr_picture_present_flag", true) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", false, 3, 0, 3) &&
        element->getIntAttribute(video_properties_tag, u"video_properties_tag", false, 0, 0, 15) &&
        element->getOptionalIntAttribute(temporal_id_min, u"temporal_id_min", 0, 7) &&
        element->getOptionalIntAttribute(temporal_id_max, u"temporal_id_max", 0, 7);

    if (ok && temporal_id_min.has_value() + temporal_id_max.has_value() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", {element->lineNumber(), element->name()});
        ok = false;
    }
    return ok;
}
