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

#include "tsShortSmoothingBufferDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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

void ts::ShortSmoothingBufferDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(sb_size << 6) | (sb_leak_rate & 0x3F));
    bbp->append(DVB_reserved);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ShortSmoothingBufferDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    const size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        sb_size = (data[0] >> 6) & 0x03;
        sb_leak_rate = data[0] & 0x3F;
        DVB_reserved.copy(data + 1, size - 1);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortSmoothingBufferDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"Smoothing buffer size: %s", {NameFromSection(u"SmoothingBufferSize", (data[0] >> 6) & 0x03, names::FIRST)}) << std::endl
             << margin << UString::Format(u"Smoothing buffer leak rate: %s", {NameFromSection(u"SmoothingBufferLeakRate", data[0] & 0x3F, names::FIRST)}) << std::endl;
        display.displayPrivateData(u"DVB-reserved data", data + 1, size - 1, indent);
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


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ShortSmoothingBufferDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(sb_size, u"sb_size", true, 0, 0, 3) &&
           element->getIntAttribute<uint8_t>(sb_leak_rate, u"sb_leak_rate", true, 0, 0, 0x3F) &&
           element->getHexaText(DVB_reserved, 0, MAX_DESCRIPTOR_SIZE - 3);
}
