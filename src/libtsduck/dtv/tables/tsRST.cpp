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

#include "tsRST.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"RST"
#define MY_CLASS ts::RST
#define MY_TID ts::TID_RST
#define MY_PID ts::PID_RST
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Definition of names for running status values.
//----------------------------------------------------------------------------

const ts::Enumeration ts::RST::RunningStatusNames({
    {u"undefined",   RS_UNDEFINED},
    {u"not-running", RS_NOT_RUNNING},
    {u"starting",    RS_STARTING},
    {u"pausing",     RS_PAUSING},
    {u"running",     RS_RUNNING},
    {u"off-air",     RS_OFF_AIR},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RST::RST() :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    events()
{
}

ts::RST::RST(DuckContext& duck, const BinaryTable& table) :
    RST()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::RST::clearContent()
{
    events.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RST::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    events.clear();

    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
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

void ts::RST::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
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
    table.addSection(new Section(MY_TID,
                                 true,   // is_private_section
                                 payload,
                                 data - payload));
}


//----------------------------------------------------------------------------
// A static method to display a RST section.
//----------------------------------------------------------------------------

void ts::RST::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
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
             << UString::Format(u"TS: %d (0x%X), Orig. Netw.: %d (0x%X), Service: %d (0x%X), Event: %d (0x%X), Status: %s",
                       {transport_stream_id, transport_stream_id,
                        original_network_id, original_network_id,
                        service_id, service_id,
                        event_id, event_id,
                        RunningStatusNames.name(running_status)})
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RST::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EventList::const_iterator it = events.begin(); it != events.end(); ++it) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"transport_stream_id", it->transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", it->original_network_id, true);
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"event_id", it->event_id, true);
        e->setEnumAttribute(RunningStatusNames, u"running_status", it->running_status);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RST::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"event");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        Event event;
        ok = children[index]->getIntAttribute<uint16_t>(event.transport_stream_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute<uint16_t>(event.original_network_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute<uint16_t>(event.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute<uint16_t>(event.event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntEnumAttribute<uint8_t>(event.running_status, RunningStatusNames, u"running_status", true);
        if (ok) {
            events.push_back(event);
        }
    }
    return ok;
}
