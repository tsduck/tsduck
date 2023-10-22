//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExtendedEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"extended_event_descriptor"
#define MY_CLASS ts::ExtendedEventDescriptor
#define MY_DID ts::DID_EXTENDED_EVENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ExtendedEventDescriptor::NormalizeNumbering(DuckContext& duck, uint8_t* desc_base, size_t desc_size)
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
            UString lang;
            lang.assignFromUTF8(reinterpret_cast<const char*>(data + 1), 3);
            const auto it = desc_last.find(lang);
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
            UString lang;
            lang.assignFromUTF8(reinterpret_cast<const char*>(data + 1), 3);
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
    auto it = entries.begin();

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

void ts::ExtendedEventDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(descriptor_number, 4);
    buf.putBits(last_descriptor_number, 4);
    buf.putLanguageCode(language_code);
    buf.pushWriteSequenceWithLeadingLength(8); // start length_of_items
    for (const auto& it : entries) {
        buf.putStringWithByteLength(it.item_description);
        buf.putStringWithByteLength(it.item);
    }
    buf.popState(); // update length_of_items
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(descriptor_number, 4);
    buf.getBits(last_descriptor_number, 4);
    buf.getLanguageCode(language_code);
    buf.pushReadSizeFromLength(8); // start length_of_items
    while (buf.canRead()) {
        Entry entry;
        buf.getStringWithByteLength(entry.item_description);
        buf.getStringWithByteLength(entry.item);
        entries.push_back(entry);
    }
    buf.popState(); // close length_of_items
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << "Descriptor number: " << buf.getBits<uint32_t>(4);
        disp << ", last: " << buf.getBits<uint32_t>(4) << std::endl;
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        buf.pushReadSizeFromLength(8); // start length_of_items
        while (buf.canRead()) {
            disp << margin << "\"" << buf.getStringWithByteLength();
            disp << "\" : \"" << buf.getStringWithByteLength()<< "\"" << std::endl;
        }
        buf.popState(); // close length_of_items
        disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
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

    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"item");
        e->addElement(u"description")->addText(it.item_description);
        e->addElement(u"name")->addText(it.item);
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
