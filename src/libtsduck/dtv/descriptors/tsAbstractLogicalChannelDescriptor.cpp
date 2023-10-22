//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractLogicalChannelDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractLogicalChannelDescriptor::AbstractLogicalChannelDescriptor(DID tag,
                                                                       const UChar* xml_name,
                                                                       Standards standards,
                                                                       PDS pds,
                                                                       const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
}

void ts::AbstractLogicalChannelDescriptor::clearContent()
{
    entries.clear();
}

ts::AbstractLogicalChannelDescriptor::AbstractLogicalChannelDescriptor(DuckContext& duck,
                                                                       const Descriptor& desc,
                                                                       DID tag,
                                                                       const UChar* xml_name,
                                                                       Standards standards,
                                                                       PDS pds,
                                                                       const UChar* xml_legacy_name) :
    AbstractLogicalChannelDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.service_id);
        buf.putBit(it.visible);
        buf.putBits(0xFF, 5);
        buf.putBits(it.lcn, 10);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.service_id = buf.getUInt16();
        e.visible = buf.getBool();
        buf.skipBits(5);
        buf.getBits(e.lcn, 10);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Service Id: %5d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Visible: %1d", {buf.getBit()});
        buf.skipBits(5);
        disp << UString::Format(u", Channel number: %3d", {buf.getBits<uint16_t>(10)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"logical_channel_number", it.lcn, false);
        e->setBoolAttribute(u"visible_service", it.visible);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractLogicalChannelDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0x03FF) &&
             children[i]->getBoolAttribute(entry.visible, u"visible_service", false, true);
        if (ok) {
            entries.push_back(entry);
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::AbstractLogicalChannelDescriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::AbstractLogicalChannelDescriptor::merge(const AbstractDescriptor& desc)
{
    const AbstractLogicalChannelDescriptor* other = dynamic_cast<const AbstractLogicalChannelDescriptor*>(&desc);
    if (other == nullptr) {
        return false;
    }
    else {
        // Loop on all service entries in "other" descriptor.
        for (auto oth = other->entries.begin(); oth != other->entries.end(); ++oth) {
            // Replace entry with same service id in "this" descriptor.
            bool found = false;
            for (auto th = entries.begin(); !found && th != entries.end(); ++th) {
                found = th->service_id == oth->service_id;
                if (found) {
                    *th = *oth;
                }
            }
            // Add service ids which were not found at end of the list.
            if (!found) {
                entries.push_back(*oth);
            }
        }
        // If the result is too large, truncate it.
        bool success = entries.size() <= MAX_ENTRIES;
        while (entries.size() > MAX_ENTRIES) {
            entries.pop_back();
        }
        return success;
    }
}
