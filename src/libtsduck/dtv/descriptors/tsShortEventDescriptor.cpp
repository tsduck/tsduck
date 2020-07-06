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

#include "tsShortEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"short_event_descriptor"
#define MY_CLASS ts::ShortEventDescriptor
#define MY_DID ts::DID_SHORT_EVENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ShortEventDescriptor::ShortEventDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    language_code(),
    event_name(),
    text()
{
}

ts::ShortEventDescriptor::ShortEventDescriptor(const UString& lang_, const UString& name_, const UString& text_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    language_code(lang_),
    event_name(name_),
    text(text_)
{
}

void ts::ShortEventDescriptor::clearContent()
{
    language_code.clear();
    event_name.clear();
    text.clear();
}

ts::ShortEventDescriptor::ShortEventDescriptor(DuckContext& duck, const Descriptor& desc) :
    ShortEventDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Split the content into several ShortEventDescriptor if the content
// is too long and add them in a descriptor list.
//----------------------------------------------------------------------------

size_t ts::ShortEventDescriptor::splitAndAdd(DuckContext& duck, DescriptorList& dlist) const
{
    // Common data in all descriptors.
    ShortEventDescriptor sed;
    sed.language_code = language_code;
    sed.language_code.resize(3, SPACE);

    // We loop on new descriptor generation until the event name and text are
    // fully serialized and at least one descriptor is generated.
    size_t name_index = 0;  // current index in event_name
    size_t text_index = 0;  // current index in text
    size_t desc_count = 0;

    while (desc_count == 0 || name_index < event_name.length() || text_index < text.length()) {

        // Create a new descriptor, reset variable fields, keep common fields.
        sed.event_name.clear();
        sed.text.clear();

        // We simulate the serialization.
        uint8_t buffer[MAX_DESCRIPTOR_SIZE];

        // Descriptor binary size so far, from descriptor_tag to language code, inclusive: 5 bytes.
        // Required minimum remaining space for text: 1 byte.
        size_t remain = MAX_DESCRIPTOR_SIZE - 6;

        // Insert as much as possible of event name.
        uint8_t* addr = buffer;
        const size_t name_size = duck.encodeWithByteLength(addr, remain, event_name, name_index);
        sed.event_name = event_name.substr(name_index, name_size);
        name_index += name_size;

        // In fact, there is one more remaining bytes, the text length.
        remain++;

        // Insert as much as possible of event text.
        const size_t text_size = duck.encodeWithByteLength(addr, remain, text, text_index);
        sed.text = text.substr(text_index, text_size);
        text_index += text_size;

        // Descriptor ready, add it in list
        dlist.add(duck, sed);
        desc_count++;
    }

    return desc_count;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    if (!SerializeLanguageCode(*bbp, language_code)) {
        desc.invalidate();
        return;
    }
    bbp->append(duck.encodedWithByteLength(event_name));
    bbp->append(duck.encodedWithByteLength(text));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 4)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    language_code = DeserializeLanguageCode(data);
    data += 3; size -= 3;

    duck.decodeWithByteLength(event_name, data, size);
    duck.decodeWithByteLength(text, data, size);
    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const UString lang(DeserializeLanguageCode(data));
        data += 3; size -= 3;
        const UString name(duck.decodedWithByteLength(data, size));
        const UString text(duck.decodedWithByteLength(data, size));
        strm << margin << "Language: " << lang << std::endl
             << margin << "Event name: \"" << name << "\"" << std::endl
             << margin << "Description: \"" << text << "\"" << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"language_code", language_code);
    root->addElement(u"event_name")->addText(event_name);
    root->addElement(u"text")->addText(text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ShortEventDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
           element->getTextChild(event_name, u"event_name") &&
           element->getTextChild(text, u"text");
}
