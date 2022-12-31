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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    sb_size(0),
    sb_leak_rate(0),
    DVB_reserved()
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
