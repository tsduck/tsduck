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

#include "tsMultilingualComponentDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"multilingual_component_descriptor"
#define MY_XML_ATTR u"description"
#define MY_DID ts::DID_MLINGUAL_COMPONENT

TS_XML_DESCRIPTOR_FACTORY(ts::MultilingualComponentDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::MultilingualComponentDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::MultilingualComponentDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::MultilingualComponentDescriptor::MultilingualComponentDescriptor() :
    AbstractMultilingualDescriptor(MY_DID, MY_XML_NAME, MY_XML_ATTR),
    component_tag(0)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::MultilingualComponentDescriptor::MultilingualComponentDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    MultilingualComponentDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialize / deserialize the prolog (overidden methods).
//----------------------------------------------------------------------------

void ts::MultilingualComponentDescriptor::serializeProlog(const ByteBlockPtr& bbp, const DVBCharset* charset) const
{
    bbp->appendUInt8(component_tag);
}

void ts::MultilingualComponentDescriptor::deserializeProlog(const uint8_t*& data, size_t& size, const DVBCharset* charset)
{
    _is_valid = _is_valid && size >= 1;
    if (_is_valid) {
        component_tag = data[0];
        data++;
        size--;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultilingualComponentDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        // Display prolog here
        strm << margin << UString::Format(u"Component tag: 0x%X (%d)", {data[0], data[0]}) << std::endl;
        // Delegate the rest to the superclass.
        AbstractMultilingualDescriptor::DisplayDescriptor(display, did, data + 1, size - 1, indent, tid, pds);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::MultilingualComponentDescriptor::buildXML(xml::Element* root) const
{
    AbstractMultilingualDescriptor::buildXML(root);
    root->setIntAttribute(u"component_tag", component_tag);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::MultilingualComponentDescriptor::fromXML(const xml::Element* element)
{
    AbstractMultilingualDescriptor::fromXML(element);
    _is_valid = _is_valid && element->getIntAttribute<uint8_t>(component_tag, u"component_tag", true);
}
