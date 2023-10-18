//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPartialReceptionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"partial_reception_descriptor"
#define MY_CLASS ts::PartialReceptionDescriptor
#define MY_DID ts::DID_ISDB_PARTIAL_RECP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PartialReceptionDescriptor::PartialReceptionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::PartialReceptionDescriptor::PartialReceptionDescriptor(DuckContext& duck, const Descriptor& desc) :
    PartialReceptionDescriptor()
{
    deserialize(duck, desc);
}

void ts::PartialReceptionDescriptor::clearContent()
{
    service_ids.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PartialReceptionDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto it : service_ids) {
        buf.putUInt16(it);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PartialReceptionDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        service_ids.push_back(buf.getUInt16());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PartialReceptionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PartialReceptionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it : service_ids) {
        root->addElement(u"service")->setIntAttribute(u"id", it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PartialReceptionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xserv;
    bool ok = element->getChildren(xserv, u"service", 0, 127);

    for (auto it = xserv.begin(); ok && it != xserv.end(); ++it) {
        uint16_t id = 0;
        ok = (*it)->getIntAttribute(id, u"id", true);
        service_ids.push_back(id);
    }
    return ok;
}
