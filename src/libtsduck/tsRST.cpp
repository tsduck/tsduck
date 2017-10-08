//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Running Status Table (RST)
//
//----------------------------------------------------------------------------

#include "tsRST.h"
#include "tsFormat.h"
#include "tsTablesFactory.h"
#include "tsXMLTables.h"
TSDUCK_SOURCE;
TS_XML_TABLE_FACTORY(ts::RST, "RST");
TS_ID_TABLE_FACTORY(ts::RST, ts::TID_RST);
TS_ID_SECTION_DISPLAY(ts::RST::DisplaySection, ts::TID_RST);


//----------------------------------------------------------------------------
// Definition of names for running status values.
//----------------------------------------------------------------------------

const ts::Enumeration ts::RST::RunningStatusNames(
    "undefined", 0,
    "not-running", 1,
    "starting", 2,
    "pausing", 3,
    "running", 4,
    "off-air", 5,
    TS_NULL);


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::RST::RST() :
    AbstractTable(TID_RST, "RST"),
    events()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::RST::RST(const BinaryTable& table, const DVBCharset* charset) :
    AbstractTable(TID_RST, "RST"),
    events()
{
    deserialize(table, charset);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RST::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    events.clear();

    // This is a short table, must have only one section
    if (!table.isValid() || table.tableId() != _table_id || table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect(*table.sectionAt(0));
    const uint8_t* data = sect.payload();
    size_t remain = sect.payloadSize();

    // Analyze the section payload.
    while (remain >= 9) {
        Event event;
        event.transport_stream_id = GetUInt16(data);
        event.original_network_id = GetUInt16(data + 2);
        event.service_id = GetUInt16(data + 4);
        event.event_id = GetUInt16(data + 6);
        event.running_status = data[8] & 0x07;

        events.push_back(event);
        data += 9;
        remain -= 9;
    }

    _is_valid = remain == 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RST::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the section
    uint8_t payload[MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    for (EventList::const_iterator it = events.begin(); it != events.end() && remain >= 9; ++it) {
        PutUInt16(data, it->transport_stream_id);
        PutUInt16(data + 2, it->original_network_id);
        PutUInt16(data + 4, it->service_id);
        PutUInt16(data + 6, it->event_id);
        PutUInt8(data + 8, it->running_status | 0xF8);
        data += 9;
        remain -= 9;
    }

    // Add the section in the table.
    table.addSection(new Section(TID_RST,
                                 true,   // is_private_section
                                 payload,
                                 data - payload));
}


//----------------------------------------------------------------------------
// A static method to display a RST section.
//----------------------------------------------------------------------------

void ts::RST::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    while (size >= 9) {
        const uint16_t transport_stream_id = GetUInt16(data);
        const uint16_t original_network_id = GetUInt16(data + 2);
        const uint16_t service_id = GetUInt16(data + 4);
        const uint16_t event_id = GetUInt16(data + 6);
        const uint8_t running_status = data[8] & 0x07;
        data += 9;
        size -= 9;

        strm << margin
             << Format("TS: %d (0x%04X), Orig. Netw.: %d (0x%04X), Service: %d (0x%04X), Event: %d (0x%04X), Status: ",
                       int(transport_stream_id), int(transport_stream_id),
                       int(original_network_id), int(original_network_id),
                       int(service_id), int(service_id),
                       int(event_id), int(event_id))
             << RunningStatusNames.name(running_status)
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::RST::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    for (EventList::const_iterator it = events.begin(); it != events.end(); ++it) {
        XML::Element* e = xml.addElement(root, "event");
        xml.setIntAttribute(e, "transport_stream_id", it->transport_stream_id, true);
        xml.setIntAttribute(e, "original_network_id", it->original_network_id, true);
        xml.setIntAttribute(e, "service_id", it->service_id, true);
        xml.setIntAttribute(e, "event_id", it->event_id, true);
        xml.setEnumAttribute(RunningStatusNames, e, "running_status", it->running_status);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::RST::fromXML(XML& xml, const XML::Element* element)
{
    events.clear();

    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getChildren(children, element, "event");

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        Event event;
        _is_valid =
            xml.getIntAttribute<uint16_t>(event.transport_stream_id, children[index], "transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
            xml.getIntAttribute<uint16_t>(event.original_network_id, children[index], "original_network_id", true, 0, 0x0000, 0xFFFF) &&
            xml.getIntAttribute<uint16_t>(event.service_id, children[index], "service_id", true, 0, 0x0000, 0xFFFF) &&
            xml.getIntAttribute<uint16_t>(event.event_id, children[index], "event_id", true, 0, 0x0000, 0xFFFF) &&
            xml.getIntEnumAttribute<uint8_t>(event.running_status, RunningStatusNames, children[index], "running_status", true);
        if (_is_valid) {
            events.push_back(event);
        }
    }
}
