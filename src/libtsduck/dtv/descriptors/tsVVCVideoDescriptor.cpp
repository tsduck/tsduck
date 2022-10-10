//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_idc(0),
    tier(false),
    sub_profile_idc(),
    progressive_source(false),
    interlaced_source(false),
    non_packed_constraint(false),
    frame_only_constraint(false),
    level_idc(0),
    VVC_still_present(false),
    VVC_24hr_picture_present(false),
    HDR_WCG_idc(3),
    video_properties_tag(0),
    temporal_id_min(),
    temporal_id_max()
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
    temporal_id_min.clear();
    temporal_id_max.clear();
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
    const bool temporal_layer_subset_flag = temporal_id_min.set() && temporal_id_max.set();
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

ts::UString ts::VVCVideoDescriptor::VVCProfileIDC(uint8_t pi)
{
    switch (pi) {
        case  1: return u"Main 10";
        case 17: return u"Multilayer Main 10";
        case 33: return u"Main 10 4:4:4";
        case 49: return u"Multilayer Main 10 4:4:4 ";
        case 65: return u"Main 10 Still Picture";
        case 97: return u"Main 10 4:4:4 Still Picture";
        default: return u"unknown";
    }
}

ts::UString ts::VVCVideoDescriptor::VVCTier(bool t)
{
    return t ? u"High" : u"Main";
}

ts::UString ts::VVCVideoDescriptor::VVCHDRandWCG(uint8_t hw)
{
    // H222.0, TAble 2-134
    switch (hw) {
        case 0: return u"SDR";
        case 1: return u"WCG only";
        case 2: return u"HDR and WCG";
        case 3: return u"no indication";
        default: return u"unknown";
    }
}

ts::UString ts::VVCVideoDescriptor::VVCLevelIDC(uint8_t li)
{
    // H.266, Table A.1
    switch (li) {
        case  16: return u"1.0";
        case  32: return u"2.0";
        case  35: return u"2.1";
        case  48: return u"3.0";
        case  51: return u"3.1";
        case  64: return u"4.0";
        case  67: return u"4.1";
        case  80: return u"5.0";
        case  83: return u"5.1";
        case  86: return u"5.2";
        case  96: return u"6.0";
        case  99: return u"6.1";
        case 102: return u"6.2";
        default: return u"unknown";
    }
}

ts::UString ts::VVCVideoDescriptor::VVCVideoProperties(uint8_t hdr_wcg_idc, uint8_t vprop_tag)
{
    const uint16_t combi_val = uint16_t(uint16_t(hdr_wcg_idc) << 8) | vprop_tag;

    switch (combi_val) {

        // H.222.0 Table 2-135
        case 0x0000: return u"not known";
        case 0x0001: return u"BT709_YCC";
        case 0x0002: return u"BT709_RGB";
        case 0x0003: return u"BT601_525";
        case 0x0004: return u"BT601_625";
        case 0x0005: return u"FR709_RGB";

        // H.222.0 Table 2-136
        case 0x0100: return u"not known";
        case 0x0101: return u"BT2020_YCC_NCL";
        case 0x0102: return u"BT2020_RBG";
        case 0x0103: return u"FR2020_RGB";
        case 0x0104: return u"FRP3D34_YCC";

        // H.222.0 Table 2-137
        case 0x0200: return u"not known";
        case 0x0201: return u"BT2100_PQ_YCC";
        case 0x0202: return u"BT2100_HLG_YCC";
        case 0x0203: return u"BT2100_PQ_ICTCP";
        case 0x0204: return u"BT2100_PQ_RGB";
        case 0x0205: return u"BT2100_HLG_RGB";

        // H.222.0 Table 2-138
        case 0x0300:return u"not known";

        default: return u"reserved";
    }
}

void ts::VVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t profile_idc = buf.getBits<uint8_t>(7);
        disp << margin << "Profile IDC: " << VVCVideoDescriptor::VVCProfileIDC(profile_idc) << " (" << UString::Hexa(profile_idc);
        disp << "), tier: " << VVCVideoDescriptor::VVCTier(buf.getBool()) << std::endl;
        const size_t num_sub_profiles = buf.getUInt8();
        if (num_sub_profiles > 0) {
            disp << margin << "Sub profile IDC:";
            for (uint8_t i = 0; i < num_sub_profiles; i++) {
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
        buf.skipBits(4);
        const uint8_t level_idc = buf.getUInt8();
        disp << margin << "Level IDC: " << VVCVideoDescriptor::VVCLevelIDC(level_idc) << " (" << UString::Hexa(level_idc) << ")";
        const bool temporal = buf.getBool();
        disp << ", still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(5);
        const uint8_t hdr_wcg_idc = buf.getBits<uint8_t>(2);
        disp << margin << "HDR WCG idc: " << VVCVideoDescriptor::VVCHDRandWCG(hdr_wcg_idc) << " (" << int(hdr_wcg_idc) << ")";
        buf.skipBits(2);
        const uint8_t vprop_tag = buf.getBits<uint8_t>(4);
        disp << ", video properties: " << VVCVideoDescriptor::VVCVideoProperties(hdr_wcg_idc, vprop_tag) << " (" << int(vprop_tag) << ")" << std::endl;
        if (temporal && buf.canReadBytes(2)) {
            buf.skipBits(5);
            disp << margin << "Temporal id min: " << buf.getBits<int>(3);
            buf.skipBits(5);
            disp << ", max: " << buf.getBits<uint>(3) << std::endl;
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
    if (ok && temporal_id_min.set() + temporal_id_max.set() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", { element->lineNumber(), element->name() });
        ok = false;
    }
    return ok;
}
