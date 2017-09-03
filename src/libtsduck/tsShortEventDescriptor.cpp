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
//  Representation of an short_event_descriptor
//
//----------------------------------------------------------------------------

#include "tsShortEventDescriptor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ShortEventDescriptor::ShortEventDescriptor() :
    AbstractDescriptor (DID_SHORT_EVENT),
    language_code (),
    event_name (),
    text ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor with description
//----------------------------------------------------------------------------

ts::ShortEventDescriptor::ShortEventDescriptor (const std::string& lang_, const std::string& name_, const std::string& text_) :
    AbstractDescriptor (DID_SHORT_EVENT),
    language_code (lang_),
    event_name (name_),
    text (text_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ShortEventDescriptor::ShortEventDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_SHORT_EVENT),
    language_code (),
    event_name (),
    text ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Split the content into several ShortEventDescriptor if the content
// is too long and add them in a descriptor list.
//----------------------------------------------------------------------------

namespace {
    void Insert (std::string& segment, const std::string& original, bool has_table, size_t& index, size_t max_size)
    {
        assert (index <= original.length());
        if (!has_table || index == original.length()) {
            const size_t length = std::min (max_size, original.length() - index);
            segment = original.substr (index, length);
            index += length;
        }
        else if (max_size > 1) {
            const size_t length = std::min (max_size - 1, original.length() - index);
            segment.resize (1);
            segment[0] = original[0];
            segment.append (original.substr (index, length));
            index += length;
        }
        else {
            segment.clear();
        }
    }
}

size_t ts::ShortEventDescriptor::splitAndAdd (DescriptorList& dlist) const
{
    ShortEventDescriptor sed;
    sed.language_code = language_code;
    sed.language_code.resize (3, ' ');

    const bool name_has_table = !event_name.empty() && event_name[0] > 0 && event_name[0] < 0x20;
    const bool text_has_table = !text.empty() && text[0] > 0 && text[0] < 0x20;
    size_t name_index = name_has_table ? 1 : 0;
    size_t text_index = text_has_table ? 1 : 0;
    size_t desc_count = 0;

    while (desc_count == 0 || name_index < event_name.length() || text_index < text.length()) {
        Insert (sed.event_name, event_name, name_has_table, name_index, 249);
        Insert (sed.text, text, text_has_table, text_index, 249 - sed.event_name.length());
        dlist.add (sed);
        desc_count++;
    }

    return desc_count;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::serialize (Descriptor& desc) const
{
    if (language_code.length() != 3 || event_name.length() + text.length() > 249) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->append(language_code);
    bbp->appendUInt8(uint8_t(event_name.length()));
    bbp->append(event_name);
    bbp->appendUInt8(uint8_t(text.length()));
    bbp->append(text);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::deserialize (const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 4)) {
        return;
    }
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    language_code = std::string (reinterpret_cast <const char*> (data), 3);
    size_t name_length = data[3];
    data += 4; size -= 4;
    _is_valid = name_length < size;
    if (!_is_valid) {
        return;
    }
    event_name = std::string (reinterpret_cast <const char*> (data), name_length);
    size_t text_length = data[name_length];
    data += name_length + 1; size -= name_length + 1;
    _is_valid = text_length <= size;
    if (!_is_valid) {
        return;
    }
    text = std::string (reinterpret_cast <const char*> (data), text_length);
    data += text_length; size -= text_length;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const std::string lang(Printable(data, 3));
        size_t length = data[3];
        data += 4; size -= 4;
        if (length > size) {
            length = size;
        }
        strm << margin << "Language: " << lang << std::endl
             << margin << "Event name: \"" << Printable(data, length) << "\"" << std::endl;
        data += length; size -= length;
        if (size < 1) {
            length = 0;
        }
        else {
            length = *data;
            data += 1; size -= 1;
            if (length > size) {
                length = size;
            }
        }
        strm << margin << "Description: \"" << Printable(data, length) << "\"" << std::endl;
        data += length; size -= length;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::ShortEventDescriptor::toXML(XML& xml, XML::Document& doc) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ShortEventDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
