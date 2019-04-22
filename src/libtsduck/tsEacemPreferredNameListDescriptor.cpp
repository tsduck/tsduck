//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Representation of an eacem_preferred_name_list_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#include "tsEacemPreferredNameListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"eacem_preferred_name_list_descriptor"
#define MY_DID ts::DID_PREF_NAME_LIST
#define MY_PDS ts::PDS_EACEM
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::EacemPreferredNameListDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::EacemPreferredNameListDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameListDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_ID_DESCRIPTOR_FACTORY(ts::EacemPreferredNameListDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));
TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameListDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::EacemPreferredNameListDescriptor::EacemPreferredNameListDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::EacemPreferredNameListDescriptor::EacemPreferredNameListDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameListDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (LanguageMap::const_iterator it1 = entries.begin(); it1 != entries.end(); ++it1) {
        if (!SerializeLanguageCode(duck, *bbp, it1->first)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(uint8_t(it1->second.size()));  // name_count
        for (NameByIdMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            bbp->appendUInt8(it2->first);  // name_id
            bbp->append(duck.toDVBWithByteLength(it2->second));
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameListDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();

        // Loop on languages.
        while (size >= 4) {
            // Get language and name count.
            const UString lang(UString::FromDVB(data, 3));
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
                names[id] = duck.fromDVB(data, length);
                data += length; size -= length;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EacemPreferredNameListDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const UString lang(UString::FromDVB(data, 3));
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
            strm << margin << "Id: " << int(id) << ", Name: \"" << display.duck().fromDVB(data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
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

void ts::EacemPreferredNameListDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children1;
    _is_valid = checkXMLName(element) && element->getChildren(children1, u"language");

    for (size_t i1 = 0; _is_valid && i1 < children1.size(); ++i1) {
        xml::ElementVector children2;
        UString lang;
        _is_valid = children1[i1]->getAttribute(lang, u"code", true, u"", 3, 3) && children1[i1]->getChildren(children2, u"name");
        if (_is_valid) {
            // Force the creation of a language entry.
            NameByIdMap& names(entries[lang]);
            for (size_t i2 = 0; _is_valid && i2 < children2.size(); ++i2) {
                uint8_t id = 0;
                _is_valid = children2[i2]->getIntAttribute(id, u"name_id", true) && children2[i2]->getAttribute(names[id], u"name");
            }
        }
    }
}
