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

#include "tsAbstractPreferredNameListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractPreferredNameListDescriptor::AbstractPreferredNameListDescriptor(DID tag,
                                                                             const UChar* xml_name,
                                                                             Standards standards,
                                                                             PDS pds,
                                                                             const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name),
    entries()
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
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name),
    entries()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (LanguageMap::const_iterator it1 = entries.begin(); it1 != entries.end(); ++it1) {
        if (!SerializeLanguageCode(*bbp, it1->first)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(uint8_t(it1->second.size()));  // name_count
        for (NameByIdMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            bbp->appendUInt8(it2->first);  // name_id
            bbp->append(duck.encodedWithByteLength(it2->second));
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag();
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();

        // Loop on languages.
        while (size >= 4) {
            // Get language and name count.
            const UString lang(DeserializeLanguageCode(data));
            uint8_t count = data[3];
            data += 4; size -= 4;

            // Force the creation of a language entry.
            NameByIdMap& names(entries[lang]);

            // Get all names for the lanuage.
            while (count-- > 0 && size >= 2) {
                uint8_t id = data[0];
                size_t length = data[1];
                data += 2; size -= 2;
                if (length > size) {
                    length = size;
                }
                duck.decode(names[id], data, length);
                data += length; size -= length;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const UString lang(DeserializeLanguageCode(data));
        uint8_t count = data[3];
        data += 4; size -= 4;

        strm << margin << "Language: " << lang << ", name count: " << int(count) << std::endl;
        while (count-- > 0 && size >= 2) {
            uint8_t id = data[0];
            size_t length = data[1];
            data += 2; size -= 2;
            if (length > size) {
                length = size;
            }
            strm << margin << "Id: " << int(id) << ", Name: \"" << duck.decoded(data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (LanguageMap::const_iterator it1 = entries.begin(); it1 != entries.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"language");
        e1->setAttribute(u"code", it1->first);
        for (NameByIdMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"name");
            e2->setIntAttribute(u"name_id", it2->first, true);
            e2->setAttribute(u"name", it2->second);
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
            for (size_t i2 = 0; _is_valid && i2 < children2.size(); ++i2) {
                uint8_t id = 0;
                ok = children2[i2]->getIntAttribute(id, u"name_id", true) && children2[i2]->getAttribute(names[id], u"name");
            }
        }
    }
    return ok;
}
