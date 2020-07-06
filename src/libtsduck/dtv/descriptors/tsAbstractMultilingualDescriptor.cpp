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

#include "tsAbstractMultilingualDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Protected constructor for subclasses.
//----------------------------------------------------------------------------

ts::AbstractMultilingualDescriptor::AbstractMultilingualDescriptor(DID tag, const UChar* xml_name, const UChar* xml_attribute) :
    AbstractDescriptor(tag, xml_name, Standards::DVB, 0),
    entries(),
    _xml_attribute(xml_attribute)
{
}

ts::AbstractMultilingualDescriptor::Entry::Entry(const UString& lang_, const UString& name_) :
    language(lang_),
    name(name_)
{
}

void ts::AbstractMultilingualDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Default implementation of prolog serialization.
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::serializeProlog(DuckContext& duck, const ByteBlockPtr& bbp) const
{
}

void ts::AbstractMultilingualDescriptor::deserializeProlog(DuckContext& duck, const uint8_t*& data, size_t& size)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    // Let the subclass serialize the prolog here.
    serializeProlog(duck, bbp);

    // Serialize the multi-lingual name loop.
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language)) {
            desc.invalidate();
            return;
        }
        bbp->append(duck.encodedWithByteLength(it->name));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    entries.clear();

    // Let the subclass deserialize the prolog here.
    deserializeProlog(duck, data, size);

    // Deserialize the multillingual name loop.
    while (_is_valid && size >= 4) {
        // Always default DVB character set for language code.
        const UString lang(DeserializeLanguageCode(data));
        const size_t len = data[3];
        data += 4; size -= 4;
        _is_valid = len <= size;
        if (_is_valid) {
            entries.push_back(Entry(lang, duck.decoded(data, len)));
            data += len; size -= len;
        }
    }
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const size_t len = std::min<size_t>(data[3], size - 4);
        strm << margin
             << "Language: " << DeserializeLanguageCode(data)
             << ", name: \"" << duck.decoded(data + 4, len) << "\""
             << std::endl;
        data += 4 + len; size -= 4 + len;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"language");
        e->setAttribute(u"code", it->language);
        e->setAttribute(_xml_attribute, it->name);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractMultilingualDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"language");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.language, u"code", true, u"", 3, 3) &&
             children[i]->getAttribute(entry.name, _xml_attribute, true);
        if (ok) {
            entries.push_back(entry);
        }
    }
    return ok;
}
