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

#include "tsVideoDecodeControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"video_decode_control_descriptor"
#define MY_CLASS ts::VideoDecodeControlDescriptor
#define MY_DID ts::DID_ISDB_VIDEO_CONTROL
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VideoDecodeControlDescriptor::VideoDecodeControlDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    still_picture(false),
    sequence_end_code(false),
    video_encode_format(0),
    reserved_future_use(3)
{
}

void ts::VideoDecodeControlDescriptor::clearContent()
{
    still_picture = false;
    sequence_end_code = false;
    video_encode_format = 0;
    reserved_future_use = 3;
}

ts::VideoDecodeControlDescriptor::VideoDecodeControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    VideoDecodeControlDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((still_picture ? 0x80 : 0x00) |
                     (sequence_end_code ? 0x40 : 0x00) |
                     uint8_t((video_encode_format & 0x0F) << 2) |
                     (reserved_future_use & 0x03));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size == 1;

    if (_is_valid) {
        still_picture = (data[0] & 0x80) != 0;
        sequence_end_code = (data[0] & 0x40) != 0;
        video_encode_format = (data[0] >> 2) & 0x0F;
        reserved_future_use = data[0] & 0x03;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        strm << margin << UString::Format(u"Still picture: %s", {(data[0] & 0x80) != 0}) << std::endl
             << margin << UString::Format(u"Sequence end code: %s", {(data[0] & 0x40) != 0}) << std::endl
             << margin << "Video encode format: " << NameFromSection(u"VideoEncodeFormat", (data[0] >> 2) & 0x0F, names::DECIMAL_FIRST) << std::endl
             << margin << UString::Format(u"Reserve future use: %d", {data[0] & 0x03}) << std::endl;
        data++; size--;
    }
    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"still_picture", still_picture);
    root->setBoolAttribute(u"sequence_end_code", sequence_end_code);
    root->setIntAttribute(u"video_encode_format", video_encode_format);
    if (reserved_future_use != 3) {
        root->setIntAttribute(u"reserved_future_use", reserved_future_use);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VideoDecodeControlDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(still_picture, u"still_picture", true) &&
           element->getBoolAttribute(sequence_end_code, u"sequence_end_code", true) &&
           element->getIntAttribute<uint8_t>(video_encode_format, u"video_encode_format", true, 0, 0, 0x0F) &&
           element->getIntAttribute<uint8_t>(reserved_future_use, u"reserved_future_use", false, 3, 0, 3);
}
