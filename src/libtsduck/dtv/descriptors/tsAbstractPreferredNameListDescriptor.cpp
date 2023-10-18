//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractPreferredNameListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractPreferredNameListDescriptor::AbstractPreferredNameListDescriptor(DID tag,
                                                                             const UChar* xml_name,
                                                                             Standards standards,
                                                                             PDS pds,
                                                                             const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
}

void ts::AbstractPreferredNameListDescriptor::clearContent()
{
    entries.clear();
}

ts::AbstractPreferredNameListDescriptor::AbstractPreferredNameListDescriptor(DuckContext& duck,
                                                                             const Descriptor& desc,
                                                                             DID tag,
                                                                             const UChar* xml_name,
                                                                             Standards standards,
                                                                             PDS pds,
                                                                             const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : entries) {
        buf.putLanguageCode(it1.first); // language
        buf.putUInt8(uint8_t(it1.second.size())); // name_count
        for (const auto& it2 : it1.second) {
            buf.putUInt8(it2.first);  // name_id
            buf.putStringWithByteLength(it2.second);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        // Force the creation of a language entry.
        NameByIdMap& names(entries[buf.getLanguageCode()]);

        // Get all names for the language.
        uint8_t count = buf.getUInt8();
        while (count-- > 0 && !buf.error()) {
            const uint8_t id = buf.getUInt8();
            buf.getStringWithByteLength(names[id]);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        uint8_t count = buf.getUInt8();
        disp << ", name count: " << int(count) << std::endl;
        while (count-- > 0 && buf.canReadBytes(2)) {
            disp << margin << "Id: " << int(buf.getUInt8());
            disp << ", Name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : entries) {
        xml::Element* e1 = root->addElement(u"language");
        e1->setAttribute(u"code", it1.first);
        for (const auto& it2 : it1.second) {
            xml::Element* e2 = e1->addElement(u"name");
            e2->setIntAttribute(u"name_id", it2.first, true);
            e2->setAttribute(u"name", it2.second);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractPreferredNameListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children1;
    bool ok = element->getChildren(children1, u"language");

    for (size_t i1 = 0; ok && i1 < children1.size(); ++i1) {
        xml::ElementVector children2;
        UString lang;
        ok = children1[i1]->getAttribute(lang, u"code", true, u"", 3, 3) && children1[i1]->getChildren(children2, u"name");
        if (ok) {
            // Force the creation of a language entry.
            NameByIdMap& names(entries[lang]);
            for (size_t i2 = 0; ok && i2 < children2.size(); ++i2) {
                uint8_t id = 0;
                ok = children2[i2]->getIntAttribute(id, u"name_id", true) && children2[i2]->getAttribute(names[id], u"name");
            }
        }
    }
    return ok;
}
