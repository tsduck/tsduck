//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVideoWindowDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"video_window_descriptor"
#define MY_CLASS ts::VideoWindowDescriptor
#define MY_DID ts::DID_VIDEO_WIN
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VideoWindowDescriptor::VideoWindowDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::VideoWindowDescriptor::VideoWindowDescriptor(DuckContext& duck, const Descriptor& desc) :
    VideoWindowDescriptor()
{
    deserialize(duck, desc);
}

void ts::VideoWindowDescriptor::clearContent()
{
    horizontal_offset = 0;
    vertical_offset = 0;
    window_priority = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(horizontal_offset, 14);
    buf.putBits(vertical_offset, 14);
    buf.putBits(window_priority, 4);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(horizontal_offset, 14);
    buf.getBits(vertical_offset, 14);
    buf.getBits(window_priority, 4);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Offset x: %d", {buf.getBits<uint16_t>(14)});
        disp << UString::Format(u", y: %d", {buf.getBits<uint16_t>(14)});
        disp << UString::Format(u", window priority: %d", {buf.getBits<uint8_t>(4)})<< std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"horizontal_offset", horizontal_offset);
    root->setIntAttribute(u"vertical_offset", vertical_offset);
    root->setIntAttribute(u"window_priority", window_priority);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VideoWindowDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(horizontal_offset, u"horizontal_offset", true, 0, 0, 0x3FFF) &&
           element->getIntAttribute(vertical_offset, u"vertical_offset", true, 0, 0, 0x3FFF) &&
           element->getIntAttribute(window_priority, u"window_priority", true, 0, 0, 0x0F);
}
