//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVideoDecodeControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::VideoDecodeControlDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(still_picture);
    buf.putBit(sequence_end_code);
    buf.putBits(video_encode_format, 4);
    buf.putBits(reserved_future_use, 2);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::deserializePayload(PSIBuffer& buf)
{
    still_picture = buf.getBool();
    sequence_end_code = buf.getBool();
    buf.getBits(video_encode_format, 4);
    buf.getBits(reserved_future_use, 2);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoDecodeControlDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Still picture: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Sequence end code: %s", {buf.getBool()}) << std::endl;
        disp << margin << "Video encode format: " << DataName(MY_XML_NAME, u"EncodeFormat", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
        disp << margin << UString::Format(u"Reserve future use: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
    }
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
           element->getIntAttribute(video_encode_format, u"video_encode_format", true, 0, 0, 0x0F) &&
           element->getIntAttribute(reserved_future_use, u"reserved_future_use", false, 3, 0, 3);
}
