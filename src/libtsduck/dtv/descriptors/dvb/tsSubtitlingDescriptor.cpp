//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSubtitlingDescriptor.h"
#include "tsComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SubtitlingDescriptor::clearContent()
{
    entries.clear();
}

ts::SubtitlingDescriptor::SubtitlingDescriptor(DuckContext& duck, const Descriptor& desc) :
    SubtitlingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Get the name of the subtitling type.
//----------------------------------------------------------------------------

ts:: UString ts::SubtitlingDescriptor::Entry::subtitlingTypeName() const
{
    DuckContext duck; // only needed by component_descriptor when in Japan.
    return ComponentDescriptor::ComponentTypeName(duck, 3, 0, subtitling_type);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(8)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        const uint8_t type = buf.getUInt8();
        disp << UString::Format(u", Type: %d (0x%<X)", {type}) << std::endl;
        disp << margin << "Type: " << ComponentDescriptor::ComponentTypeName(disp.duck(), 3, 0, type) << std::endl;
        disp << margin << UString::Format(u"Composition page: %d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Ancillary page: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language_code);
        buf.putUInt8(it.subtitling_type);
        buf.putUInt16(it.composition_page_id);
        buf.putUInt16(it.ancillary_page_id);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry entry;
        buf.getLanguageCode(entry.language_code);
        entry.subtitling_type = buf.getUInt8();
        entry.composition_page_id = buf.getUInt16();
        entry.ancillary_page_id = buf.getUInt16();
        entries.push_back(entry);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SubtitlingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"subtitling");
        e->setAttribute(u"language_code", it.language_code);
        e->setIntAttribute(u"subtitling_type", it.subtitling_type, true);
        e->setIntAttribute(u"composition_page_id", it.composition_page_id, true);
        e->setIntAttribute(u"ancillary_page_id", it.ancillary_page_id, true);
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
             children[i]->getIntAttribute(entry.subtitling_type, u"subtitling_type", true) &&
             children[i]->getIntAttribute(entry.composition_page_id, u"composition_page_id", true) &&
             children[i]->getIntAttribute(entry.ancillary_page_id, u"ancillary_page_id", true);
        entries.push_back(entry);
    }
    return ok;
}
