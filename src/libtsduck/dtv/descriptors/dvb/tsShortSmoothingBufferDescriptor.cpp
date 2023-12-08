//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsShortSmoothingBufferDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"short_smoothing_buffer_descriptor"
#define MY_CLASS ts::ShortSmoothingBufferDescriptor
#define MY_DID ts::DID_SHORT_SMOOTH_BUF
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ShortSmoothingBufferDescriptor::ShortSmoothingBufferDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ShortSmoothingBufferDescriptor::ShortSmoothingBufferDescriptor(DuckContext& duck, const Descriptor& desc) :
    ShortSmoothingBufferDescriptor()
{
    deserialize(duck, desc);
}

void ts::ShortSmoothingBufferDescriptor::clearContent()
{
    sb_size = 0;
    sb_leak_rate = 0;
    DVB_reserved.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ShortSmoothingBufferDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(sb_size, 2);
    buf.putBits(sb_leak_rate, 6);
    buf.putBytes(DVB_reserved);
}

void ts::ShortSmoothingBufferDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(sb_size, 2);
    buf.getBits(sb_leak_rate, 6);
    buf.getBytes(DVB_reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortSmoothingBufferDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Smoothing buffer size: %s", {DataName(MY_XML_NAME, u"BufferSize", buf.getBits<uint8_t>(2), NamesFlags::FIRST)}) << std::endl;
        disp << margin << UString::Format(u"Smoothing buffer leak rate: %s", {DataName(MY_XML_NAME, u"LeakRate", buf.getBits<uint8_t>(6), NamesFlags::FIRST)}) << std::endl;
        disp.displayPrivateData(u"DVB-reserved data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ShortSmoothingBufferDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sb_size", sb_size);
    root->setIntAttribute(u"sb_leak_rate", sb_leak_rate);
    root->addHexaText(DVB_reserved, true);
}

bool ts::ShortSmoothingBufferDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(sb_size, u"sb_size", true, 0, 0, 3) &&
           element->getIntAttribute(sb_leak_rate, u"sb_leak_rate", true, 0, 0, 0x3F) &&
           element->getHexaText(DVB_reserved, 0, MAX_DESCRIPTOR_SIZE - 3);
}
