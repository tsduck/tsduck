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
//
//  Representation of a stream_identifier_descriptor.
//
//----------------------------------------------------------------------------

#include "tsStreamIdentifierDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"stream_identifier_descriptor"
#define MY_DID ts::DID_STREAM_ID

TS_XML_DESCRIPTOR_FACTORY(ts::StreamIdentifierDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::StreamIdentifierDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::StreamIdentifierDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::StreamIdentifierDescriptor::StreamIdentifierDescriptor(uint8_t ctag) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    component_tag(ctag)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::StreamIdentifierDescriptor::StreamIdentifierDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    component_tag(0)
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (3));
    CheckNonNull (bbp.pointer());

    (*bbp)[0] = _tag;
    (*bbp)[1] = 1;
    (*bbp)[2] = component_tag;
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 1;

    if (_is_valid) {
        component_tag = GetUInt8 (desc.payload());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());

    if (size >= 1) {
        uint8_t id = data[0];
        data += 1; size -= 1;
        strm << UString::Format(u"%*sComponent tag: %d (0x%X)", {indent, u"", id, id}) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"component_tag", component_tag, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(component_tag, u"component_tag", true);
}
