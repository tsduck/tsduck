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

#include "tsMultilingualServiceNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"multilingual_service_name_descriptor"
#define MY_CLASS ts::MultilingualServiceNameDescriptor
#define MY_DID ts::DID_MLINGUAL_SERVICE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultilingualServiceNameDescriptor::MultilingualServiceNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

ts::MultilingualServiceNameDescriptor::MultilingualServiceNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultilingualServiceNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::MultilingualServiceNameDescriptor::clearContent()
{
    entries.clear();
}

ts::MultilingualServiceNameDescriptor::Entry::Entry(const UString& lang_, const UString& prov_, const UString& name_) :
    language(lang_),
    service_provider_name(prov_),
    service_name(name_)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language)) {
            desc.invalidate();
            return;
        }
        bbp->append(duck.encodedWithByteLength(it->service_provider_name));
        bbp->append(duck.encodedWithByteLength(it->service_name));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    entries.clear();

    while (_is_valid && size >= 4) {
        const UString lang(DeserializeLanguageCode(data));
        const size_t prov_len = data[3];
        data += 4; size -= 4;
        _is_valid = prov_len + 1 <= size;
        if (_is_valid) {
            const size_t name_len = data[prov_len];
            _is_valid = prov_len + 1 + name_len <= size;
            if (_is_valid) {
                entries.push_back(Entry(lang, duck.decoded(data, prov_len), duck.decoded(data + prov_len + 1, name_len)));
                data += prov_len + 1 + name_len;
                size -= prov_len + 1 + name_len;
            }
        }
    }
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const size_t prov_len = std::min<size_t>(data[3], size - 4);
        strm << margin
             << "Language: " << DeserializeLanguageCode(data)
             << ", provider: \"" << duck.decoded(data + 4, prov_len) << "\"";
        data += 4 + prov_len; size -= 4 + prov_len;
        if (size >= 1) {
            const size_t name_len = std::min<size_t>(data[0], size - 1);
            strm << ", service: \"" << duck.decoded(data + 1, name_len) << "\"";
            data += 1 + name_len; size -= 1 + name_len;
        }
        strm << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MultilingualServiceNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"language");
        e->setAttribute(u"code", it->language);
        e->setAttribute(u"service_provider_name", it->service_provider_name);
        e->setAttribute(u"service_name", it->service_name);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MultilingualServiceNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"language");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language, u"code", true, u"", 3, 3) &&
             children[i]->getAttribute(entry.service_provider_name, u"service_provider_name", true) &&
             children[i]->getAttribute(entry.service_name, u"service_name", true);
        entries.push_back(entry);
    }
    return ok;
}
