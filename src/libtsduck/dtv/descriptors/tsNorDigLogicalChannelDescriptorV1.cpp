//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNorDigLogicalChannelDescriptorV1.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"nordig_logical_channel_descriptor_v1"
#define MY_CLASS ts::NorDigLogicalChannelDescriptorV1
#define MY_DID ts::DID_NORDIG_CHAN_NUM_V1
#define MY_PDS ts::PDS_NORDIG
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NorDigLogicalChannelDescriptorV1::NorDigLogicalChannelDescriptorV1() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::NorDigLogicalChannelDescriptorV1::NorDigLogicalChannelDescriptorV1(DuckContext& duck, const Descriptor& desc) :
    NorDigLogicalChannelDescriptorV1()
{
    deserialize(duck, desc);
}

ts::NorDigLogicalChannelDescriptorV1::Entry::Entry(uint16_t id, bool vis, uint16_t lc) :
    service_id(id),
    visible(vis),
    lcn(lc)
{
}

void ts::NorDigLogicalChannelDescriptorV1::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV1::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.service_id);
        buf.putBit(it.visible);
        buf.putBit(1);
        buf.putBits(it.lcn, 14);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV1::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.service_id = buf.getUInt16();
        e.visible = buf.getBool();
        buf.skipBits(1);
        buf.getBits(e.lcn, 14);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV1::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Service Id: %5d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Visible: %1d", {buf.getBool()});
        buf.skipBits(1);
        disp << UString::Format(u", Channel number: %3d", {buf.getBits<uint16_t>(14)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV1::buildXML(DuckContext& duck, xml::Element* root) const
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

bool ts::NorDigLogicalChannelDescriptorV1::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.service_id, u"service_id", true) &&
             children[i]->getIntAttribute(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0x3FFF) &&
             children[i]->getBoolAttribute(entry.visible, u"visible_service", false, true);
        entries.push_back(entry);
    }
    return ok;
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::NorDigLogicalChannelDescriptorV1::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::NorDigLogicalChannelDescriptorV1::merge(const AbstractDescriptor& desc)
{
    const NorDigLogicalChannelDescriptorV1* other = dynamic_cast<const NorDigLogicalChannelDescriptorV1*>(&desc);
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
