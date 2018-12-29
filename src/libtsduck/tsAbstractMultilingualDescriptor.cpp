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

#include "tsAbstractMultilingualDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Protected constructor for subclasses.
//----------------------------------------------------------------------------

ts::AbstractMultilingualDescriptor::AbstractMultilingualDescriptor(DID tag, const UChar* xml_name, const UChar* xml_attribute, PDS pds) :
    AbstractDescriptor(tag, xml_name, pds),
    entries(),
    _xml_attribute(xml_attribute)
{
}


//----------------------------------------------------------------------------
// Contructor for language entry.
//----------------------------------------------------------------------------

ts::AbstractMultilingualDescriptor::Entry::Entry(const UString& lang_, const UString& name_) :
    language(lang_),
    name(name_)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());

    // Let the subclass serialize the prolog here.
    serializeProlog(bbp, charset);

    // Serialize the multillingual name loop.
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (!SerializeLanguageCode(*bbp, it->language, charset)) {
            desc.invalidate();
            return;
        }
        bbp->append(it->name.toDVBWithByteLength(0, NPOS, charset));
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    entries.clear();

    // Let the subclass deserialize the prolog here.
    deserializeProlog(data, size, charset);

    // Deserialize the multillingual name loop.
    while (_is_valid && size >= 4) {
        const UString lang(UString::FromDVB(data, 3, charset));
        const size_t len = data[3];
        data += 4; size -= 4;
        _is_valid = len <= size;
        if (_is_valid) {
            entries.push_back(Entry(lang, UString::FromDVB(data, len, charset)));
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
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const size_t len = std::min<size_t>(data[3], size - 4);
        strm << margin
             << "Language: " << UString::FromDVB(data, 3, display.dvbCharset())
             << ", name: \"" << UString::FromDVB(data + 4, len, display.dvbCharset()) << "\""
             << std::endl;
        data += 4 + len; size -= 4 + len;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::buildXML(xml::Element* root) const
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

void ts::AbstractMultilingualDescriptor::fromXML(const xml::Element* element)
{
    entries.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"language");

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            children[i]->getAttribute(entry.language, u"code", true, u"", 3, 3) &&
            children[i]->getAttribute(entry.name, _xml_attribute, true);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
