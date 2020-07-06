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

#include "tsTeletextDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"teletext_descriptor"
#define MY_CLASS ts::TeletextDescriptor
#define MY_DID ts::DID_TELETEXT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TeletextDescriptor::Entry::Entry(const UChar* code, uint8_t type, uint16_t page) :
    teletext_type(type),
    page_number(page),
    language_code(code)
{
}

ts::TeletextDescriptor::Entry::Entry(const UString& code, uint8_t type, uint16_t page) :
    teletext_type(type),
    page_number(page),
    language_code(code)
{
}

ts::TeletextDescriptor::TeletextDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

ts::TeletextDescriptor::TeletextDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds) :
    AbstractDescriptor(tag, xml_name, standards, pds),
    entries()
{
}

void ts::TeletextDescriptor::clearContent()
{
    entries.clear();
}

ts::TeletextDescriptor::TeletextDescriptor(DuckContext& duck, const Descriptor& desc) :
    TeletextDescriptor()
{
    deserialize(duck, desc);
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
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 5) {
        const uint8_t type = data[3] >> 3;
        const uint8_t mag = data[3] & 0x07;
        const uint8_t page = data[4];
        Entry e;
        e.setFullNumber(mag, page);
        strm << margin << UString::Format(u"Language: %s, Type: %d (0x%X)", {DeserializeLanguageCode(data), type, type}) << std::endl
             << margin << "Type: " << names::TeletextType(type) << std::endl
             << margin << "Magazine: " << int(mag) << ", page: " << int(page) << ", full page: " << e.page_number << std::endl;
        data += 5; size -= 5;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language_code)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(uint8_t(it->teletext_type << 3) | (it->magazineNumber() & 0x07));
        bbp->appendUInt8(it->pageNumber());
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    if (!(_is_valid = desc.isValid() && desc.tag() == tag())) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    while (size >= 5) {
        Entry entry;
        entry.language_code = DeserializeLanguageCode(data);
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

void ts::TeletextDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"teletext");
        e->setAttribute(u"language_code", it->language_code);
        e->setIntAttribute(u"teletext_type", it->teletext_type, true);
        e->setIntAttribute(u"page_number", it->page_number);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TeletextDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"teletext", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language_code, u"language_code", true, u"", 3, 3) &&
             children[i]->getIntAttribute<uint8_t>(entry.teletext_type, u"teletext_type", true) &&
             children[i]->getIntAttribute<uint16_t>(entry.page_number, u"page_number", true);
        entries.push_back(entry);
    }
    return ok;
}
