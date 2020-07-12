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

#include "tsLIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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
    event_id(0),
    service_id(0),
    transport_stream_id(0),
    original_network_id(0),
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
    EntryWithDescriptors(table),
    local_event_id(0)
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

void ts::LIT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    events.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table or payload too short.
        if (sect.tableId() != _table_id || remain < 6) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        event_id = sect.tableIdExtension();
        service_id = GetUInt16(data);
        transport_stream_id = GetUInt16(data + 2);
        original_network_id = GetUInt16(data + 4);
        data += 6; remain -= 6;

        // Loop across all local events.
        while (remain >= 4) {

            // Get fixed part.
            Event& ev(events.newEntry());
            ev.local_event_id = GetUInt16(data);
            size_t len = GetUInt16(data + 2) & 0x0FFF;
            data += 4; remain -= 4;

            if (len > remain) {
                return;
            }

            // Get descriptor loop.
            ev.descs.add(data, len);
            data += len; remain -= len;
        }

        // Abort in case of extra data in section.
        if (remain != 0) {
            return;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;

    // Add fixed part (6 bytes). Will remain unmodified in all sections.
    PutUInt16(payload, service_id);
    PutUInt16(payload + 2, transport_stream_id);
    PutUInt16(payload + 4, original_network_id);

    uint8_t* data = payload + 6;
    size_t remain = sizeof(payload) - 6;

    // Add all local events.
    for (auto it = events.begin(); it != events.end(); ++it) {

        // If we cannot at least add the fixed part of an event description, open a new section
        if (remain < 4) {
            addSection(table, section_number, payload, data, remain);
        }

        // If we are not at the beginning of the event loop, make sure that the entire
        // event description fits in the section. If it does not fit, start a new section. Huge
        // descriptions may not fit into one section, even when starting at the beginning
        // of the description loop. In that case, the description will span two sections later.
        const Event& ev(it->second);
        const DescriptorList& dlist(ev.descs);
        const size_t event_length = 4 + dlist.binarySize();
        if (data > payload + 6 && event_length > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
        }

        // Fill fixed part of the content version.
        assert(remain >= 4);
        PutUInt16(data, ev.local_event_id);
        data += 2; remain -= 2;

        // Serialize the descriptor loop.
        for (size_t start_index = 0; ; ) {

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this local event.
            addSection(table, section_number, payload, data, remain);
            PutUInt16(data, ev.local_event_id);
            data += 2; remain -= 2;
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::LIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 event_id,                // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers after fixed part of payload (6 bytes).
    remain += data - payload - 6;
    data = payload + 6;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display a LIT section.
//----------------------------------------------------------------------------

void ts::LIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Event id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 6) {

        // Display common information
        strm << margin << UString::Format(u"Service id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << UString::Format(u"Transport stream id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
             << margin << UString::Format(u"Original network id: 0x%X (%d)", {GetUInt16(data + 4), GetUInt16(data + 4)}) << std::endl;
        data += 6; size -= 6;

        // Loop across all local events.
        while (size >= 4) {
            strm << margin << UString::Format(u"- Local event id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
            size_t len = GetUInt16(data + 2) & 0x0FFF;
            data += 4; size -= 4;
            len = std::min(len, size);
            display.displayDescriptorList(section, data, len, indent + 2);
            data += len; size -= len;
        }
    }

    display.displayExtraData(data, size, indent);
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

    for (auto it = events.begin(); it != events.end(); ++it) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"local_event_id", it->second.local_event_id, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xevent;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(event_id, u"event_id", true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true) &&
        element->getIntAttribute<uint16_t>(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute<uint16_t>(original_network_id, u"original_network_id", true) &&
        element->getChildren(xevent, u"event");

    for (auto it = xevent.begin(); ok && it != xevent.end(); ++it) {
        Event& ev(events.newEntry());
        xml::ElementVector xschedule;
        ok = (*it)->getIntAttribute<uint16_t>(ev.local_event_id, u"local_event_id", true) &&
             ev.descs.fromXML(duck, *it);
    }
    return ok;
}
