//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsATSCEIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    protocol_version(0),
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
    EntryWithDescriptors(table),
    event_id(0),
    start_time(),
    ETM_location(0),
    length_in_seconds(0),
    title_text()
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

void ts::ATSCEIT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    events.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Abort if not expected table
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent(); // should be true
        source_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        if (remain < 2) {
            return;
        }

        protocol_version = data[0];
        size_t event_count = data[1];
        data += 2; remain -= 2;

        // Get events description
        while (event_count > 0 && remain >= 10) {
            Event& event(events.newEntry());
            event.event_id = GetUInt16(data) & 0x3FFF;
            event.start_time = Time::GPSSecondsToUTC(GetUInt32(data + 2));
            event.ETM_location = (data[6] >> 4) & 0x03;
            event.length_in_seconds = GetUInt24(data + 6) & 0x000FFFFF;
            data += 9; remain -= 9;

            if (!event.title_text.lengthDeserialize(duck, data, remain)) {
                return;
            }

            if (remain < 2) {
                return;
            }

            size_t info_length = GetUInt16(data) & 0x0FFF;
            data += 2; remain -= 2;

            info_length = std::min(info_length, remain);
            event.descs.add(data, info_length);
            data += info_length; remain -= info_length;
            event_count--;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::ATSCEIT::addSection(BinaryTable& table, int& section_number, size_t& event_count, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    // Update fixed part and event count in this section.
    payload[0] = protocol_version;
    payload[1] = uint8_t(event_count);

    // Add a new section in the table.
    table.addSection(new Section(_table_id,
                                 true,         // is_private_section
                                 source_id,    // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), // last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize payload pointers after fixed part (start of the first event).
    remain += data - payload - 2;
    data = payload + 2;

    // Reset event count in payload, move to next section.
    event_count = 0;
    section_number++;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections one by one, starting at first event (offset 2).
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload + 2;
    size_t remain = sizeof(payload) - 2;

    // Count sections and events in sections (reset in addSection()).
    int section_number = 0;
    size_t event_count = 0;

    // Add all events.
    for (auto it = events.begin(); it != events.end(); ++it) {
        const Event& event(it->second);

        // Pre-serialize the title_text. Its max size is 255 bytes since its size must fit in a byte.
        ByteBlock title;
        event.title_text.serialize(duck, title, 255, true);

        // According to A/65, an event shall entirely fit into one section.
        // We try to serialize the current event and if it does not fit, close
        // the current section and open a new one. Of course, if one event is
        // so large that it cannot fit alone in a section, it will be truncated.
        const size_t descs_size = event.descs.binarySize();
        if (event_count > 0 && 10 + title.size() + 2 + descs_size > remain) {
            addSection(table, section_number, event_count, payload, data, remain);
        }

        // At this point, the free space is sufficient to store at least the fixed part and title string.
        // If this is the first event in the payload, it is still possible that the descriptor list does
        // not fit (will be truncated).
        assert(remain >= 10 + title.size() + 2);

        // Serialize fixed part and title.
        PutUInt16(data, 0xC000 | event.event_id);
        PutUInt32(data + 2, uint32_t(event.start_time.toGPSSeconds()));
        PutUInt24(data + 6, 0x00C00000 | (uint32_t(event.ETM_location & 0x03) << 20) | (event.length_in_seconds & 0x000FFFFF));
        PutUInt8(data + 9, uint8_t(title.size()));
        ::memcpy(data + 10, title.data(), title.size());
        data += 10 + title.size();
        remain -= 10 + title.size();

        // Serialize descriptors with 2-byte length prefix.
        event.descs.lengthSerialize(data, remain);
        event_count++;
    }

    // Add partial section (if there is one)
    if (data > payload + 2 || table.sectionCount() == 0) {
        addSection(table, section_number, event_count, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// A static method to display an ATSC EIT section.
//----------------------------------------------------------------------------

void ts::ATSCEIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    const uint16_t sid = section.tableIdExtension();
    size_t event_count = size < 2 ? 0 : data[1];

    strm << margin << UString::Format(u"Source Id: 0x%X (%d)", {sid, sid}) << std::endl;

    if (size >= 2) {
        strm << margin << UString::Format(u"Protocol version: %d, number of events: %d", {data[0], data[1]}) << std::endl;
        data += 2; size -= 2;
    }

    // Get events description
    while (event_count > 0 && size >= 10) {

        const uint16_t evid = GetUInt16(data) & 0x3FFF;
        const Time start(Time::GPSSecondsToUTC(GetUInt32(data + 2)));
        const uint8_t loc = (data[6] >> 4) & 0x03;
        const uint32_t length = GetUInt24(data + 6) & 0x000FFFFF;
        const size_t title_length = data[9];
        data += 10; size -= 10;

        strm << margin << UString::Format(u"- Event Id: 0x%X (%d)", {evid, evid}) << std::endl
             << margin << "  Start UTC: " << start.format(Time::DATETIME) << std::endl
             << margin << UString::Format(u"  ETM location: %d", {loc}) << std::endl
             << margin << UString::Format(u"  Duration: %d seconds", {length}) << std::endl;

        ATSCMultipleString::Display(display, u"Title text: ", indent + 2, data, size, title_length);

        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;

        info_length = std::min(info_length, size);
        display.displayDescriptorList(section, data, info_length, indent + 2);
        data += info_length; size -= info_length;
        event_count--;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"source_id", source_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);

    for (auto it = events.begin(); it != events.end(); ++it) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"event_id", it->second.event_id, true);
        e->setDateTimeAttribute(u"start_time", it->second.start_time);
        e->setIntAttribute(u"ETM_location", it->second.ETM_location, true);
        e->setIntAttribute(u"length_in_seconds", it->second.length_in_seconds, false);
        it->second.title_text.toXML(duck, e, u"title_text", true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCEIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint16_t>(source_id, u"source_id", true) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getChildren(children, u"event");

    // Get all events.
    for (size_t i = 0; ok && i < children.size(); ++i) {
        Event& event(events.newEntry());
        xml::ElementVector titles;
        ok = children[i]->getIntAttribute<uint16_t>(event.event_id, u"event_id", true, 0, 0, 0x3FFF) &&
             children[i]->getDateTimeAttribute(event.start_time, u"start_time", true) &&
             children[i]->getIntAttribute<uint8_t>(event.ETM_location, u"ETM_location", true, 0, 0, 3) &&
             children[i]->getIntAttribute<Second>(event.length_in_seconds, u"length_in_seconds", true, 0, 0, 0x000FFFFF) &&
             event.descs.fromXML(duck, titles, children[i], u"title_text");
        if (_is_valid && !titles.empty()) {
            _is_valid = event.title_text.fromXML(duck, titles[0]);
        }
    }
    return ok;
}
