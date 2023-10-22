//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISO639LanguageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISO_639_language_descriptor"
#define MY_CLASS ts::ISO639LanguageDescriptor
#define MY_DID ts::DID_LANGUAGE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISO639LanguageDescriptor::Entry::Entry(const UChar* code, uint8_t type) :
    language_code(code),
    audio_type(type)
{
}

ts::ISO639LanguageDescriptor::Entry::Entry(const UString& code, uint8_t type) :
    language_code(code),
    audio_type(type)
{
}

ts::ISO639LanguageDescriptor::ISO639LanguageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISO639LanguageDescriptor::ISO639LanguageDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISO639LanguageDescriptor()
{
    deserialize(duck, desc);
}

ts::ISO639LanguageDescriptor::ISO639LanguageDescriptor(const UString& code, uint8_t type) :
    ISO639LanguageDescriptor()
{
    entries.push_back(Entry(code, type));
}

void ts::ISO639LanguageDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Get a string representing the audio type.
//----------------------------------------------------------------------------

ts::UString ts::ISO639LanguageDescriptor::Entry::audioTypeName(NamesFlags flags) const
{
    return DataName(MY_XML_NAME, u"audio_type", audio_type, flags);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISO639LanguageDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language_code);
        buf.putUInt8(it.audio_type);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISO639LanguageDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        const UString lang(buf.getLanguageCode());
        entries.push_back(Entry(lang, buf.getUInt8()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISO639LanguageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        disp << ", Type: " << DataName(MY_XML_NAME, u"audio_type", buf.getUInt8(), NamesFlags::FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISO639LanguageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"language");
        e->setAttribute(u"code", it.language_code);
        e->setIntAttribute(u"audio_type", it.audio_type, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISO639LanguageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"language", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language_code, u"code", true, u"", 3, 3) &&
             children[i]->getIntAttribute(entry.audio_type, u"audio_type", true, 0, 0x00, 0xFF);
        entries.push_back(entry);
    }
    return ok;
}
