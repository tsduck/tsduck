//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"HEVC_video_descriptor"
#define MY_DID ts::DID_HEVC_VIDEO

TS_XML_DESCRIPTOR_FACTORY(ts::HEVCVideoDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::HEVCVideoDescriptor, ts::EDID(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::HEVCVideoDescriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::HEVCVideoDescriptor::HEVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    profile_space(0),
    tier(false),
    profile_idc(0),
    profile_compatibility_indication(0),
    progressive_source(false),
    interlaced_source(false),
    non_packed_constraint(false),
    frame_only_constraint(false),
    reserved_zero_44bits(0),
    level_idc(0),
    HEVC_still_present(false),
    HEVC_24hr_picture_present(false),
    temporal_id_min(),
    temporal_id_max()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::HEVCVideoDescriptor::HEVCVideoDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    HEVCVideoDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8(((profile_space & 0x03) << 6) | (tier ? 0x20 : 0x00) | (profile_idc & 0x1F));
    bbp->appendUInt32(profile_compatibility_indication);
    bbp->appendUInt16((progressive_source    ? 0x8000 : 0x0000) |
                      (interlaced_source     ? 0x4000 : 0x0000) |
                      (non_packed_constraint ? 0x2000 : 0x0000) |
                      (frame_only_constraint ? 0x1000 : 0x0000) |
                      (uint16_t(reserved_zero_44bits >> 32) & 0x0FFF));
    bbp->appendUInt32(uint32_t(reserved_zero_44bits));
    bbp->appendUInt8(level_idc);
    const bool temporal = temporal_id_min.set() && temporal_id_max.set();
    bbp->appendUInt8((temporal ? 0x80 : 0x00) | (HEVC_still_present ? 0x40 : 0x00) | (HEVC_24hr_picture_present ? 0x20 : 0x00) | 0x1F);
    if (temporal) {
        bbp->appendUInt8(0xF8 | (temporal_id_min.value() & 0x07));
        bbp->appendUInt8(0xF8 | (temporal_id_max.value() & 0x07));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && (desc.payloadSize() == 13 || desc.payloadSize() == 15);

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
        reserved_zero_44bits = uint64_t(GetUInt16(data + 5) & 0x0FFF) | GetUInt32(data + 7);
        level_idc = data[11];
        const bool temporal = (data[12] & 0x80) != 0;
        HEVC_still_present = (data[12] & 0x40) != 0;
        HEVC_24hr_picture_present = (data[12] & 0x20) != 0;

        temporal_id_min.reset();
        temporal_id_max.reset();
        if (temporal) {
            _is_valid = desc.payloadSize() >= 15;
            if (_is_valid) {
                temporal_id_min = data[13] & 0x07;
                temporal_id_max = data[14] & 0x07;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
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
        const uint64_t reserved_zero_44bits = uint64_t(GetUInt16(data + 5) & 0x0FFF) | GetUInt32(data + 7);
        const int level_idc = data[11];
        const bool temporal = (data[12] & 0x80) != 0;
        const bool HEVC_still_present = (data[12] & 0x40) != 0;
        const bool HEVC_24hr_picture_present = (data[12] & 0x20) != 0;
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
             << margin << "Reserved-zero 44 bits: " << UString::Hexa(reserved_zero_44bits, 11)
             << std::endl
             << margin << "Level IDC: " << level_idc
             << ", still pictures: " << UString::TrueFalse(HEVC_still_present)
             << ", 24-hour pictures: " << UString::TrueFalse(HEVC_24hr_picture_present)
             << std::endl;

        if (temporal && size >= 2) {
            strm << margin << "Temporal id min: " << int(data[0] & 0x07) << ", max: " << int(data[1] & 0x07) << std::endl;
            data += 2; size -= 2;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"profile_space", profile_space, true);
    root->setBoolAttribute(u"tier_flag", tier);
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setIntAttribute(u"profile_compatibility_indication", profile_compatibility_indication, true);
    root->setBoolAttribute(u"progressive_source_flag", progressive_source);
    root->setBoolAttribute(u"interlaced_source_flag", interlaced_source);
    root->setBoolAttribute(u"non_packed_constraint_flag", non_packed_constraint);
    root->setBoolAttribute(u"frame_only_constraint_flag", frame_only_constraint);
    root->setIntAttribute(u"reserved_zero_44bits", reserved_zero_44bits, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"HEVC_still_present_flag", HEVC_still_present);
    root->setBoolAttribute(u"HEVC_24hr_picture_present_flag", HEVC_24hr_picture_present);
    root->setOptionalIntAttribute(u"temporal_id_min", temporal_id_min, true);
    root->setOptionalIntAttribute(u"temporal_id_max", temporal_id_max, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::HEVCVideoDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(profile_space, u"profile_space", true, 0, 0x00, 0x03) &&
        element->getBoolAttribute(tier, u"tier_flag", true) &&
        element->getIntAttribute<uint8_t>(profile_idc, u"profile_idc", true, 0, 0x00, 0x1F) &&
        element->getIntAttribute<uint32_t>(profile_compatibility_indication, u"profile_compatibility_indication", true) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        element->getIntAttribute<uint64_t>(reserved_zero_44bits, u"reserved_zero_44bits", true, 0, 0, TS_UCONST64(0x00000FFFFFFFFFFF)) &&
        element->getIntAttribute<uint8_t>(level_idc, u"level_idc", true) &&
        element->getBoolAttribute(HEVC_still_present, u"HEVC_still_present_flag", true) &&
        element->getBoolAttribute(HEVC_24hr_picture_present, u"HEVC_24hr_picture_present_flag", true) &&
        element->getOptionalIntAttribute<uint8_t>(temporal_id_min, u"temporal_id_min", 0x00, 0x07) &&
        element->getOptionalIntAttribute<uint8_t>(temporal_id_max, u"temporal_id_max", 0x00, 0x07);

    if (_is_valid  && temporal_id_min.set() + temporal_id_max.set() == 1) {
        _is_valid = false;
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", {element->lineNumber(), _xml_name});
    }
}
