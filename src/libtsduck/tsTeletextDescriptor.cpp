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
//  Representation of a teletext_descriptor
//
//----------------------------------------------------------------------------

#include "tsTeletextDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::TeletextDescriptor, "teletext_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::TeletextDescriptor, ts::EDID(ts::DID_TELETEXT));
TS_ID_DESCRIPTOR_DISPLAY(ts::TeletextDescriptor::DisplayDescriptor, ts::EDID(ts::DID_TELETEXT));


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TeletextDescriptor::Entry::Entry(uint8_t type, uint16_t page, const char* code) :
    teletext_type(type),
    page_number(page),
    language_code(code == 0 ? "" : code)
{
}

ts::TeletextDescriptor::TeletextDescriptor() :
    AbstractDescriptor(DID_TELETEXT, "teletext_descriptor"),
    entries()
{
    _is_valid = true;
}

ts::TeletextDescriptor::TeletextDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(DID_TELETEXT, "teletext_descriptor"),
    entries()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Convert between full Teletext page number and magazine / page numbers.
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::Entry::setFullNumber(uint8_t teletext_magazine_number, uint8_t teletext_page_number)
{
    page_number =
        100 * uint16_t(teletext_magazine_number == 0 ? 8 : teletext_magazine_number) +
        10 * uint16_t(teletext_page_number >> 4) +
        uint16_t(teletext_page_number & 0x0F);
}

uint8_t ts::TeletextDescriptor::Entry::pageNumber() const
{
    return uint8_t((((page_number / 10) % 10) << 4) | (page_number % 10));
}

uint8_t ts::TeletextDescriptor::Entry::magazineNumber() const
{
    return uint8_t((page_number / 100) % 8);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 5) {
        const uint8_t type = data[3] >> 3;
        const uint8_t mag = data[3] & 0x07;
        const uint8_t page = data[4];
        Entry e;
        e.setFullNumber(mag, page);
        strm << margin << "Language: " << Printable(data, 3)
             << ", Type: " << int(type)
             << Format(" (0x%02X)", int(type)) << std::endl
             << margin << "Type: " << names::TeletextType(type) << std::endl
             << margin << "Magazine: " << int(mag)
             << ", page: " << int(page)
             << ", full page: " << e.page_number
             << std::endl;
        data += 5; size -= 5;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    if (entries.size() > MAX_ENTRIES) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (it->language_code.length() != 3) {
            desc.invalidate();
            return;
        }
        bbp->append(it->language_code);
        bbp->appendUInt8((it->teletext_type << 3) | (it->magazineNumber() & 0x07));
        bbp->appendUInt8(it->pageNumber());
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    entries.clear();

    if (!(_is_valid = desc.isValid() && desc.tag() == _tag)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    while (size >= 5) {
        Entry entry;
        entry.language_code = std::string(reinterpret_cast<const char*>(data), 3);
        entry.teletext_type = data[3] >> 3;
        entry.setFullNumber(data[3] & 0x07, data[4]);
        entries.push_back(entry);
        data += 5; size -= 5;
    }

    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::TeletextDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        XML::Element* e = xml.addElement(root, "teletext");
        xml.setAttribute(e, "language_code", it->language_code);
        xml.setIntAttribute(e, "teletext_type", it->teletext_type, true);
        xml.setIntAttribute(e, "page_number", it->page_number);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    entries.clear();
    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getChildren(children, element, "teletext", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            xml.getAttribute(entry.language_code, children[i], "language_code", true, "", 3, 3) &&
            xml.getIntAttribute<uint8_t>(entry.teletext_type, children[i], "teletext_type", true) &&
            xml.getIntAttribute<uint16_t>(entry.page_number, children[i], "page_number", true);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
