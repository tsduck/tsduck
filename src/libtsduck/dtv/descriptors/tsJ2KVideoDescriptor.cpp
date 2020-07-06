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

#include "tsJ2KVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"J2K_video_descriptor"
#define MY_CLASS ts::J2KVideoDescriptor
#define MY_DID ts::DID_J2K_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::J2KVideoDescriptor::J2KVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_and_level(0),
    horizontal_size(0),
    vertical_size(0),
    max_bit_rate(0),
    max_buffer_size(0),
    DEN_frame_rate(0),
    NUM_frame_rate(0),
    color_specification(0),
    still_mode(false),
    interlaced_video(false),
    private_data()
{
}

void ts::J2KVideoDescriptor::clearContent()
{
    profile_and_level = 0;
    horizontal_size = 0;
    vertical_size = 0;
    max_bit_rate = 0;
    max_buffer_size = 0;
    DEN_frame_rate = 0;
    NUM_frame_rate = 0;
    color_specification = 0;
    still_mode = false;
    interlaced_video = false;
    private_data.clear();
}

ts::J2KVideoDescriptor::J2KVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    J2KVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(profile_and_level);
    bbp->appendUInt32(horizontal_size);
    bbp->appendUInt32(vertical_size);
    bbp->appendUInt32(max_bit_rate);
    bbp->appendUInt32(max_buffer_size);
    bbp->appendUInt16(DEN_frame_rate);
    bbp->appendUInt16(NUM_frame_rate);
    bbp->appendUInt8(color_specification);
    bbp->appendUInt8((still_mode ? 0x80 : 0x00) | (interlaced_video ? 0x40 : 0x00) | 0x3F);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 24;
    private_data.clear();

    if (_is_valid) {
        profile_and_level = GetUInt16(data);
        horizontal_size = GetUInt32(data + 2);
        vertical_size = GetUInt32(data + 6);
        max_bit_rate = GetUInt32(data + 10);
        max_buffer_size = GetUInt32(data + 14);
        DEN_frame_rate = GetUInt16(data + 18);
        NUM_frame_rate = GetUInt16(data + 20);
        color_specification = GetUInt8(data + 22);
        still_mode = (data[23] & 0x80) != 0;
        interlaced_video = (data[23] & 0x40) != 0;
        private_data.copy(data + 24, size - 24);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 24) {
        const uint16_t profile_and_level = GetUInt16(data);
        const uint32_t horizontal_size = GetUInt32(data + 2);
        const uint32_t vertical_size = GetUInt32(data + 6);
        const uint32_t max_bit_rate = GetUInt32(data + 10);
        const uint32_t max_buffer_size = GetUInt32(data + 14);
        const uint16_t DEN_frame_rate = GetUInt16(data + 18);
        const uint16_t NUM_frame_rate = GetUInt16(data + 20);
        const uint8_t color_specification = GetUInt8(data + 22);
        const bool still_mode = (data[23] & 0x80) != 0;
        const bool interlaced_video = (data[23] & 0x40) != 0;

        strm << margin << UString::Format(u"Profile and level: 0x%X (%d)", {profile_and_level, profile_and_level}) << std::endl
             << margin << UString::Format(u"Horizontal size: 0x%X (%d)", {horizontal_size, horizontal_size}) << std::endl
             << margin << UString::Format(u"Vertical size: 0x%X (%d)", {vertical_size, vertical_size}) << std::endl
             << margin << UString::Format(u"Max bit rate: 0x%X (%d)", {max_bit_rate, max_bit_rate}) << std::endl
             << margin << UString::Format(u"Max buffer size: 0x%X (%d)", {max_buffer_size, max_buffer_size}) << std::endl
             << margin << UString::Format(u"Frame rate: %d/%d", {NUM_frame_rate, DEN_frame_rate}) << std::endl
             << margin << UString::Format(u"Color specification: 0x%X (%d)", {color_specification, color_specification}) << std::endl
             << margin << UString::Format(u"Still mode: %s", {still_mode}) << std::endl
             << margin << UString::Format(u"Interlaced video: %s", {interlaced_video}) << std::endl;

        display.displayPrivateData(u"Private data", data + 24, size - 24, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_and_level", profile_and_level, true);
    root->setIntAttribute(u"horizontal_size", horizontal_size);
    root->setIntAttribute(u"vertical_size", vertical_size);
    root->setIntAttribute(u"max_bit_rate", max_bit_rate);
    root->setIntAttribute(u"max_buffer_size", max_buffer_size);
    root->setIntAttribute(u"DEN_frame_rate", DEN_frame_rate);
    root->setIntAttribute(u"NUM_frame_rate", NUM_frame_rate);
    root->setIntAttribute(u"color_specification", color_specification, true);
    root->setBoolAttribute(u"still_mode", still_mode);
    root->setBoolAttribute(u"interlaced_video", interlaced_video);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::J2KVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute<uint16_t>(profile_and_level, u"profile_and_level", true) &&
            element->getIntAttribute<uint32_t>(horizontal_size, u"horizontal_size", true) &&
            element->getIntAttribute<uint32_t>(vertical_size, u"vertical_size", true) &&
            element->getIntAttribute<uint32_t>(max_bit_rate, u"max_bit_rate", true) &&
            element->getIntAttribute<uint32_t>(max_buffer_size, u"max_buffer_size", true) &&
            element->getIntAttribute<uint16_t>(DEN_frame_rate, u"DEN_frame_rate", true) &&
            element->getIntAttribute<uint16_t>(NUM_frame_rate, u"NUM_frame_rate", true) &&
            element->getIntAttribute<uint8_t>(color_specification, u"color_specification", true) &&
            element->getBoolAttribute(still_mode, u"still_mode", true) &&
            element->getBoolAttribute(interlaced_video, u"interlaced_video", true) &&
            element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 26);
}
