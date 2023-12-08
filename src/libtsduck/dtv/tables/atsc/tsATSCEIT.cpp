//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCEIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_EIT"
#define MY_CLASS ts::ATSCEIT
#define MY_TID ts::TID_ATSC_EIT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCEIT::ATSCEIT(uint8_t version_, uint16_t source_id_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // ATSC EIT is always "current"
    source_id(source_id_),
    events(this)
{
}

ts::ATSCEIT::ATSCEIT(DuckContext& duck, const BinaryTable& table) :
    ATSCEIT()
{
    deserialize(duck, table);
}


ts::ATSCEIT::ATSCEIT(const ATSCEIT& other) :
    AbstractLongTable(other),
    source_id(other.source_id),
    protocol_version(other.protocol_version),
    events(this, other.events)
{
}

ts::ATSCEIT::Event::Event(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ATSCEIT::tableIdExtension() const
{
    return source_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ATSCEIT::clearContent()
{
    source_id = 0;
    protocol_version = 0;
    events.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    source_id = section.tableIdExtension();
    protocol_version = buf.getUInt8();
    size_t event_count = buf.getUInt8();

    // Get events description
    while (!buf.error() && event_count-- > 0) {
        Event& event(events.newEntry());
        buf.skipBits(2);
        buf.getBits(event.event_id, 14);
        event.start_time = Time::GPSSecondsToUTC(buf.getUInt32());
        buf.skipBits(2);
        buf.getBits(event.ETM_location, 2);
        buf.getBits(event.length_in_seconds, 20);
        buf.getMultipleStringWithLength(event.title_text);
        buf.getDescriptorListWithLength(event.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt8(protocol_version);

    // Save position before num_events_in_section. Will be updated at each event.
    uint8_t num_events_in_section = 0;
    buf.pushState();
    buf.putUInt8(num_events_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Loop on event definitions.
    for (auto it = events.begin(); !buf.error() && it != events.end(); ++it) {
        const Event& event(it->second);

        // Pre-serialize the title_text. Its max size is 255 bytes since its size must fit in a byte.
        ByteBlock title;
        event.title_text.serialize(buf.duck(), title, 255, true);

        // Binary size of the event definition.
        const size_t entry_size = 10 + title.size() + 2 + event.descs.binarySize();

        // If we are not at the beginning of the event loop, make sure that the entire
        // event fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of num_events_in_section in the new section.
            buf.putUInt8(num_events_in_section = 0);
        }

        // Serialize the event definition.
        buf.putBits(0xFF, 2);
        buf.putBits(event.event_id, 14);
        buf.putUInt32(uint32_t(event.start_time.toGPSSeconds()));
        buf.putBits(0xFF, 2);
        buf.putBits(event.ETM_location, 2);
        buf.putBits(event.length_in_seconds, 20);
        buf.putUInt8(uint8_t(title.size()));
        buf.putBytes(title);
        buf.putPartialDescriptorListWithLength(event.descs);

        // Now increment the field num_events_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++num_events_in_section);
        buf.popState();
        buf.swapState();
    }
}


//----------------------------------------------------------------------------
// A static method to display an ATSC EIT section.
//----------------------------------------------------------------------------

void ts::ATSCEIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Source Id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    size_t event_count = 0;

    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()});
        disp << UString::Format(u", number of events: %d", {event_count = buf.getUInt8()}) << std::endl;

        // Loop on all event definitions.
        while (buf.canReadBytes(8) && event_count-- > 0) {
            buf.skipBits(2);
            disp << margin << UString::Format(u"- Event Id: 0x%X (%<d)", {buf.getBits<uint16_t>(14)}) << std::endl;
            disp << margin << "  Start UTC: " << Time::GPSSecondsToUTC(buf.getUInt32()).format(Time::DATETIME) << std::endl;
            buf.skipBits(2);
            disp << margin << UString::Format(u"  ETM location: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
            disp << margin << UString::Format(u"  Duration: %d seconds", {buf.getBits<Second>(20)}) << std::endl;
            disp.displayATSCMultipleString(buf, 1, margin + u"  ", u"Title text: ");
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"source_id", source_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);

    for (const auto& it : events) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"event_id", it.second.event_id, true);
        e->setDateTimeAttribute(u"start_time", it.second.start_time);
        e->setIntAttribute(u"ETM_location", it.second.ETM_location, true);
        e->setIntAttribute(u"length_in_seconds", it.second.length_in_seconds, false);
        it.second.title_text.toXML(duck, e, u"title_text", true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCEIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xevent;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(source_id, u"source_id", true) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getChildren(xevent, u"event");

    // Get all events.
    for (size_t i = 0; ok && i < xevent.size(); ++i) {
        Event& event(events.newEntry());
        xml::ElementVector xtitle;
        ok = xevent[i]->getIntAttribute(event.event_id, u"event_id", true, 0, 0, 0x3FFF) &&
             xevent[i]->getDateTimeAttribute(event.start_time, u"start_time", true) &&
             xevent[i]->getIntAttribute(event.ETM_location, u"ETM_location", true, 0, 0, 3) &&
             xevent[i]->getIntAttribute<Second>(event.length_in_seconds, u"length_in_seconds", true, 0, 0, 0x000FFFFF) &&
             event.descs.fromXML(duck, xtitle, xevent[i], u"title_text") &&
             (xtitle.empty() || event.title_text.fromXML(duck, xtitle[0]));
    }
    return ok;
}
