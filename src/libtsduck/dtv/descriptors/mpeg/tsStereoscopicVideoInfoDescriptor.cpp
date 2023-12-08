//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStereoscopicVideoInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"stereoscopic_video_info_descriptor"
#define MY_CLASS ts::StereoscopicVideoInfoDescriptor
#define MY_DID ts::DID_STEREO_VIDEO_INFO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StereoscopicVideoInfoDescriptor::StereoscopicVideoInfoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::StereoscopicVideoInfoDescriptor::clearContent()
{
    base_video = false;
    leftview = false;
    usable_as_2D = false;
    horizontal_upsampling_factor = 0;
    vertical_upsampling_factor = 0;
}

ts::StereoscopicVideoInfoDescriptor::StereoscopicVideoInfoDescriptor(DuckContext& duck, const Descriptor& desc) :
    StereoscopicVideoInfoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 7);
    buf.putBit(base_video);
    if (base_video) {
        buf.putBits(0xFF, 7);
        buf.putBit(leftview);
    }
    else {
        buf.putBits(0xFF, 7);
        buf.putBit(usable_as_2D);
        buf.putBits(horizontal_upsampling_factor, 4);
        buf.putBits(vertical_upsampling_factor, 4);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(7);
    base_video = buf.getBool();
    if (base_video) {
        buf.skipBits(7);
        leftview = buf.getBool();
    }
    else {
        buf.skipBits(7);
        usable_as_2D = buf.getBool();
        buf.getBits(horizontal_upsampling_factor, 4);
        buf.getBits(vertical_upsampling_factor, 4);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(7);
        const bool base = buf.getBool();
        disp << margin << UString::Format(u"Base video: %s", {base}) << std::endl;
        if (base && buf.canReadBytes(1)) {
            buf.skipBits(7);
            disp << margin << UString::Format(u"Left view: %s", {buf.getBool()}) << std::endl;
        }
        else if (!base && buf.canReadBytes(2)) {
            buf.skipBits(7);
            disp << margin << UString::Format(u"Usable as 2D: %s", {buf.getBool()}) << std::endl;
            disp << margin << "Horizontal upsampling factor: " << DataName(MY_XML_NAME, u"UpsamplingFactor", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
            disp << margin << "Vertical upsampling factor: " << DataName(MY_XML_NAME, u"UpsamplingFactor", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"base_video", base_video);
    if (base_video) {
        root->setBoolAttribute(u"leftview", leftview);
    }
    else {
        root->setBoolAttribute(u"usable_as_2D", usable_as_2D);
        root->setIntAttribute(u"horizontal_upsampling_factor", horizontal_upsampling_factor);
        root->setIntAttribute(u"vertical_upsampling_factor", vertical_upsampling_factor);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::StereoscopicVideoInfoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(base_video, u"base_video", true) &&
           element->getBoolAttribute(leftview, u"leftview", base_video) &&
           element->getBoolAttribute(usable_as_2D, u"usable_as_2D", !base_video) &&
           element->getIntAttribute(horizontal_upsampling_factor, u"horizontal_upsampling_factor", !base_video, 0, 0, 15) &&
           element->getIntAttribute(vertical_upsampling_factor, u"vertical_upsampling_factor", !base_video, 0, 0, 15);
}
