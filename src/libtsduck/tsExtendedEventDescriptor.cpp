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


#define MAX_DESC_SIZE 255

//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor() :
    AbstractDescriptor (DID_EXTENDED_EVENT),
    descriptor_number (0),
    last_descriptor_number (0),
    language_code (),
    entries (),
    text ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ExtendedEventDescriptor::ExtendedEventDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_EXTENDED_EVENT),
    descriptor_number (0),
    last_descriptor_number (0),
    language_code (),
    entries (),
    text ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Split the content into several ExtendedEventDescriptor if the content
// is too long and add them in a descriptor list.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::splitAndAdd (DescriptorList& dlist) const
{
    ExtendedEventDescriptor eed;
    eed.language_code = language_code;
    eed.language_code.resize (3, ' ');

    size_t desc_count = 0;
    EntryList::const_iterator it = entries.begin();
    const bool text_has_table = !text.empty() && text[0] > 0 && text[0] < 0x20;
    size_t text_index = text_has_table ? 1 : 0;

    while (desc_count == 0 || it != entries.end() || text_index < text.length()) {

        // Create a new descriptor
        eed.entries.clear();
        eed.text.clear();

        // Descriptor binary size so far, from descriptor_tag to length_of_items, inclusive
        size_t size = 7;

        // Insert as many item entries as possible
        while (it != entries.end() && size + 1 + it->item_description.length() + 1 + it->item.length() + 1 <= MAX_DESC_SIZE) {
            eed.entries.push_back (*it);
            size += 1 + it->item_description.length() + 1 + it->item.length();
            ++it;
        }
        assert (size < MAX_DESC_SIZE);

        // If first entry is too long to fit into one descriptor, truncate it
        if (it != entries.end() && eed.entries.size() == 0) {
            Entry entry (*it);
            if (size + 1 + entry.item_description.length() + 2 > MAX_DESC_SIZE) {
                entry.item_description.resize (MAX_DESC_SIZE - size - 3);
            }
            if (size + 1 + entry.item_description.length() + 1 + entry.item.length() + 1 > MAX_DESC_SIZE) {
                entry.item.resize (MAX_DESC_SIZE - size - 1 - entry.item_description.length() - 2);
            }
            eed.entries.push_back (entry);
            size += 1 + entry.item_description.length() + 1 + entry.item.length();
            ++it;
        }
        assert (size < MAX_DESC_SIZE);

        // Insert as much as possible of extended description
        const size_t max_size = MAX_DESC_SIZE - size - 1;
        if (!text_has_table || text_index == text.length()) {
            const size_t length = std::min (max_size, text.length() - text_index);
            eed.text = text.substr (text_index, length);
            text_index += length;
        }
        else if (max_size > 1) {
            const size_t length = std::min (max_size - 1, text.length() - text_index);
            eed.text.resize (1);
            eed.text[0] = text[0];
            eed.text.append (text.substr (text_index, length));
            text_index += length;
        }
        else {
            eed.text.clear();
        }

        // Descriptor ready, add it in list
        dlist.add (eed);
        desc_count++;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::serialize (Descriptor& desc) const
{
    if (language_code.length() != 3) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt8 ((descriptor_number << 4) | (last_descriptor_number & 0x0F));
    bbp->append (language_code);

    // Placeholder for length_of_items
    const size_t length_index = bbp->size();
    bbp->appendUInt8 (0);

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (bbp->size() + 1 + it->item_description.length() + 1 + it->item.length() + 1 > MAX_DESC_SIZE) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8 (uint8_t(it->item_description.length()));
        bbp->append (it->item_description);
        bbp->appendUInt8 (uint8_t(it->item.length()));
        bbp->append (it->item);
    }

    // Update length_of_items
    (*bbp)[length_index] = uint8_t(bbp->size() - length_index - 1);

    if (bbp->size() + 1 + text.length() > MAX_DESC_SIZE) {
        desc.invalidate();
        return;
    }
    bbp->appendUInt8 (uint8_t(text.length()));
    bbp->append (text);
    
    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::deserialize (const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 5)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    descriptor_number = data[0] >> 4;
    last_descriptor_number = data[0] & 0x0F;
    language_code = std::string (reinterpret_cast <const char*> (data + 1), 3);
    size_t items_length = data[4];
    data += 5; size -= 5;

    _is_valid = items_length < size;
    if (!_is_valid) {
        return;
    }
    entries.clear();
    while (items_length >= 2) {
        size_t desc_length = data[0];
        data++; size--; items_length--;
        _is_valid = desc_length < items_length;
        if (!_is_valid) {
            return;
        }
        Entry entry;
        entry.item_description = std::string (reinterpret_cast <const char*> (data), desc_length);
        size_t item_length = data[desc_length];
        data += desc_length + 1; size -= desc_length + 1; items_length -= desc_length + 1;
        _is_valid = item_length <= items_length;
        if (!_is_valid) {
                return;
        }
        entry.item = std::string (reinterpret_cast <const char*> (data), item_length);
        data += item_length; size -= item_length; items_length -= item_length;
        entries.push_back (entry);
    }
    _is_valid = items_length == 0 && size > 0;
    if (!_is_valid) {
        return;
    }

    size_t text_length = data[0];
    data++; size--;
    _is_valid = text_length <= size;
    if (!_is_valid) {
        return;
    }
    text = std::string (reinterpret_cast <const char*> (data), text_length);
    data += text_length; size -= text_length;
}


//----------------------------------------------------------------------------
// Normalize all ExtendedEventDescriptor in a descriptor list.
// Update all descriptor_number and last_descriptor_number.
//----------------------------------------------------------------------------

void ts::ExtendedEventDescriptor::NormalizeNumbering (uint8_t* desc_base, size_t desc_size)
{
    typedef std::map <std::string, size_t> SizeMap; // key=language_code
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
            const std::string lang (std::string (reinterpret_cast <const char*> (data + 1), 3));
            SizeMap::iterator it (desc_last.find (lang));
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
            const std::string lang (std::string (reinterpret_cast <const char*> (data + 1), 3));
            data[0] = ((desc_index[lang] & 0x0F) << 4) | (desc_last[lang] & 0x0F);
            desc_index[lang]++;
        }
        data += len; size -= len;
    }
}
