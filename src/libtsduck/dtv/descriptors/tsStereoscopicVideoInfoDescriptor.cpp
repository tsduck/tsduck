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

#include "tsStereoscopicVideoInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"stereoscopic_video_info_descriptor"
#define MY_CLASS ts::StereoscopicVideoInfoDescriptor
#define MY_DID ts::DID_STEREO_VIDEO_INFO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StereoscopicVideoInfoDescriptor::StereoscopicVideoInfoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    base_video(false),
    leftview(false),
    usable_as_2D(false),
    horizontal_upsampling_factor(0),
    vertical_upsampling_factor(0)
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

void ts::StereoscopicVideoInfoDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(base_video ? 0xFF : 0xFE);
    if (base_video) {
        bbp->appendUInt8(leftview ? 0xFF : 0xFE);
    }
    else {
        bbp->appendUInt8(usable_as_2D ? 0xFF : 0xFE);
        bbp->appendUInt8(uint8_t(horizontal_upsampling_factor << 4) | (vertical_upsampling_factor & 0x0F));
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size > 1;

    if (_is_valid) {
        base_video = (data[0] & 0x01) != 0;
        if (base_video && size == 2) {
            leftview = (data[1] & 0x01) != 0;
        }
        else if (!base_video && size == 3) {
            usable_as_2D = (data[1] & 0x01) != 0;
            horizontal_upsampling_factor = (data[2] >> 4) & 0x0F;
            vertical_upsampling_factor = data[2] & 0x0F;
        }
        else {
            _is_valid = false;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StereoscopicVideoInfoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const bool base = (data[0] & 0x01) != 0;
        strm << margin << UString::Format(u"Base video: %s", {base}) << std::endl;
        data += 1; size -= 1;
        if (base && size >= 1) {
            strm << margin << UString::Format(u"Left view: %s", {(data[0] & 0x01) != 0}) << std::endl;
            data += 1; size -= 1;
        }
        else if (!base && size >= 2) {
            strm << margin << UString::Format(u"Usable as 2D: %s", {(data[0] & 0x01) != 0}) << std::endl
                 << margin << "Horizontal upsampling factor: " << NameFromSection(u"StereoscopicUpsamplingFactor", (data[1] >> 4) & 0x0F, names::DECIMAL_FIRST) << std::endl
                 << margin << "Vertical upsampling factor: " << NameFromSection(u"StereoscopicUpsamplingFactor", data[1] & 0x0F, names::DECIMAL_FIRST) << std::endl;
            data += 2; size -= 2;
        }
    }

    display.displayExtraData(data, size, indent);
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
           element->getIntAttribute<uint8_t>(horizontal_upsampling_factor, u"horizontal_upsampling_factor", !base_video, 0, 0, 15) &&
           element->getIntAttribute<uint8_t>(vertical_upsampling_factor, u"vertical_upsampling_factor", !base_video, 0, 0, 15);
}
