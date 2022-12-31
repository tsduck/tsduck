//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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

#include "tsLCEVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"LCEVC_video_descriptor"
#define MY_CLASS ts::LCEVCVideoDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_LCEVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LCEVCVideoDescriptor::LCEVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    lcevc_stream_tag(0),
    profile_idc(0),
    level_idc(0),
    sublevel_idc(0),
    processed_planes_type_flag(false),
    picture_type_bit_flag(false),
    field_type_bit_flag(false),
    HDR_WCG_idc(3),
    video_properties_tag(0)
{
}

void ts::LCEVCVideoDescriptor::clearContent()
{
    lcevc_stream_tag = 0;
    profile_idc = 0;
    level_idc = 0;
    sublevel_idc = 0;
    processed_planes_type_flag = false;
    picture_type_bit_flag = false;
    field_type_bit_flag = false;
    HDR_WCG_idc = 3;
    video_properties_tag = 0;
}

ts::LCEVCVideoDescriptor::LCEVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    LCEVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::LCEVCVideoDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LCEVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(lcevc_stream_tag);
    buf.putBits(profile_idc, 4);
    buf.putBits(level_idc, 4);
    buf.putBits(sublevel_idc, 2);
    buf.putBit(processed_planes_type_flag);
    buf.putBit(picture_type_bit_flag);
    buf.putBit(field_type_bit_flag);
    buf.putBits(0xFF, 3);
    buf.putBits(HDR_WCG_idc, 2);
    buf.putBits(0x00, 2);
    buf.putBits(video_properties_tag, 4);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LCEVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    lcevc_stream_tag = buf.getUInt8();
    buf.getBits(profile_idc, 4);
    buf.getBits(level_idc, 4);
    buf.getBits(sublevel_idc, 2);
    processed_planes_type_flag = buf.getBool();
    picture_type_bit_flag = buf.getBool();
    field_type_bit_flag = buf.getBool();
    buf.skipBits(3);
    buf.getBits(HDR_WCG_idc, 2);
    buf.skipBits(2);
    buf.getBits(video_properties_tag, 4);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LCEVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "LCEVC stream tag: " << UString::Hexa(buf.getUInt8());
        disp << ", profile IDC: " << DataName(MY_XML_NAME, u"profile_idc", buf.getBits<uint16_t>(4), NamesFlags::VALUE);
        disp << ", level IDC: " << buf.getBits<uint16_t>(4);
        disp << ", sublevel: " << buf.getBits<uint16_t>(2) << std::endl;
        disp << margin << "Processed planes: " << UString::TrueFalse(buf.getBool());
        disp << ", picture type: " << UString::TrueFalse(buf.getBool());
        disp << ", field type: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(3);
        const uint16_t hdr_wcg_idc = buf.getBits<uint16_t>(2);
        disp << margin << "HDR WCG idc: " << DataName(MY_XML_NAME, u"hdr_wcg_idc", hdr_wcg_idc, NamesFlags::VALUE | NamesFlags::DECIMAL);
        buf.skipReservedBits(2, 0);
        const uint16_t vprop = buf.getBits<uint16_t>(4);
        disp << ", video properties: " << DataName(MY_XML_NAME, u"video_properties", (hdr_wcg_idc << 8) | vprop) << " (" << vprop << ")" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LCEVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"lcevc_stream_tag", lcevc_stream_tag, true);
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setIntAttribute(u"sublevel_idc", sublevel_idc, true);
    root->setBoolAttribute(u"processed_planes_type_flag", processed_planes_type_flag);
    root->setBoolAttribute(u"picture_type_bit_flag", picture_type_bit_flag);
    root->setBoolAttribute(u"field_type_bit_flag", field_type_bit_flag);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setIntAttribute(u"video_properties_tag", video_properties_tag);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LCEVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(lcevc_stream_tag, u"lcevc_stream_tag", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute(profile_idc, u"profile_idc", true, 0, 0x00, 0x0F) &&
        element->getIntAttribute(level_idc, u"level_idc", true, 0, 0x00, 0x0F) &&
        element->getIntAttribute(sublevel_idc, u"sublevel_idc", true, 0, 0x00, 0x03) &&
        element->getBoolAttribute(processed_planes_type_flag, u"processed_planes_type_flag", true) &&
        element->getBoolAttribute(picture_type_bit_flag, u"picture_type_bit_flag", true) &&
        element->getBoolAttribute(field_type_bit_flag, u"field_type_bit_flag", true) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", false, 3, 0, 3) &&
        element->getIntAttribute(video_properties_tag, u"video_properties_tag", false, 0, 0, 15);
    return ok;
}
