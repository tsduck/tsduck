//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"service_availability_descriptor"
#define MY_CLASS ts::ServiceAvailabilityDescriptor
#define MY_DID ts::DID_SERVICE_AVAIL
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceAvailabilityDescriptor::ServiceAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ServiceAvailabilityDescriptor::clearContent()
{
    availability = false;
    cell_ids.clear();
}

ts::ServiceAvailabilityDescriptor::ServiceAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceAvailabilityDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(availability);
    buf.putBits(0xFF, 7);
    for (auto it : cell_ids) {
        buf.putUInt16(it);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::deserializePayload(PSIBuffer& buf)
{
    availability = buf.getBool();
    buf.skipBits(7);
    while (buf.canRead()) {
        cell_ids.push_back(buf.getUInt16());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Availability: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(7);
        while (buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"Cell id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"availability", availability);
    for (auto it : cell_ids) {
        root->addElement(u"cell")->setIntAttribute(u"id", it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceAvailabilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getBoolAttribute(availability, u"availability", true) &&
        element->getChildren(children, u"cell", 0, MAX_CELLS);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint16_t id = 0;
        ok = children[i]->getIntAttribute(id, u"id", true);
        cell_ids.push_back(id);
    }
    return ok;
}
