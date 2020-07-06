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

#include "tsExtendedEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"extended_event_descriptor"
#define MY_CLASS ts::ExtendedEventDescriptor
#define MY_DID ts::DID_EXTENDED_EVENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    descriptor_number(0),
    last_descriptor_number(0),
    language_code(),
    entries(),
    text()
{
}

ts::ExtendedEventDescriptor::ExtendedEventDescriptor(DuckContext& duck, const Descriptor& desc) :
    ExtendedEventDescriptor()
{
    deserialize(duck, desc);
}

void ts::ExtendedEventDescriptor::clearContent()
{
    descriptor_number = 0;
    last_descriptor_number = 0;
    language_code.clear();
    entries.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Normalize all ExtendedEventDescriptor in a descriptor list.
// Update all descriptor_number and last_descriptor_number.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::NormalizeNumbering(DuckContext& duck,uint8_t* desc_base, size_t desc_size)
{
    typedef std::map<UString, size_t> SizeMap; // key=language_code
    SizeMap desc_last;
    SizeMap desc_index;

    // Count the number of ExtendedEventDescriptor per language in the list
    uint8_t* data = desc_base;
    size_t size = desc_size;
    while (size >= 2) {
        uint8_t tag = data[0];
        uint8_t len = data[1];
        data += 2; size -= 2;
        if (len > size) {
            len = uint8_t(size);
        }
        if (tag == MY_DID && len >= 4) {
            const UString lang(DeserializeLanguageCode(data + 1));
            SizeMap::iterator it(desc_last.find(lang));
            if (it == desc_last.end()) {
                desc_last[lang] = 0;
                desc_index[lang] = 0;
            }
            else {
                it->second++;
            }
        }
        data += len; size -= len;
    }

    // Then, update all ExtendedEventDescriptor in the list
    data = desc_base;
    size = desc_size;
    while (size >= 2) {
        uint8_t tag = data[0];
        uint8_t len = data[1];
        data += 2; size -= 2;
        if (len > size) {
            len = uint8_t(size);
        }
        if (tag == MY_DID && len >= 4) {
            const UString lang(DeserializeLanguageCode(data + 1));
            data[0] = uint8_t((desc_index[lang] & 0x0F) << 4) | (desc_last[lang] & 0x0F);
            desc_index[lang]++;
        }
        data += len; size -= len;
    }
}


//----------------------------------------------------------------------------
// Split the content into several ExtendedEventDescriptor if the content
// is too long and add them in a descriptor list.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::splitAndAdd(DuckContext& duck, DescriptorList& dlist) const
{
    // Common data in all descriptors.
    ExtendedEventDescriptor eed;
    eed.language_code = language_code;
    eed.language_code.resize(3, SPACE);

    // We loop on new descriptor generation until all the following conditions are met:
    // - At least one descriptor was generated.
    // - All entries are serialized.
    // - The event text is fully serialized.
    // We fill each descriptor with complete entries. If an entry does not fit, start a new descriptor.
    // If one entry is so large that it does not fit in a descriptor alone, it is truncated.
    // The event text is potentially split into several descriptors.

    // Iterate over all entries.
    EntryList::const_iterator it = entries.begin();

    // Iterate over event text.
    size_t text_index = 0;

    size_t desc_count = 0;
    while (desc_count == 0 || it != entries.end() || text_index < text.length()) {

        // Create a new descriptor, reset variable fields, keep common fields.
        eed.entries.clear();
        eed.text.clear();

        // We simulate the serialization.
        uint8_t buffer[MAX_DESCRIPTOR_SIZE];

        // Descriptor binary size so far, from descriptor_tag to length_of_items, inclusive: 7 bytes.
        // Required minimum remaining space for text: 1 byte.
        size_t remain = MAX_DESCRIPTOR_SIZE - 8;

        // Insert as many item entries as possible
        while (it != entries.end()) {
            const ByteBlock desc(duck.encodedWithByteLength(it->item_description));
            const ByteBlock item(duck.encodedWithByteLength(it->item));
            if (desc.size() + item.size() > remain) {
                break;
            }
            eed.entries.push_back(*it);
            remain -= desc.size() + item.size();
            ++it;
        }

        // If first entry in current descriptor is too long to fit into one descriptor, truncate it.
        if (it != entries.end() && eed.entries.size() == 0) {
            Entry entry(*it);
            uint8_t* addr = buffer;
            const size_t desc_size = duck.encodeWithByteLength(addr, remain, entry.item_description);
            const size_t item_size = duck.encodeWithByteLength(addr, remain, entry.item);
            assert(desc_size <= entry.item_description.length());
            assert(item_size <= entry.item.length());
            entry.item_description.resize(desc_size);
            entry.item.resize(item_size);
            eed.entries.push_back(entry);
            ++it;
        }

        // In fact, there is one more remaining bytes, the text length.
        remain++;

        // Insert as much as possible of extended description
        uint8_t* addr = buffer;
        const size_t text_size = duck.encodeWithByteLength(addr, remain, text, text_index);
        eed.text = text.substr(text_index, text_size);
        text_index += text_size;

        // Descriptor ready, add it in list
        dlist.add(duck, eed);
        desc_count++;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8(uint8_t(descriptor_number << 4) | (last_descriptor_number & 0x0F));
    if (!SerializeLanguageCode(*bbp, language_code)) {
        desc.invalidate();
        return;
    }

    // Placeholder for length_of_items
    const size_t length_index = bbp->size();
    bbp->appendUInt8(0);

    // Serialize all entries.
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->append(duck.encodedWithByteLength(it->item_description));
        bbp->append(duck.encodedWithByteLength(it->item));
    }

    // Update length_of_items
    (*bbp)[length_index] = uint8_t(bbp->size() - length_index - 1);

    // Final text
    bbp->append(duck.encodedWithByteLength(text));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 5)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    descriptor_number = data[0] >> 4;
    last_descriptor_number = data[0] & 0x0F;
    language_code = DeserializeLanguageCode(data + 1);
    size_t items_length = data[4];
    data += 5; size -= 5;
    _is_valid = items_length < size;

    if (!_is_valid) {
        return;
    }

    size -= items_length;
    entries.clear();
    while (items_length >= 2) {
        Entry entry;
        duck.decodeWithByteLength(entry.item_description, data, items_length);
        duck.decodeWithByteLength(entry.item, data, items_length);
        entries.push_back(entry);
    }

    if (!(_is_valid = items_length == 0 && size > 0)) {
        return;
    }

    duck.decodeWithByteLength(text, data, size);
    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 5) {
        const uint8_t desc_num = data[0];
        const UString lang(DeserializeLanguageCode(data + 1));
        size_t length = data[4];
        data += 5; size -= 5;
        if (length > size) {
            length = size;
        }
        strm << margin << "Descriptor number: " << int((desc_num >> 4) & 0x0F)
             << ", last: " << int(desc_num & 0x0F) << std::endl
             << margin << "Language: " << lang << std::endl;
        size -= length;
        while (length > 0) {
            const UString description(duck.decodedWithByteLength(data, length));
            const UString item(duck.decodedWithByteLength(data, length));
            strm << margin << "\"" << description << "\" : \"" << item << "\"" << std::endl;
        }
        const UString text(duck.decodedWithByteLength(data, size));
        strm << margin << "Text: \"" << text << "\"" << std::endl;
        data += length; size -= length;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_number", descriptor_number, false);
    root->setIntAttribute(u"last_descriptor_number", last_descriptor_number, false);
    root->setAttribute(u"language_code", language_code);
    root->addElement(u"text")->addText(text);

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"item");
        e->addElement(u"description")->addText(it->item_description);
        e->addElement(u"name")->addText(it->item);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ExtendedEventDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(descriptor_number, u"descriptor_number", true) &&
        element->getIntAttribute(last_descriptor_number, u"last_descriptor_number", true) &&
        element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
        element->getTextChild(text, u"text") &&
        element->getChildren(children, u"item");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getTextChild(entry.item_description, u"description") &&
             children[i]->getTextChild(entry.item, u"name");
        entries.push_back(entry);
    }
    return ok;
}
