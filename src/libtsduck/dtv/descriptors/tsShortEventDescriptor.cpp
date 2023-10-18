//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsShortEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"short_event_descriptor"
#define MY_CLASS ts::ShortEventDescriptor
#define MY_DID ts::DID_SHORT_EVENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ShortEventDescriptor::ShortEventDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ShortEventDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putStringWithByteLength(event_name);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getStringWithByteLength(event_name);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Event name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Description: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
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
