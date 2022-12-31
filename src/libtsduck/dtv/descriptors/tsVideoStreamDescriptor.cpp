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

#include "tsVideoStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"video_stream_descriptor"
#define MY_CLASS ts::VideoStreamDescriptor
#define MY_DID ts::DID_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VideoStreamDescriptor::VideoStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    multiple_frame_rate(false),
    frame_rate_code(0),
    MPEG_1_only(false),
    constrained_parameter(false),
    still_picture(false),
    profile_and_level_indication(0),
    chroma_format(0),
    frame_rate_extension(false)
{
}

ts::VideoStreamDescriptor::VideoStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    VideoStreamDescriptor()
{
    deserialize(duck, desc);
}

void ts::VideoStreamDescriptor::clearContent()
{
    multiple_frame_rate = false;
    frame_rate_code = 0;
    MPEG_1_only = false;
    constrained_parameter = false;
    still_picture = false;
    profile_and_level_indication = 0;
    chroma_format = 0;
    frame_rate_extension = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(multiple_frame_rate);
    buf.putBits(frame_rate_code, 4);
    buf.putBit(MPEG_1_only);
    buf.putBit(constrained_parameter);
    buf.putBit(still_picture);
    if (!MPEG_1_only) {
        buf.putUInt8(profile_and_level_indication);
        buf.putBits(chroma_format, 2);
        buf.putBit(frame_rate_extension);
        buf.putBits(0xFF, 5);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    multiple_frame_rate = buf.getBool();
    buf.getBits(frame_rate_code, 4);
    MPEG_1_only = buf.getBool();
    constrained_parameter = buf.getBool();
    still_picture = buf.getBool();
    if (!MPEG_1_only) {
        profile_and_level_indication = buf.getUInt8();
        buf.getBits(chroma_format, 2);
        frame_rate_extension = buf.getBool();
        buf.skipReservedBits(5);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        disp << margin << UString::Format(u"Multiple frame rate: %s", {buf.getBool()});
        disp << ", frame rate: " << NameFromDTV(u"mpeg2.frame_rate", buf.getBits<uint8_t>(4), NamesFlags::FIRST) << std::endl;
        const bool mp1only = buf.getBool();
        disp << margin << UString::Format(u"MPEG-1 only: %s, constained parameter: %s", {mp1only, buf.getBool()});
        disp << UString::Format(u", still picture: %s", {buf.getBool()}) << std::endl;
        if (!mp1only && buf.canRead()) {
            disp << margin << UString::Format(u"Profile and level: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            disp << margin << "Chroma format: " << NameFromDTV(u"mpeg2.chroma_format", buf.getBits<uint8_t>(2), NamesFlags::FIRST) << std::endl;
            disp << margin << UString::Format(u"Frame rate extension: %s", {buf.getBool()}) << std::endl;
            buf.skipReservedBits(5);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"multiple_frame_rate", multiple_frame_rate);
    root->setIntAttribute(u"frame_rate_code", frame_rate_code);
    root->setBoolAttribute(u"MPEG_1_only", MPEG_1_only);
    root->setBoolAttribute(u"constrained_parameter", constrained_parameter);
    root->setBoolAttribute(u"still_picture", still_picture);
    if (!MPEG_1_only) {
        root->setIntAttribute(u"profile_and_level_indication", profile_and_level_indication, true);
        root->setIntAttribute(u"chroma_format", chroma_format);
        root->setBoolAttribute(u"frame_rate_extension", frame_rate_extension);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VideoStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(multiple_frame_rate, u"multiple_frame_rate", true) &&
            element->getIntAttribute(frame_rate_code, u"frame_rate_code", true, 0, 0x00, 0x0F) &&
            element->getBoolAttribute(MPEG_1_only, u"MPEG_1_only", true) &&
            element->getBoolAttribute(constrained_parameter, u"constrained_parameter", true) &&
            element->getBoolAttribute(still_picture, u"still_picture", true) &&
            element->getIntAttribute(profile_and_level_indication, u"profile_and_level_indication", !MPEG_1_only) &&
            element->getIntAttribute(chroma_format, u"chroma_format", !MPEG_1_only, 0, 0x00, 0x03) &&
            element->getBoolAttribute(frame_rate_extension, u"frame_rate_extension", !MPEG_1_only);
}
