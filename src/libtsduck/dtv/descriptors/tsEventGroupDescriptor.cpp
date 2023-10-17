//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEventGroupDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"

#define MY_XML_NAME u"event_group_descriptor"
#define MY_CLASS ts::EventGroupDescriptor
#define MY_DID ts::DID_ISDB_EVENT_GROUP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EventGroupDescriptor::EventGroupDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::EventGroupDescriptor::EventGroupDescriptor(DuckContext& duck, const Descriptor& desc) :
    EventGroupDescriptor()
{
    deserialize(duck, desc);
}

void ts::EventGroupDescriptor::clearContent()
{
    group_type = 0;
    actual_events.clear();
    other_events.clear();
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(group_type, 4);
    buf.putBits(actual_events.size(), 4);
    for (const auto& it : actual_events) {
        buf.putUInt16(it.service_id);
        buf.putUInt16(it.event_id);
    }
    if (group_type == 4 || group_type == 5) {
        for (const auto& it : other_events) {
            buf.putUInt16(it.original_network_id);
            buf.putUInt16(it.transport_stream_id);
            buf.putUInt16(it.service_id);
            buf.putUInt16(it.event_id);
        }
    }
    else {
        buf.putBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(group_type, 4);
    const size_t event_count = buf.getBits<size_t>(4);
    for (size_t i = 0; i < event_count && buf.canRead(); ++i) {
        ActualEvent ev;
        ev.service_id = buf.getUInt16();
        ev.event_id = buf.getUInt16();
        actual_events.push_back(ev);
    }
    if (group_type == 4 || group_type == 5) {
        while (buf.canRead()) {
            OtherEvent ev;
            ev.original_network_id = buf.getUInt16();
            ev.transport_stream_id = buf.getUInt16();
            ev.service_id = buf.getUInt16();
            ev.event_id = buf.getUInt16();
            other_events.push_back(ev);
        }
    }
    else {
        buf.getBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t type = buf.getBits<uint8_t>(4);
        disp << margin << "Group type: " << DataName(MY_XML_NAME, u"Type", type, NamesFlags::DECIMAL_FIRST) << std::endl;
        size_t count = buf.getBits<size_t>(4);
        disp << margin << "Actual events:" << (count == 0 ? " none" : "") << std::endl;
        while (count-- > 0 && buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"- Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Event id:   0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        if (type == 4 || type == 5) {
            disp << margin << "Other networks events:" << (buf.canReadBytes(8) ? "" : " none") << std::endl;
            while (buf.canReadBytes(8)) {
                disp << margin << UString::Format(u"- Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Service id:          0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Event id:            0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Private data", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"group_type", group_type);
    for (const auto& it : actual_events) {
        xml::Element* e = root->addElement(u"actual");
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"event_id", it.event_id, true);
    }
    if (group_type == 4 || group_type == 5) {
        for (const auto& it : other_events) {
            xml::Element* e = root->addElement(u"other");
            e->setIntAttribute(u"original_network_id", it.original_network_id, true);
            e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
            e->setIntAttribute(u"service_id", it.service_id, true);
            e->setIntAttribute(u"event_id", it.event_id, true);
        }
    }
    else {
        root->addHexaTextChild(u"private_data", private_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EventGroupDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xactual;
    xml::ElementVector xother;
    bool ok =
        element->getIntAttribute(group_type, u"group_type", true, 0, 0, 15) &&
        element->getChildren(xactual, u"actual", 0, 15) &&
        element->getChildren(xother, u"other", 0, group_type == 4 || group_type == 5 ? 31 : 0) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, group_type == 4 || group_type == 5 ? 0 : 254);

    for (auto it = xactual.begin(); ok && it != xactual.end(); ++it) {
        ActualEvent ev;
        ok = (*it)->getIntAttribute(ev.service_id, u"service_id", true) &&
             (*it)->getIntAttribute(ev.event_id, u"event_id", true);
        actual_events.push_back(ev);
    }

    for (auto it = xother.begin(); ok && it != xother.end(); ++it) {
        OtherEvent ev;
        ok = (*it)->getIntAttribute(ev.original_network_id, u"original_network_id", true) &&
             (*it)->getIntAttribute(ev.transport_stream_id, u"transport_stream_id", true) &&
             (*it)->getIntAttribute(ev.service_id, u"service_id", true) &&
             (*it)->getIntAttribute(ev.event_id, u"event_id", true);
        other_events.push_back(ev);
    }
    return ok;
}
