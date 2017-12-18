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
//  Representation of an Event Information Table (EIT)
//
//----------------------------------------------------------------------------

#include "tsEIT.h"
#include "tsNames.h"
#include "tsRST.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"EIT"

TS_XML_TABLE_FACTORY(ts::EIT, MY_XML_NAME);
TS_ID_TABLE_RANGE_FACTORY(ts::EIT, ts::TID_EIT_MIN, ts::TID_EIT_MAX);
TS_ID_SECTION_RANGE_DISPLAY(ts::EIT::DisplaySection, ts::TID_EIT_MIN, ts::TID_EIT_MAX);


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::EIT::EIT(bool is_actual_,
             bool is_pf_,
             uint8_t eits_index_,
             uint8_t version_,
             bool is_current_,
             uint16_t service_id_,
             uint16_t ts_id_,
             uint16_t onetw_id_) :
    AbstractLongTable(ComputeTableId(is_actual_, is_pf_, eits_index_), MY_XML_NAME, version_, is_current_),
    service_id(service_id_),
    ts_id(ts_id_),
    onetw_id(onetw_id_),
    segment_last(0),
    last_table_id(_table_id),
    events()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::EIT::EIT(const BinaryTable& table, const DVBCharset* charset) :
    AbstractLongTable(TID_EIT_PF_ACT, MY_XML_NAME),  // TID will be updated by deserialize()
    service_id(0),
    ts_id(0),
    onetw_id(0),
    segment_last(0),
    last_table_id(0),
    events()
{
    deserialize(table, charset);
}


//----------------------------------------------------------------------------
// Compute an EIT table id.
//----------------------------------------------------------------------------

ts::TID ts::EIT::ComputeTableId(bool is_actual, bool is_pf, uint8_t eits_index)
{
    if (is_pf) {
        return is_actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
    }
    else {
        return (is_actual ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN) + (eits_index & 0x0F);
    }
}


//----------------------------------------------------------------------------
// Check if this is an "actual" EIT.
//----------------------------------------------------------------------------

bool ts::EIT::isActual() const
{
    return _table_id == TID_EIT_PF_ACT || (_table_id >= TID_EIT_S_ACT_MIN &&_table_id <= TID_EIT_S_ACT_MAX);
}


//----------------------------------------------------------------------------
// Set if this is an "actual" EIT.
//----------------------------------------------------------------------------

