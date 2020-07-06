//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//
//  Representation of an HEVC_video_descriptor
//
//----------------------------------------------------------------------------

#include "tsHEVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"HEVC_video_descriptor"
#define MY_CLASS ts::HEVCVideoDescriptor
#define MY_DID ts::DID_HEVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCVideoDescriptor::HEVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_space(0),
    tier(false),
    profile_idc(0),
    profile_compatibility_indication(0),
    progressive_source(false),
    interlaced_source(false),
    non_packed_constraint(false),
    frame_only_constraint(false),
    copied_44bits(0),
    level_idc(0),
    HEVC_still_present(false),
    HEVC_24hr_picture_present(false),
    sub_pic_hrd_params_not_present(true),
    HDR_WCG_idc(3),
    temporal_id_min(),
    temporal_id_max()
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
    temporal_id_min.clear();
    temporal_id_max.clear();
}

ts::HEVCVideoDescriptor::HEVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8(uint8_t((profile_space & 0x03) << 6) | (tier ? 0x20 : 0x00) | (profile_idc & 0x1F));
    bbp->appendUInt32(profile_compatibility_indication);
    bbp->appendUInt16((progressive_source    ? 0x8000 : 0x0000) |
                      (interlaced_source     ? 0x4000 : 0x0000) |
                      (non_packed_constraint ? 0x2000 : 0x0000) |
                      (frame_only_constraint ? 0x1000 : 0x0000) |
                      (uint16_t(copied_44bits >> 32) & 0x0FFF));
    bbp->appendUInt32(uint32_t(copied_44bits));
    bbp->appendUInt8(level_idc);
    const bool temporal = temporal_id_min.set() && temporal_id_max.set();
    bbp->appendUInt8((temporal ? 0x80 : 0x00) |
                     (HEVC_still_present ? 0x40 : 0x00) |
                     (HEVC_24hr_picture_present ? 0x20 : 0x00) |
                     (sub_pic_hrd_params_not_present ? 0x10 : 0x00) |
                     0x0C |
                     (HDR_WCG_idc & 0x03));
    if (temporal) {
        bbp->appendUInt8(uint8_t((temporal_id_min.value() << 5) | 0x1F));
        bbp->appendUInt8(uint8_t((temporal_id_max.value() << 5) | 0x1F));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && (desc.payloadSize() == 13 || desc.payloadSize() == 15);

    if (_is_valid) {
        const uint8_t* data = desc.payload();

        profile_space = (data[0] >> 6) & 0x03;
        tier = (data[0] & 0x20) != 0;
        profile_idc = data[0] & 0x1F;
        profile_compatibility_indication = GetUInt32(data + 1);
        progressive_source = (data[5] & 0x80) != 0;
        interlaced_source = (data[5] & 0x40) != 0;
        non_packed_constraint = (data[5] & 0x20) != 0;
        frame_only_constraint = (data[5] & 0x10) != 0;
        copied_44bits = (uint64_t(GetUInt16(data + 5) & 0x0FFF) << 32) | GetUInt32(data + 7);
        level_idc = data[11];
        const bool temporal = (data[12] & 0x80) != 0;
        HEVC_still_present = (data[12] & 0x40) != 0;
        HEVC_24hr_picture_present = (data[12] & 0x20) != 0;
        sub_pic_hrd_params_not_present = (data[12] & 0x10) != 0;
        HDR_WCG_idc = data[12] & 0x03;
        temporal_id_min.clear();
        temporal_id_max.clear();
        if (temporal) {
            _is_valid = desc.payloadSize() >= 15;
            if (_is_valid) {
                temporal_id_min = (data[13] >> 5) & 0x07;
                temporal_id_max = (data[14] >> 5) & 0x07;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 13) {

        const int profile_space = (data[0] >> 6) & 0x03;
        const bool tier = (data[0] & 0x20) != 0;
        const int profile_idc = data[0] & 0x1F;
        const uint32_t profile_compatibility_indication = GetUInt32(data + 1);
        const bool progressive_source = (data[5] & 0x80) != 0;
        const bool interlaced_source = (data[5] & 0x40) != 0;
        const bool non_packed_constraint = (data[5] & 0x20) != 0;
        const bool frame_only_constraint = (data[5] & 0x10) != 0;
        const uint64_t copied_44bits = (uint64_t(GetUInt16(data + 5) & 0x0FFF) << 32) | GetUInt32(data + 7);
        const int level_idc = data[11];
        const bool temporal = (data[12] & 0x80) != 0;
        const bool HEVC_still_present = (data[12] & 0x40) != 0;
        const bool HEVC_24hr_picture_present = (data[12] & 0x20) != 0;
        const bool sub_pic_hrd_params_not_present = (data[12] & 0x10) != 0;
        const uint8_t HDR_WCG_idc = data[12] & 0x03;
        data += 13; size -= 13;

        strm << margin << "Profile space: " << profile_space
             << ", tier: " << UString::TrueFalse(tier)
             << ", profile IDC: " << profile_idc
             << std::endl
             << margin << "Profile compatibility: " << UString::Hexa(profile_compatibility_indication)
             << std::endl
             << margin << "Progressive source: " << UString::TrueFalse(progressive_source)
             << ", interlaced source: " << UString::TrueFalse(interlaced_source)
             << ", non packed: " << UString::TrueFalse(non_packed_constraint)
             << ", frame only: " << UString::TrueFalse(frame_only_constraint)
             << std::endl
             << margin << "Copied 44 bits: " << UString::Hexa(copied_44bits, 11)
             << std::endl
             << margin << "Level IDC: " << level_idc
             << ", still pictures: " << UString::TrueFalse(HEVC_still_present)
             << ", 24-hour pictures: " << UString::TrueFalse(HEVC_24hr_picture_present)
             << std::endl
             << margin << "No sub-pic HRD params: " << UString::TrueFalse(sub_pic_hrd_params_not_present)
             << ", HDR WCG idc: " << UString::Decimal(HDR_WCG_idc)
             << std::endl;

        if (temporal && size >= 2) {
            strm << margin << "Temporal id min: " << int(data[0] >> 5) << ", max: " << int(data[1] >> 5) << std::endl;
            data += 2; size -= 2;
        }
    }

    display.displayExtraData(data, size, indent);
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
        element->getIntAttribute<uint8_t>(profile_space, u"profile_space", true, 0, 0x00, 0x03) &&
        element->getBoolAttribute(tier, u"tier_flag", true) &&
        element->getIntAttribute<uint8_t>(profile_idc, u"profile_idc", true, 0, 0x00, 0x1F) &&
        element->getIntAttribute<uint32_t>(profile_compatibility_indication, u"profile_compatibility_indication", true) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        // copied_44bits and reserved_zero_44bits are synonyms
        element->getIntAttribute<uint64_t>(copied_44bits, u"copied_44bits", false, 0, 0, TS_UCONST64(0x00000FFFFFFFFFFF)) &&
        element->getIntAttribute<uint64_t>(copied_44bits, u"reserved_zero_44bits", false, copied_44bits, 0, TS_UCONST64(0x00000FFFFFFFFFFF)) &&
        element->getIntAttribute<uint8_t>(level_idc, u"level_idc", true) &&
        element->getBoolAttribute(HEVC_still_present, u"HEVC_still_present_flag", true) &&
        element->getBoolAttribute(HEVC_24hr_picture_present, u"HEVC_24hr_picture_present_flag", true) &&
        element->getBoolAttribute(sub_pic_hrd_params_not_present, u"sub_pic_hrd_params_not_present", false, true) &&
        element->getIntAttribute<uint8_t>(HDR_WCG_idc, u"HDR_WCG_idc", false, 3, 0, 3) &&
        element->getOptionalIntAttribute<uint8_t>(temporal_id_min, u"temporal_id_min", 0x00, 0x07) &&
        element->getOptionalIntAttribute<uint8_t>(temporal_id_max, u"temporal_id_max", 0x00, 0x07);

    if (ok && temporal_id_min.set() + temporal_id_max.set() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", {element->lineNumber(), element->name()});
        ok = false;
    }
    return ok;
}
