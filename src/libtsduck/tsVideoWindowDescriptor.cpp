//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsVideoWindowDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"video_window_descriptor"
#define MY_DID ts::DID_VIDEO_WIN

TS_XML_DESCRIPTOR_FACTORY(ts::VideoWindowDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::VideoWindowDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::VideoWindowDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VideoWindowDescriptor::VideoWindowDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    horizontal_offset(0),
    vertical_offset(0),
    window_priority(0)
{
    _is_valid = true;
}

ts::VideoWindowDescriptor::VideoWindowDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    VideoWindowDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32((uint32_t(horizontal_offset & 0x3FFF) << 18) |
                      (uint32_t(vertical_offset & 0x3FFF) << 4) |
                      (window_priority & 0x0F));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 4;

    if (_is_valid) {
        const uint32_t x = GetUInt32(data);
        horizontal_offset = uint16_t(x >> 18) & 0x3FFF;
        vertical_offset = uint16_t(x >> 4) & 0x3FFF;
        window_priority = uint8_t(x) & 0x0F;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const uint32_t x = GetUInt32(data);
        strm << margin
             << UString::Format(u"Offset x: %d, y: %d, window priority: %d", {(x >> 18) & 0x3FFF, (x >> 4) & 0x3FFF, x & 0x0F})
             << std::endl;
        data += 4; size -= 4;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"horizontal_offset", horizontal_offset);
    root->setIntAttribute(u"vertical_offset", vertical_offset);
    root->setIntAttribute(u"window_priority", window_priority);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::VideoWindowDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint16_t>(horizontal_offset, u"horizontal_offset", true, 0, 0, 0x3FFF) &&
        element->getIntAttribute<uint16_t>(vertical_offset, u"vertical_offset", true, 0, 0, 0x3FFF) &&
        element->getIntAttribute<uint8_t>(window_priority, u"window_priority", true, 0, 0, 0x0F);
}
