//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractMultilingualDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Protected constructor for subclasses.
//----------------------------------------------------------------------------

ts::AbstractMultilingualDescriptor::AbstractMultilingualDescriptor(DID tag, const UChar* xml_name, const UChar* xml_attribute) :
    AbstractDescriptor(tag, xml_name, Standards::DVB, 0),
    _xml_attribute(xml_attribute)
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
    for (const auto& it : entries) {
        buf.putLanguageCode(it.language);
        buf.putStringWithByteLength(it.name);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
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
    while (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode();
        disp << ", name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractMultilingualDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"language");
        e->setAttribute(u"code", it.language);
        e->setAttribute(_xml_attribute, it.name);
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


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::AbstractMultilingualDescriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::AbstractMultilingualDescriptor::merge(const AbstractDescriptor& desc)
{
    const AbstractMultilingualDescriptor* other = dynamic_cast<const AbstractMultilingualDescriptor*>(&desc);
    if (other == nullptr) {
        return false;
    }
    else {
        // Loop on all service entries in "other" descriptor.
        for (auto oth = other->entries.begin(); oth != other->entries.end(); ++oth) {
            // Replace entry with same service id in "this" descriptor.
            bool found = false;
            for (auto th = entries.begin(); !found && th != entries.end(); ++th) {
                found = th->language == oth->language;
                if (found) {
                    *th = *oth;
                }
            }
            // Add service ids which were not found at end of the list.
            if (!found) {
                entries.push_back(*oth);
            }
        }
        return true;
    }
}
