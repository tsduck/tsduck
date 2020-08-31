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
#include "tsPSIBuffer.h"
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
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        buf.putLanguageCode(it->language);
        buf.putStringWithByteLength(it->name);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (!buf.error() && !buf.endOfRead()) {
        Entry e;
        buf.getLanguageCode(e.language);
        buf.getStringWithByteLength(e.name);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (!buf.error() && buf.remainingReadBytes() >= 4) {
        disp << margin << "Language: " << buf.getLanguageCode();
        disp << ", name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
    disp.displayExtraData(buf, margin);
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
