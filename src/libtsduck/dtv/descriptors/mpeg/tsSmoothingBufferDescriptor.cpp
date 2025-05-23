//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSmoothingBufferDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"smoothing_buffer_descriptor"
#define MY_CLASS    ts::SmoothingBufferDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_SMOOTH_BUF, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SmoothingBufferDescriptor::SmoothingBufferDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SmoothingBufferDescriptor::clearContent()
{
    sb_leak_rate = 0;
    sb_size = 0;
}

ts::SmoothingBufferDescriptor::SmoothingBufferDescriptor(DuckContext& duck, const Descriptor& desc) :
    SmoothingBufferDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SmoothingBufferDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(sb_leak_rate, 22);
    buf.putBits(0xFF, 2);
    buf.putBits(sb_size, 22);
}

void ts::SmoothingBufferDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    buf.getBits(sb_leak_rate, 22);
    buf.skipBits(2);
    buf.getBits(sb_size, 22);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SmoothingBufferDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(6)) {
        buf.skipBits(2);
        disp << margin << UString::Format(u"Smoothing buffer leak rate: %n x 400 b/s", buf.getBits<uint32_t>(22)) << std::endl;
        buf.skipBits(2);
        disp << margin << UString::Format(u"Smoothing buffer size: %n bytes", buf.getBits<uint32_t>(22)) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SmoothingBufferDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sb_leak_rate", sb_leak_rate, true);
    root->setIntAttribute(u"sb_size", sb_size, true);
}

bool ts::SmoothingBufferDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(sb_leak_rate, u"sb_leak_rate", true, 0, 0, 0x003FFFFF) &&
           element->getIntAttribute(sb_size, u"sb_size", true, 0, 0, 0x003FFFFF);
}
