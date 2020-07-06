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

#include "tsSubtitlingDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"subtitling_descriptor"
#define MY_CLASS ts::SubtitlingDescriptor
#define MY_DID ts::DID_SUBTITLING
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::SubtitlingDescriptor::Entry::Entry(const UChar* code, uint8_t subt, uint16_t comp, uint16_t ancil) :
    language_code(code),
    subtitling_type(subt),
    composition_page_id(comp),
    ancillary_page_id(ancil)
{
}

ts::SubtitlingDescriptor::Entry::Entry(const UString& code, uint8_t subt, uint16_t comp, uint16_t ancil) :
    language_code(code),
    subtitling_type(subt),
    composition_page_id(comp),
    ancillary_page_id(ancil)
{
}

ts::SubtitlingDescriptor::SubtitlingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

void ts::SubtitlingDescriptor::clearContent()
{
    entries.clear();
}

ts::SubtitlingDescriptor::SubtitlingDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 8) {
        uint8_t type = data[3];
        uint16_t comp_page = GetUInt16(data + 4);
        uint16_t ancil_page = GetUInt16(data + 6);
        strm << margin << UString::Format(u"Language: %s, Type: %d (0x%X)", {DeserializeLanguageCode(data), type, type}) << std::endl
             << margin << "Type: " << names::SubtitlingType(type) << std::endl
             << margin << UString::Format(u"Composition page: %d (0x%X), Ancillary page: %d (0x%X)", {comp_page, comp_page, ancil_page, ancil_page}) << std::endl;
        data += 8; size -= 8;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language_code)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(it->subtitling_type);
        bbp->appendUInt16(it->composition_page_id);
        bbp->appendUInt16(it->ancillary_page_id);
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    if (!(_is_valid = desc.isValid() && desc.tag() == tag())) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    while (size >= 8) {
        Entry entry;
        entry.language_code = DeserializeLanguageCode(data);
        entry.subtitling_type = data[3];
        entry.composition_page_id = GetUInt16(data + 4);
        entry.ancillary_page_id = GetUInt16(data + 6);
        entries.push_back(entry);
        data += 8; size -= 8;
    }

    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"subtitling");
        e->setAttribute(u"language_code", it->language_code);
        e->setIntAttribute(u"subtitling_type", it->subtitling_type, true);
        e->setIntAttribute(u"composition_page_id", it->composition_page_id, true);
        e->setIntAttribute(u"ancillary_page_id", it->ancillary_page_id, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SubtitlingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"subtitling", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language_code, u"language_code", true, u"", 3, 3) &&
             children[i]->getIntAttribute<uint8_t>(entry.subtitling_type, u"subtitling_type", true) &&
             children[i]->getIntAttribute<uint16_t>(entry.composition_page_id, u"composition_page_id", true) &&
             children[i]->getIntAttribute<uint16_t>(entry.ancillary_page_id, u"ancillary_page_id", true);
        entries.push_back(entry);
    }
    return ok;
}
