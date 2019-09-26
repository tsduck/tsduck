//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"ATSC_EIT"
#define MY_TID ts::TID_ATSC_EIT
#define MY_STD ts::STD_ATSC

TS_XML_TABLE_FACTORY(ts::ATSCEIT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::ATSCEIT, MY_TID, MY_STD);
TS_FACTORY_REGISTER(ts::ATSCEIT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCEIT::ATSCEIT(uint8_t version_, uint16_t source_id_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // ATSC EIT is always "current"
    source_id(source_id_),
    protocol_version(0),
    events(this)
{
    _is_valid = true;
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
            const size_t title_length = data[9];
            data += 10; remain -= 10;

            if (!event.title_text.deserialize(duck, data, remain, title_length, true)) {
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
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCEIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    //@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::ATSCEIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,         // is_private_section
                                 source_id,    // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), // last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    remain += data - payload;
    data = payload;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display an ATSCEIT section.
//----------------------------------------------------------------------------

void ts::ATSCEIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.duck().out());
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

        strm << margin << UString::Format(u"Event Id: 0x%X (%d)", {evid, evid}) << std::endl
             << margin << "Start UTC: " << start.format(Time::DATETIME) << std::endl
             << margin << UString::Format(u"ETM location: %d", {loc}) << std::endl
             << margin << UString::Format(u"Duration: %d seconds", {length}) << std::endl;

        ATSCMultipleString::Display(display, u"Title text: ", indent, data, size, title_length);

        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;

        info_length = std::min(info_length, size);
        display.displayDescriptorList(section, data, info_length, indent);
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
    root->setIntAttribute(u"source_id", source_id);
    root->setIntAttribute(u"protocol_version", protocol_version, true);

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

void ts::ATSCEIT::fromXML(DuckContext& duck, const xml::Element* element)
{
    events.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint16_t>(source_id, u"source_id", true) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getChildren(children, u"event");

    // Get all events.
    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Event& event(events.newEntry());
        xml::ElementVector titles;
        _is_valid =
            children[i]->getIntAttribute<uint16_t>(event.event_id, u"event_id", true) &&
            children[i]->getDateTimeAttribute(event.start_time, u"start_time", true) &&
            children[i]->getIntAttribute<uint8_t>(event.ETM_location, u"ETM_location", true, 0, 0, 3) &&
            children[i]->getIntAttribute<Second>(event.length_in_seconds, u"length_in_seconds", true, 0, 0, 0x000FFFFF) &&
            event.descs.fromXML(duck, titles, children[i], u"title_text");
        if (_is_valid && !titles.empty()) {
            _is_valid = event.title_text.fromXML(duck, titles[0]);
        }
    }
}
