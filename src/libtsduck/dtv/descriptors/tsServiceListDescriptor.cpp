//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"service_list_descriptor"
#define MY_CLASS ts::ServiceListDescriptor
#define MY_DID ts::DID_SERVICE_LIST
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceListDescriptor::ServiceListDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ServiceListDescriptor::ServiceListDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceListDescriptor()
{
    deserialize(duck, desc);
}

void ts::ServiceListDescriptor::clearContent()
{
    entries.clear();
}

ts::ServiceListDescriptor::Entry::Entry(uint16_t id, uint8_t type) :
    service_id(id),
    service_type(type)
{
}


//----------------------------------------------------------------------------
// Check if a service is present in the descriptor.
//----------------------------------------------------------------------------

bool ts::ServiceListDescriptor::hasService(uint16_t id) const
{
    for (const auto& it : entries) {
        if (it.service_id == id) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Add or replace a service.
//----------------------------------------------------------------------------

bool ts::ServiceListDescriptor::addService(uint16_t id, uint8_t type)
{
    for (auto& it : entries) {
        if (it.service_id == id) {
            // The service alreay exists, only overwrite the service type.
            if (it.service_type == type) {
                return false; // descriptor not modified
            }
            else {
                it.service_type = type;
                return true; // descriptor modified
            }
        }
    }

    // The service is not found, it an entry.
    entries.push_back(Entry(id, type));
    return true; // descriptor modified
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceListDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.service_id);
        buf.putUInt8(it.service_type);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceListDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        const uint16_t id = buf.getUInt16();
        const uint8_t type = buf.getUInt8();
        entries.push_back(Entry(id, type));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceListDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Service id: %d (0x%<X)", {buf.getUInt16()});
        disp << ", Type: " << names::ServiceType(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"service_type", it.service_type, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute(entry.service_type, u"service_type", true, 0, 0x00, 0xFF);
        entries.push_back(entry);
    }
    return ok;
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::ServiceListDescriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::ServiceListDescriptor::merge(const AbstractDescriptor& desc)
{
    const ServiceListDescriptor* other = dynamic_cast<const ServiceListDescriptor*>(&desc);
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
