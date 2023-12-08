//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSIPrimeTSDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"SI_prime_TS_descriptor"
#define MY_CLASS ts::SIPrimeTSDescriptor
#define MY_DID ts::DID_ISDB_SI_PRIME_TS
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SIPrimeTSDescriptor::SIPrimeTSDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SIPrimeTSDescriptor::clearContent()
{
    parameter_version = 0;
    update_time.clear();
    SI_prime_TS_network_id = 0;
    SI_prime_transport_stream_id = 0;
    entries.clear();
}

ts::SIPrimeTSDescriptor::SIPrimeTSDescriptor(DuckContext& duck, const Descriptor& desc) :
    SIPrimeTSDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SIPrimeTSDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(parameter_version);
    buf.putMJD(update_time, 2);  // date only
    buf.putUInt16(SI_prime_TS_network_id);
    buf.putUInt16(SI_prime_transport_stream_id);
    for (const auto& it : entries) {
        buf.putUInt8(it.table_id);
        buf.putUInt8(uint8_t(it.table_description.size()));
        buf.putBytes(it.table_description);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SIPrimeTSDescriptor::deserializePayload(PSIBuffer& buf)
{
    parameter_version = buf.getUInt8();
    update_time = buf.getMJD(2);  // date only
    SI_prime_TS_network_id = buf.getUInt16();
    SI_prime_transport_stream_id = buf.getUInt16();
    while (buf.canRead()) {
        Entry e;
        e.table_id = buf.getUInt8();
        const size_t len = buf.getUInt8();
        buf.getBytes(e.table_description, len);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SIPrimeTSDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"Parameter version: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << "Update time: " << buf.getMJD(2).format(Time::DATE) << std::endl;
        disp << margin << UString::Format(u"SI prime TS network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"SI prime TS id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        while (buf.canReadBytes(2)) {
            disp << margin << "- Table id: " << names::TID(disp.duck(), buf.getUInt8(), CASID_NULL, NamesFlags::HEXA_FIRST) << std::endl;
            disp.displayPrivateData(u"Table description", buf, buf.getUInt8(), margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SIPrimeTSDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"parameter_version", parameter_version, true);
    root->setDateAttribute(u"update_time", update_time);
    root->setIntAttribute(u"SI_prime_TS_network_id", SI_prime_TS_network_id, true);
    root->setIntAttribute(u"SI_prime_transport_stream_id", SI_prime_transport_stream_id, true);
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"table");
        e->setIntAttribute(u"id", it.table_id, true);
        if (!it.table_description.empty()) {
            e->addHexaText(it.table_description);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SIPrimeTSDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtables;
    bool ok =
        element->getIntAttribute(parameter_version, u"parameter_version", true) &&
        element->getDateAttribute(update_time, u"update_time", true) &&
        element->getIntAttribute(SI_prime_TS_network_id, u"SI_prime_TS_network_id", true) &&
        element->getIntAttribute(SI_prime_transport_stream_id, u"SI_prime_transport_stream_id", true) &&
        element->getChildren(xtables, u"table");

    for (auto it = xtables.begin(); ok && it != xtables.end(); ++it) {
        Entry entry;
        ok = (*it)->getIntAttribute(entry.table_id, u"id", true) &&
             (*it)->getHexaText(entry.table_description, 0, 255);
        entries.push_back(entry);
    }
    return ok;
}
