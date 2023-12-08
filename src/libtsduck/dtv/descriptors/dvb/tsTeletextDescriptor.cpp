//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTeletextDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TeletextDescriptor::TeletextDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds) :
    AbstractDescriptor(tag, xml_name, standards, pds)
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

void ts::TeletextDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(5)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        const uint8_t type = buf.getBits<uint8_t>(5);
        disp << UString::Format(u", Type: %d (0x%<X)", {type}) << std::endl;
        disp << margin << "Type: " << DataName(MY_XML_NAME, u"teletext_type", type) << std::endl;
        const uint8_t mag = buf.getBits<uint8_t>(3);
        const uint8_t page = buf.getUInt8();
        Entry e;
        e.setFullNumber(mag, page);
        disp << margin << "Magazine: " << int(mag) << ", page: " << int(page) << ", full page: " << e.page_number << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language_code);
        buf.putBits(it.teletext_type, 5);
        buf.putBits(it.magazineNumber(), 3);
        buf.putUInt8(it.pageNumber());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry entry;
        buf.getLanguageCode(entry.language_code);
        buf.getBits(entry.teletext_type, 5);
        const uint8_t mag = buf.getBits<uint8_t>(3);
        const uint8_t page = buf.getUInt8();
        entry.setFullNumber(mag, page);
        entries.push_back(entry);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"teletext");
        e->setAttribute(u"language_code", it.language_code);
        e->setIntAttribute(u"teletext_type", it.teletext_type, true);
        e->setIntAttribute(u"page_number", it.page_number);
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
             children[i]->getIntAttribute(entry.teletext_type, u"teletext_type", true) &&
             children[i]->getIntAttribute(entry.page_number, u"page_number", true);
        entries.push_back(entry);
    }
    return ok;
}