void ts::EIT::setActual(bool is_actual)
{
    if (isPresentFollowing()) {
        _table_id = is_actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        last_table_id = _table_id;
    }
    else if (is_actual) {
        _table_id = TID_EIT_S_ACT_MIN + (_table_id & 0x0F);
        last_table_id = TID_EIT_S_ACT_MIN + (last_table_id & 0x0F);
    }
    else {
        _table_id = TID_EIT_S_OTH_MIN + (_table_id & 0x0F);
        last_table_id = TID_EIT_S_ACT_MIN + (last_table_id & 0x0F);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EIT::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    service_id = 0;
    ts_id = 0;
    onetw_id = 0;
    segment_last = 0;
    last_table_id = _table_id;
    events.clear();

    if (!table.isValid()) {
        return;
    }

    // Check table id.
    if ((_table_id = table.tableId()) < TID_EIT_MIN || _table_id > TID_EIT_MAX) {
        return;
    }

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
        is_current = sect.isCurrent();
        service_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        if (remain < 6) {
            return;
        }

        ts_id = GetUInt16(data);
        onetw_id = GetUInt16(data + 2);
        segment_last = data[4];
        last_table_id = data[5];
        data += 6;
        remain -= 6;

        // Get services description
        while (remain >= 12) {
            uint16_t event_id = GetUInt16(data);
            Event& event(events[event_id]);
            DecodeMJD(data + 2, 5, event.start_time);
            const int hour = DecodeBCD(data[7]);
            const int min = DecodeBCD(data[8]);
            const int sec = DecodeBCD(data[9]);
            event.duration = (hour * 3600) + (min * 60) + sec;
            event.running_status = (data[10] >> 5) & 0x07;
            event.CA_controlled = (data[10] >> 4) & 0x01;

            size_t info_length = GetUInt16(data + 10) & 0x0FFF;
            data += 12;
            remain -= 12;

            info_length = std::min(info_length, remain);
            event.descs.add(data, info_length);
            data += info_length;
            remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EIT::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the sections
    uint8_t payload[MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // The first 6 bytes are identical in all section. Build them once.
    PutUInt16(data, ts_id);
    PutUInt16(data + 2, onetw_id);
    data[4] = segment_last;
    data[5] = last_table_id;
    data += 6;
    remain -= 6;

    // Add all events
    for (EventMap::const_iterator it = events.begin(); it != events.end(); ++it) {

        const uint16_t event_id = it->first;
        const Event& event(it->second);

        // If we cannot at least add the fixed part, open a new section
        if (remain < 12) {
            addSection(table, section_number, payload, data, remain);
        }

        // Insert the characteristics of the event. When the section is
        // not large enough to hold the entire descriptor list, open a
        // new section for the rest of the descriptors. In that case, the
        // common properties of the event must be repeated.
        bool starting = true;
        size_t start_index = 0;

        while (starting || start_index < event.descs.count()) {

            // If we are at the beginning of an event description, make sure that the
            // entire event description fits in the section. If it does not fit, start
            // a new section. Note that huge event descriptions may not fit into one
            // section. In that case, the event description will span two sections later.
            if (starting && 12 + event.descs.binarySize() > remain) {
                addSection(table, section_number, payload, data, remain);
            }

            starting = false;

            // Insert common characteristics of the service
            assert(remain >= 12);
            PutUInt16(data, event_id);
            EncodeMJD(event.start_time, data + 2, 5);
            data[7] = EncodeBCD(int(event.duration / 3600));
            data[8] = EncodeBCD(int((event.duration / 60) % 60));
            data[9] = EncodeBCD(int(event.duration % 60));
            data += 10;
            remain -= 10;

            // Insert descriptors (all or some).
            uint8_t* flags = data;
            start_index = event.descs.lengthSerialize(data, remain, start_index);

            // The following fields are inserted in the 4 "reserved" bits of the descriptor_loop_length.
            flags[0] = (flags[0] & 0x0F) | (event.running_status << 5) | (event.CA_controlled ? 0x10 : 0x00);

            // If not all descriptors were written, the section is full.
            // Open a new one and continue with this service.
            if (start_index < event.descs.count()) {
                addSection(table, section_number, payload, data, remain);
            }
        }
    }

    // Add partial section (if there is one)
    if (data > payload + 6 || table.sectionCount() == 0) {
        addSection(table, section_number, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Session number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::EIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,         // is_private_section
                                 service_id,   // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    // Restart after constant part of payload (6 bytes).
    remain += data - payload - 6;
    data = payload + 6;
    section_number++;
}


//----------------------------------------------------------------------------
// Event description constructor
//----------------------------------------------------------------------------

ts::EIT::Event::Event() :
    start_time(),
    duration(0),
    running_status(0),
    CA_controlled(false),
    descs()
{
}


//----------------------------------------------------------------------------
// A static method to display an EIT section.
//----------------------------------------------------------------------------

void ts::EIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    const uint16_t sid = section.tableIdExtension();

    strm << margin << UString::Format(u"Service Id: %d (0x%X)", {sid, sid}) << std::endl;

    if (size >= 6) {
        uint16_t tsid = GetUInt16(data);
        uint16_t onid = GetUInt16(data + 2);
        uint8_t seg_last = data[4];
        uint8_t last_tid = data[5];
        data += 6; size -= 6;

        strm << margin << UString::Format(u"TS Id: %d (0x%X)", {tsid, tsid}) << std::endl
             << margin << UString::Format(u"Original Network Id: %d (0x%X)", {onid, onid}) << std::endl
             << margin << UString::Format(u"Segment last section: %d (0x%X)", {seg_last, seg_last}) << std::endl
             << margin << UString::Format(u"Last Table Id: %d (0x%X), ", {last_tid, last_tid})
             << names::TID(last_tid) << std::endl;
    }

    while (size >= 12) {
        uint16_t evid = GetUInt16(data);
        Time start;
        DecodeMJD(data + 2, 5, start);
        int hour = DecodeBCD(data[7]);
        int min = DecodeBCD(data[8]);
        int sec = DecodeBCD(data[9]);
        uint8_t run = (data[10] >> 5) & 0x07;
        uint8_t ca_mode = (data[10] >> 4) & 0x01;
        size_t loop_length = GetUInt16(data + 10) & 0x0FFF;
        data += 12; size -= 12;
        if (loop_length > size) {
            loop_length = size;
        }
        strm << margin << UString::Format(u"Event Id: %d (0x%X)", {evid, evid}) << std::endl
             << margin << "Start UTC: " << start.format(Time::DATE | Time::TIME) << std::endl
             << margin << UString::Format(u"Duration: %02d:%02d:%02d", {hour, min, sec}) << std::endl
             << margin << "Running status: " << names::RunningStatus(run) << std::endl
             << margin << "CA mode: " << (ca_mode ? "controlled" : "free") << std::endl;
        display.displayDescriptorList(data, loop_length, indent, section.tableId());
        data += loop_length; size -= loop_length;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EIT::buildXML(xml::Element* root) const
{
    if (isPresentFollowing()) {
        root->setAttribute(u"type", u"pf");
    }
    else {
        root->setIntAttribute(u"type", _table_id - (isActual() ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN));
    }
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setBoolAttribute(u"actual", isActual());
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"transport_stream_id", ts_id, true);
    root->setIntAttribute(u"original_network_id", onetw_id, true);
    root->setIntAttribute(u"segment_last_section_number", segment_last, true);
    root->setIntAttribute(u"last_table_id", last_table_id, true);

    for (EventMap::const_iterator it = events.begin(); it != events.end(); ++it) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"event_id", it->first, true);
        e->setDateTimeAttribute(u"start_time", it->second.start_time);
        e->setTimeAttribute(u"duration", it->second.duration);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it->second.running_status);
        e->setBoolAttribute(u"CA_mode", it->second.CA_controlled);
        it->second.descs.toXML(e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::EIT::fromXML(const xml::Element* element)
{
    events.clear();
    UString type;
    bool actual = 0;

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(type, u"type", false, u"pf") &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(onetw_id, u"original_network_id", true, 0, 0x00, 0xFFFF) &&
        element->getIntAttribute<uint8_t>(segment_last, u"segment_last_section_number", true, 0, 0x00, 0xFF) &&
        element->getIntAttribute<TID>(last_table_id, u"last_table_id", true, 0, 0x00, 0xFF) &&
        element->getChildren(children, u"event");

    // Update table id.
    if (_is_valid) {
        if (type.similar(u"pf")) {
            // This is an EIT p/f
            _table_id = actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        }
        else if (type.toInteger(_table_id)) {
            // This is an EIT schedule
            _table_id += actual ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN;
        }
        else {
            element->report().error(u"'%s' is not a valid value for attribute 'type' in <%s>, line %d", {type, element->name(), element->lineNumber()});
            _is_valid = false;
        }
    }

    // Get all events.
    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Event event;
        uint16_t event_id = 0;
        _is_valid =
            children[i]->getIntAttribute<uint16_t>(event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getDateTimeAttribute(event.start_time, u"start_time", true) &&
            children[i]->getTimeAttribute(event.duration, u"duration", true) &&
            children[i]->getIntEnumAttribute<uint8_t>(event.running_status, RST::RunningStatusNames, u"running_status", false, 0) &&
            children[i]->getBoolAttribute(event.CA_controlled, u"CA_mode", false, false) &&
            event.descs.fromXML(children[i]);
        if (_is_valid) {
            events[event_id] = event;
        }
    }
}
