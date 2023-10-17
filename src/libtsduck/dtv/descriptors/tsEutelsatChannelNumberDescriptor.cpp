//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEutelsatChannelNumberDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"eutelsat_channel_number_descriptor"
#define MY_CLASS ts::EutelsatChannelNumberDescriptor
#define MY_DID ts::DID_EUTELSAT_CHAN_NUM
#define MY_PDS ts::PDS_EUTELSAT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EutelsatChannelNumberDescriptor::EutelsatChannelNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::EutelsatChannelNumberDescriptor::EutelsatChannelNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    EutelsatChannelNumberDescriptor()
{
    deserialize(duck, desc);
}

void ts::EutelsatChannelNumberDescriptor::clearContent()
{
    entries.clear();
}

ts::EutelsatChannelNumberDescriptor::Entry::Entry(uint16_t onetw_id_, uint16_t ts_id_, uint16_t service_id_, uint16_t ecn_) :
    onetw_id(onetw_id_),
    ts_id(ts_id_),
    service_id(service_id_),
    ecn(ecn_)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.onetw_id);
        buf.putUInt16(it.ts_id);
        buf.putUInt16(it.service_id);
        buf.putBits(0xFF, 4);
        buf.putBits(it.ecn, 12);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.onetw_id = buf.getUInt16();
        e.ts_id = buf.getUInt16();
        e.service_id = buf.getUInt16();
        buf.skipBits(4);
        buf.getBits(e.ecn, 12);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(8)) {
        const uint16_t onetw_id = buf.getUInt16();
        const uint16_t ts_id = buf.getUInt16();
        const uint16_t service_id = buf.getUInt16();
        buf.skipBits(4);
        const uint16_t channel = buf.getBits<uint16_t>(12);
        disp << margin
             << UString::Format(u"Service Id: %5d (0x%04<X), Channel number: %3d, TS Id: %5d (0x%<04X), Net Id: %5d (0x%<04X)", {service_id, channel, ts_id, onetw_id})
             << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"original_network_id", it.onetw_id, true);
        e->setIntAttribute(u"transport_stream_id", it.ts_id, true);
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"eutelsat_channel_number", it.ecn, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EutelsatChannelNumberDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok =
            children[i]->getIntAttribute(entry.onetw_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute(entry.ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute(entry.ecn, u"eutelsat_channel_number", true, 0, 0x0000, 0x03FF);
        entries.push_back(entry);
    }
    return ok;
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::EutelsatChannelNumberDescriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::EutelsatChannelNumberDescriptor::merge(const AbstractDescriptor& desc)
{
    const EutelsatChannelNumberDescriptor* other = dynamic_cast<const EutelsatChannelNumberDescriptor*>(&desc);
    if (other == nullptr) {
        return false;
    }
    else {
        // Loop on all service entries in "other" descriptor.
        for (auto oth = other->entries.begin(); oth != other->entries.end(); ++oth) {
            // Replace entry with same service id in "this" descriptor.
            bool found = false;
            for (auto th = entries.begin(); !found && th != entries.end(); ++th) {
                found = th->onetw_id == oth->onetw_id && th->ts_id == oth->ts_id && th->service_id == oth->service_id;
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
