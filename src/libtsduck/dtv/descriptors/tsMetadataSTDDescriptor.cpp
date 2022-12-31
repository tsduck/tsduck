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

#include "tsMetadataSTDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"metadata_STD_descriptor"
#define MY_CLASS ts::MetadataSTDDescriptor
#define MY_DID ts::DID_METADATA_STD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MetadataSTDDescriptor::MetadataSTDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    metadata_input_leak_rate(0),
    metadata_buffer_size(0),
    metadata_output_leak_rate(0)
{
}

void ts::MetadataSTDDescriptor::clearContent()
{
    metadata_input_leak_rate = 0;
    metadata_buffer_size = 0;
    metadata_output_leak_rate = 0;
}

ts::MetadataSTDDescriptor::MetadataSTDDescriptor(DuckContext& duck, const Descriptor& desc) :
    MetadataSTDDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MetadataSTDDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(metadata_input_leak_rate, 22);
    buf.putBits(0xFF, 2);
    buf.putBits(metadata_buffer_size, 22);
    buf.putBits(0xFF, 2);
    buf.putBits(metadata_output_leak_rate, 22);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MetadataSTDDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    buf.getBits(metadata_input_leak_rate, 22);
    buf.skipBits(2);
    buf.getBits(metadata_buffer_size, 22);
    buf.skipBits(2);
    buf.getBits(metadata_output_leak_rate, 22);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MetadataSTDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(9)) {
        buf.skipBits(2);
        const uint32_t input = buf.getBits<uint32_t>(22);
        buf.skipBits(2);
        const uint32_t buffer = buf.getBits<uint32_t>(22);
        buf.skipBits(2);
        const uint32_t output = buf.getBits<uint32_t>(22);
        disp << margin << UString::Format(u"Metadata input leak rate: %'d (%'d bits/s)", {input, 400 * input}) << std::endl;
        disp << margin << UString::Format(u"Metadata buffer size: %'d (%'d bytes)", {buffer, 1024 * buffer}) << std::endl;
        disp << margin << UString::Format(u"Metadata output leak rate: %'d (%'d bits/s)", {output, 400 * output}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MetadataSTDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"metadata_input_leak_rate", metadata_input_leak_rate);
    root->setIntAttribute(u"metadata_buffer_size", metadata_buffer_size);
    root->setIntAttribute(u"metadata_output_leak_rate", metadata_output_leak_rate);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MetadataSTDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(metadata_input_leak_rate, u"metadata_input_leak_rate", true, 0, 0, 0x3FFFFF) &&
           element->getIntAttribute(metadata_buffer_size, u"metadata_buffer_size", true, 0, 0, 0x3FFFFF) &&
           element->getIntAttribute(metadata_output_leak_rate, u"metadata_output_leak_rate", true, 0, 0, 0x3FFFFF);
}
