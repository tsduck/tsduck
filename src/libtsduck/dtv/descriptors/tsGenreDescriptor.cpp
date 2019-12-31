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

#include "tsGenreDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"genre_descriptor"
#define MY_DID ts::DID_ATSC_GENRE
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::STD_ATSC

TS_XML_DESCRIPTOR_FACTORY(ts::GenreDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::GenreDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::GenreDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::GenreDescriptor::GenreDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    attributes()
{
    _is_valid = true;
}

ts::GenreDescriptor::GenreDescriptor(DuckContext& duck, const Descriptor& desc) :
    GenreDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(0xE0 | (attributes.size() & 0x1F)));
    bbp->append(attributes);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    attributes.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size > 0;

    if (_is_valid) {
        const size_t count = data[0] & 0x1F;
        _is_valid = 1 + count <= size;
        if (_is_valid) {
            attributes.copy(data + 1, count);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::GenreDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size > 0) {
        std::ostream& strm(display.duck().out());
        const std::string margin(indent, ' ');

        size_t count = data[0] & 0x1F;
        data++; size--;

        strm << margin << UString::Format(u"Attribute count: %d", {count}) << std::endl;
        while (count > 0 && size > 0) {
            strm << margin << " - Attribute: " << NameFromSection(u"ATSCGenreCode", data[0], names::FIRST) << std::endl;
            data++; size--; count--;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (size_t i = 0; i < attributes.size(); ++i) {
        root->addElement(u"attribute")->setIntAttribute(u"value", attributes[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    attributes.clear();
    xml::ElementVector children;

    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"attribute", 0, 0x1F);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        uint8_t attr = 0;
        _is_valid = children[i]->getIntAttribute<uint8_t>(attr, u"value", true);
        if (_is_valid) {
            attributes.push_back(attr);
        }
    }
}
