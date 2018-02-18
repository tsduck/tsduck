//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Representation of a component_descriptor
//
//----------------------------------------------------------------------------

#include "tsComponentDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"component_descriptor"
#define MY_DID ts::DID_COMPONENT

TS_XML_DESCRIPTOR_FACTORY(ts::ComponentDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::ComponentDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::ComponentDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ComponentDescriptor::ComponentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    stream_content_ext(0),
    stream_content(0),
    component_type(0),
    component_tag(0),
    language_code(),
    text()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ComponentDescriptor::ComponentDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    ComponentDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8((stream_content_ext << 4) | (stream_content & 0x0F));
    bbp->appendUInt8(component_type);
    bbp->appendUInt8(component_tag);
    if (!SerializeLanguageCode(*bbp, language_code, charset)) {
        desc.invalidate();
        return;
    }
    bbp->append(text.toDVB(0, UString::NPOS, charset));

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 6;

    if (_is_valid) {
        stream_content_ext = (data[0] >> 4) & 0x0F;
        stream_content = data[0] & 0x0F;
        component_type = data[1];
        component_tag = data[2];
        language_code.assign(UString::FromDVB(data + 3, 3, charset));
        text.assign(UString::FromDVB(data + 6, size - 6, charset));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 6) {
        const uint16_t type = GetUInt16(data);
        const uint8_t tag = data[2];
        strm << margin << "Content/type: " << names::ComponentType(type, names::FIRST) << std::endl
             << margin << UString::Format(u"Component tag: %d (0x%X)", {tag, tag}) << std::endl
             << margin << "Language: " << UString::FromDVB(data + 3, 3, display.dvbCharset()) << std::endl;
        data += 6; size -= 6;
        if (size > 0) {
            strm << margin << "Description: \"" << UString::FromDVB(data, size, display.dvbCharset()) << "\"" << std::endl;
        }
        data += size; size = 0;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"stream_content", stream_content, true);
    root->setIntAttribute(u"stream_content_ext", stream_content_ext, true);
    root->setIntAttribute(u"component_type", component_type, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setAttribute(u"language_code", language_code);
    root->setAttribute(u"text", text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ComponentDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(stream_content, u"stream_content", true, 0x00, 0x00, 0x0F) &&
        element->getIntAttribute<uint8_t>(stream_content_ext, u"stream_content_ext", false, 0x0F, 0x00, 0x0F) &&
        element->getIntAttribute<uint8_t>(component_type, u"component_type", true, 0x00, 0x00, 0xFF) &&
        element->getIntAttribute<uint8_t>(component_tag, u"component_tag", false, 0x00, 0x00, 0xFF) &&
        element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
        element->getAttribute(text, u"text", false, u"", 0, MAX_DESCRIPTOR_SIZE - 8);
}
