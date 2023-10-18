//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"LIT"
#define MY_CLASS ts::LIT
#define MY_TID ts::TID_LIT
#define MY_PID ts::PID_LIT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::LIT::LIT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    events(this)
{
}

ts::LIT::LIT(const LIT& other) :
    AbstractLongTable(other),
    event_id(other.event_id),
    service_id(other.service_id),
    transport_stream_id(other.transport_stream_id),
    original_network_id(other.original_network_id),
    events(this, other.events)
{
}

ts::LIT::LIT(DuckContext& duck, const BinaryTable& table) :
    LIT()
{
    deserialize(duck, table);
}

ts::LIT::Event::Event(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::LIT::tableIdExtension() const
{
    return event_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::LIT::clearContent()
{
    event_id = 0;
    service_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    events.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    event_id = section.tableIdExtension();
    service_id = buf.getUInt16();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();

    // Loop across all local events.
    while (buf.canRead()) {
        Event& ev(events.newEntry());
        ev.local_event_id = buf.getUInt16();
        buf.getDescriptorListWithLength(ev.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(service_id);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.pushState();

    // Minimum payload size, before loop of local events.
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all local events.
    for (const auto& it : events) {
        const Event& ev(it.second);

        // Binary size of this entry.
        const size_t entry_size = 4 + ev.descs.binarySize();

        // If we are not at the beginning of the event loop, make sure that the entire
        // event description fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
        }

        // Serialize local event. If descriptor loop is too long, it is truncated.
        buf.putUInt16(ev.local_event_id);
        buf.putPartialDescriptorListWithLength(ev.descs);
    }
}


//----------------------------------------------------------------------------
// A static method to display a LIT section.
//----------------------------------------------------------------------------

void ts::LIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Event id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        while (buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"- Local event id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"event_id", event_id, true);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);

    for (const auto& it : events) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"local_event_id", it.second.local_event_id, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xevent;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(event_id, u"event_id", true) &&
        element->getIntAttribute(service_id, u"service_id", true) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getChildren(xevent, u"event");

    for (auto it = xevent.begin(); ok && it != xevent.end(); ++it) {
        Event& ev(events.newEntry());
        xml::ElementVector xschedule;
        ok = (*it)->getIntAttribute(ev.local_event_id, u"local_event_id", true) &&
             ev.descs.fromXML(duck, *it);
    }
    return ok;
}
