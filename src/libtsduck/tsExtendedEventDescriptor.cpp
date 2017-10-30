//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of an extended_event_descriptor
//
//----------------------------------------------------------------------------

#include "tsExtendedEventDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsUString.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::ExtendedEventDescriptor, "extended_event_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::ExtendedEventDescriptor, ts::EDID(ts::DID_EXTENDED_EVENT));
TS_ID_DESCRIPTOR_DISPLAY(ts::ExtendedEventDescriptor::DisplayDescriptor, ts::EDID(ts::DID_EXTENDED_EVENT));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor() :
    AbstractDescriptor(DID_EXTENDED_EVENT, "extended_event_descriptor"),
    descriptor_number(0),
    last_descriptor_number(0),
    language_code(),
    entries(),
    text()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(DID_EXTENDED_EVENT, "extended_event_descriptor"),
    descriptor_number(0),
    last_descriptor_number(0),
    language_code(),
    entries(),
    text()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Normalize all ExtendedEventDescriptor in a descriptor list.
// Update all descriptor_number and last_descriptor_number.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::NormalizeNumbering(uint8_t* desc_base, size_t desc_size, const DVBCharset* charset)
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
        if (tag == DID_EXTENDED_EVENT && len >= 4) {
            const UString lang(UString::FromDVB(data + 1, 3, charset));
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
        if (tag == DID_EXTENDED_EVENT && len >= 4) {
            const UString lang(UString::FromDVB(data + 1, 3, charset));
            data[0] = ((desc_index[lang] & 0x0F) << 4) | (desc_last[lang] & 0x0F);
            desc_index[lang]++;
        }
        data += len; size -= len;
    }
}


//----------------------------------------------------------------------------
// Split the content into several ExtendedEventDescriptor if the content
// is too long and add them in a descriptor list.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::splitAndAdd(DescriptorList& dlist, const DVBCharset* charset) const
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
            const ByteBlock desc(it->item_description.toDVBWithByteLength(0, UString::NPOS, charset));
            const ByteBlock item(it->item.toDVBWithByteLength(0, UString::NPOS, charset));
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
            const size_t desc_size = entry.item_description.toDVBWithByteLength(addr, remain, 0, UString::NPOS, charset);
            const size_t item_size = entry.item.toDVBWithByteLength(addr, remain, 0, UString::NPOS, charset);
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
        const size_t text_size = text.toDVBWithByteLength(addr, remain, text_index, UString::NPOS, charset);
        eed.text = text.substr(text_index, text_size);
        text_index += text_size;

        // Descriptor ready, add it in list
        dlist.add(eed);
        desc_count++;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8((descriptor_number << 4) | (last_descriptor_number & 0x0F));
    if (!SerializeLanguageCode(*bbp, language_code, charset)) {
        desc.invalidate();
        return;
    }

    // Placeholder for length_of_items
    const size_t length_index = bbp->size();
    bbp->appendUInt8(0);

    // Serialize all entries.
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->append(it->item_description.toDVBWithByteLength(0, UString::NPOS, charset));
        bbp->append(it->item.toDVBWithByteLength(0, UString::NPOS, charset));
    }

    // Update length_of_items
    (*bbp)[length_index] = uint8_t(bbp->size() - length_index - 1);

    // Final text
    bbp->append(text.toDVBWithByteLength(0, UString::NPOS, charset));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 5)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    descriptor_number = data[0] >> 4;
    last_descriptor_number = data[0] & 0x0F;
    language_code = UString::FromDVB(data + 1, 3, charset);
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
        entry.item_description = UString::FromDVBWithByteLength(data, items_length, charset);
        entry.item = UString::FromDVBWithByteLength(data, items_length, charset);
        entries.push_back(entry);
    }

    if (!(_is_valid = items_length == 0 && size > 0)) {
        return;
    }

    text = UString::FromDVBWithByteLength(data, size, charset);
    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 5) {
        const uint8_t desc_num = data[0];
        const UString lang(UString::FromDVB(data + 1, 3, display.dvbCharset()));
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
            const UString description(UString::FromDVBWithByteLength(data, length, display.dvbCharset()));
            const UString item(UString::FromDVBWithByteLength(data, length, display.dvbCharset()));
            strm << margin << "\"" << description << "\" : \"" << item << "\"" << std::endl;
        }
        const UString text(UString::FromDVBWithByteLength(data, size, display.dvbCharset()));
        strm << margin << "Text: \"" << text << "\"" << std::endl;
        data += length; size -= length;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::ExtendedEventDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "descriptor_number", descriptor_number, false);
    xml.setIntAttribute(root, "last_descriptor_number", last_descriptor_number, false);
    xml.setAttribute(root, "language_code", language_code);
    xml.addText(xml.addElement(root, "text"), text);

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        XML::Element* e = xml.addElement(root, "item");
        xml.addText(xml.addElement(e, "description"), it->item_description);
        xml.addText(xml.addElement(e, "name"), it->item);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    language_code.clear();
    text.clear();
    entries.clear();

    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute(descriptor_number, element, "descriptor_number", true) &&
        xml.getIntAttribute(last_descriptor_number, element, "last_descriptor_number", true) &&
        xml.getAttribute(language_code, element, "language_code", true, "", 3, 3) &&
        xml.getTextChild(text, element, "text") &&
        xml.getChildren(children, element, "item");

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            xml.getTextChild(entry.item_description, children[i], "description") &&
            xml.getTextChild(entry.item, children[i], "name");
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
