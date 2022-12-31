//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

void ts::J2KVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(profile_and_level);
    buf.putUInt32(horizontal_size);
    buf.putUInt32(vertical_size);
    buf.putUInt32(max_bit_rate);
    buf.putUInt32(max_buffer_size);
    buf.putUInt16(DEN_frame_rate);
    buf.putUInt16(NUM_frame_rate);
    buf.putUInt8(color_specification);
    buf.putBit(still_mode);
    buf.putBit(interlaced_video);
    buf.putBits(0xFF, 6);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_and_level = buf.getUInt16();
    horizontal_size = buf.getUInt32();
    vertical_size = buf.getUInt32();
    max_bit_rate = buf.getUInt32();
    max_buffer_size = buf.getUInt32();
    DEN_frame_rate = buf.getUInt16();
    NUM_frame_rate = buf.getUInt16();
    color_specification = buf.getUInt8();
    still_mode = buf.getBool();
    interlaced_video = buf.getBool();
    buf.skipBits(6);
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::J2KVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(24)) {
        disp << margin << UString::Format(u"Profile and level: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Horizontal size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Vertical size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Max bit rate: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Max buffer size: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        const uint16_t DEN_frame_rate = buf.getUInt16();
        disp << margin << UString::Format(u"Frame rate: %d/%d", {buf.getUInt16(), DEN_frame_rate}) << std::endl;
        disp << margin << UString::Format(u"Color specification: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"Still mode: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Interlaced video: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(6);
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
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
    return  element->getIntAttribute(profile_and_level, u"profile_and_level", true) &&
            element->getIntAttribute(horizontal_size, u"horizontal_size", true) &&
            element->getIntAttribute(vertical_size, u"vertical_size", true) &&
            element->getIntAttribute(max_bit_rate, u"max_bit_rate", true) &&
            element->getIntAttribute(max_buffer_size, u"max_buffer_size", true) &&
            element->getIntAttribute(DEN_frame_rate, u"DEN_frame_rate", true) &&
            element->getIntAttribute(NUM_frame_rate, u"NUM_frame_rate", true) &&
            element->getIntAttribute(color_specification, u"color_specification", true) &&
            element->getBoolAttribute(still_mode, u"still_mode", true) &&
            element->getBoolAttribute(interlaced_video, u"interlaced_video", true) &&
            element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 26);
}
