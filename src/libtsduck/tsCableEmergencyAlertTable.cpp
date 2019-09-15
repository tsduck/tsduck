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

#include "tsCableEmergencyAlertTable.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"cable_emergency_alert_table"
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD (ts::STD_SCTE | ts::STD_ATSC)

TS_XML_TABLE_FACTORY(ts::CableEmergencyAlertTable, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::CableEmergencyAlertTable, MY_TID, MY_STD);
TS_FACTORY_REGISTER(ts::CableEmergencyAlertTable::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CableEmergencyAlertTable::CableEmergencyAlertTable(uint8_t sequence_number) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, sequence_number, true),
    protocol_version(0),
    EAS_event_ID(0),
    EAS_originator_code(),
    EAS_event_code(),
    nature_of_activation_text(),
    alert_message_time_remaining(0),
    event_start_time(),
    event_duration(0),
    alert_priority(0),
    details_OOB_source_ID(0),
    details_major_channel_number(0),
    details_minor_channel_number(0),
    audio_OOB_source_ID(0),
    alert_text(),
    locations(),
    exceptions(),
    descs(this)
{
    _is_valid = true;
}

ts::CableEmergencyAlertTable::CableEmergencyAlertTable(const CableEmergencyAlertTable& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    EAS_event_ID(other.EAS_event_ID),
    EAS_originator_code(other.EAS_originator_code),
    EAS_event_code(other.EAS_event_code),
    nature_of_activation_text(other.nature_of_activation_text),
    alert_message_time_remaining(other.alert_message_time_remaining),
    event_start_time(other.event_start_time),
    event_duration(other.event_duration),
    alert_priority(other.alert_priority),
    details_OOB_source_ID(other.details_OOB_source_ID),
    details_major_channel_number(other.details_major_channel_number),
    details_minor_channel_number(other.details_minor_channel_number),
    audio_OOB_source_ID(other.audio_OOB_source_ID),
    alert_text(other.alert_text),
    locations(other.locations),
    exceptions(other.exceptions),
    descs(this, other.descs)
{
}

ts::CableEmergencyAlertTable::CableEmergencyAlertTable(DuckContext& duck, const BinaryTable& table) :
    CableEmergencyAlertTable()
{
    deserialize(duck, table);
}

ts::CableEmergencyAlertTable::Location::Location(uint8_t state, uint8_t sub, uint16_t county) :
    state_code(state),
    county_subdivision(sub),
    county_code(county)
{
}

ts::CableEmergencyAlertTable::Exception::Exception(uint16_t oob) :
    in_band(false),
    exception_major_channel_number(0),
    exception_minor_channel_number(0),
    exception_OOB_source_ID(oob)
{
}

ts::CableEmergencyAlertTable::Exception::Exception(uint16_t major, uint16_t minor) :
    in_band(true),
    exception_major_channel_number(major),
    exception_minor_channel_number(minor),
    exception_OOB_source_ID(0)
{
}


//----------------------------------------------------------------------------
// Clear all fields.
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::clear()
{
    version = 0;
    protocol_version = 0;
    EAS_event_ID = 0;
    EAS_originator_code.clear();
    EAS_event_code.clear();
    nature_of_activation_text.clear();
    alert_message_time_remaining = 0;
    event_start_time = Time::Epoch;
    event_duration = 0;
    alert_priority = 0;
    details_OOB_source_ID = 0;
    details_major_channel_number = 0;
    details_minor_channel_number = 0;
    audio_OOB_source_ID = 0;
    alert_text.clear();
    locations.clear();
    exceptions.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    clear();

    // An EAS table may have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect(*table.sectionAt(0));
    const uint8_t* data = sect.payload();
    size_t remain = sect.payloadSize();

    // Fixed part.
    if (remain < 7) {
        return;
    }
    version = sect.version();
    protocol_version = data[0];
    EAS_event_ID = GetUInt16(data + 1);
    EAS_originator_code.assignFromUTF8(reinterpret_cast<const char*>(data + 3), 3);
    const size_t event_len = data[6];
    data += 7; remain -= 7;

    // Event code.
    if (remain < event_len + 1) {
        return;
    }
    EAS_event_code.assignFromUTF8(reinterpret_cast<const char*>(data), event_len);
    const size_t activ_len = data[event_len];
    data += event_len + 1; remain -= event_len + 1;

    // Activation text
    if (!nature_of_activation_text.deserialize(duck, data, remain, activ_len)) {
        return;
    }

    // A large portion of fixed fields
    if (remain < 19) {
        return;
    }
    alert_message_time_remaining = data[0];


    //@@@@

    // Process descriptor list.
    const size_t desc_len = GetUInt16(data) & 0x03FF;
    data += 2; remain -= 2;
    if (desc_len > remain) {
        return;
    }
    descs.add(data, desc_len);

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section (only one is allowed in an EAS table).
    uint8_t payload[MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Fixed part of the section.
    PutUInt8(data, protocol_version);
    PutUInt16(data + 1, EAS_event_ID);

    //@@@@@@@@@@@

    // Insert descriptors (all or some, depending on the remaining space).
    descs.lengthSerialize(data, remain, 0, 0x003F, 10);

    // Add one single section to the table.
    table.addSection(new Section(_table_id,
                                 false,   // is_private_section (should be true but SCTE 18 specifies it as zero).
                                 0,       // tid_ext
                                 version,
                                 is_current,
                                 0,       // section_number
                                 0,       //last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sequence_number", version, false);
    root->setIntAttribute(u"protocol_version", protocol_version, false);
    root->setIntAttribute(u"EAS_event_ID", EAS_event_ID, true);
    root->setAttribute(u"EAS_originator_code", EAS_originator_code);
    root->setAttribute(u"EAS_event_code", EAS_event_code);
    nature_of_activation_text.toXML(duck, root, u"nature_of_activation_text", true);
    if (alert_message_time_remaining != 0) {
        root->setIntAttribute(u"alert_message_time_remaining", alert_message_time_remaining, false);
    }
    root->setDateTimeAttribute(u"event_start_time", event_start_time);
    if (event_duration != 0) {
        root->setIntAttribute(u"event_duration", event_duration, false);
    }
    root->setIntAttribute(u"alert_priority", alert_priority, false);
    if (details_OOB_source_ID != 0) {
        root->setIntAttribute(u"details_OOB_source_ID", details_OOB_source_ID, true);
    }
    if (details_major_channel_number != 0) {
        root->setIntAttribute(u"details_major_channel_number", details_major_channel_number, true);
    }
    if (details_minor_channel_number != 0) {
        root->setIntAttribute(u"details_minor_channel_number", details_minor_channel_number, true);
    }
    if (audio_OOB_source_ID != 0) {
        root->setIntAttribute(u"audio_OOB_source_ID", audio_OOB_source_ID, true);
    }
    alert_text.toXML(duck, root, u"alert_text", true);
    for (auto it = locations.begin(); it != locations.end(); ++it) {
        xml::Element* e = root->addElement(u"location");
        e->setIntAttribute(u"state_code", it->state_code, false);
        e->setIntAttribute(u"county_subdivision", it->county_subdivision, false);
        e->setIntAttribute(u"county_code", it->county_code, false);
    }
    for (auto it = exceptions.begin(); it != exceptions.end(); ++it) {
        xml::Element* e = root->addElement(u"exception");
        if (it->in_band) {
            e->setIntAttribute(u"exception_major_channel_number", it->exception_major_channel_number, true);
            e->setIntAttribute(u"exception_minor_channel_number", it->exception_minor_channel_number, true);
        }
        else {
            e->setIntAttribute(u"exception_OOB_source_ID", it->exception_OOB_source_ID, true);
        }
    }
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::fromXML(DuckContext& duck, const xml::Element* element)
{
    clear();
    xml::ElementVector others;
    xml::ElementVector locs;
    xml::ElementVector exceps;

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(version, u"sequence_number", true, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint16_t>(EAS_event_ID, u"EAS_event_ID", true) &&
        element->getAttribute(EAS_originator_code, u"EAS_originator_code", true, UString(), 3, 3) &&
        element->getAttribute(EAS_event_code, u"EAS_event_code", true, UString(), 0, 255) &&
        nature_of_activation_text.fromXML(duck, element, u"nature_of_activation_text", false) &&
        element->getIntAttribute<uint8_t>(alert_message_time_remaining, u"alert_message_time_remaining", false, 0, 0, 120) &&
        element->getDateTimeAttribute(event_start_time, u"event_start_time", true) &&
        element->getIntAttribute<uint16_t>(event_duration, u"event_duration", false, 0, 0, 6000) &&
        element->getIntAttribute<uint8_t>(alert_priority, u"alert_priority", true, 0, 0, 15) &&
        element->getIntAttribute<uint16_t>(details_OOB_source_ID, u"details_OOB_source_ID", false) &&
        element->getIntAttribute<uint16_t>(details_major_channel_number, u"details_major_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute<uint16_t>(details_minor_channel_number, u"details_minor_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute<uint16_t>(audio_OOB_source_ID, u"audio_OOB_source_ID", false) &&
        alert_text.fromXML(duck, element, u"alert_text", false) &&
        element->getChildren(locs, u"location", 1, 31) &&
        element->getChildren(exceps, u"exception", 0, 255) &&
        descs.fromXML(duck, others, element, u"location,exception,nature_of_activation_text");

    for (size_t i = 0; _is_valid && i < locs.size(); ++i) {
        Location loc;
        _is_valid =
            locs[i]->getIntAttribute<uint8_t>(loc.state_code, u"state_code", true, 0, 0, 99) &&
            locs[i]->getIntAttribute<uint8_t>(loc.county_subdivision, u"county_subdivision", true, 0, 0, 9) &&
            locs[i]->getIntAttribute<uint16_t>(loc.county_code, u"county_code", true, 0, 0, 909);
        if (_is_valid) {
            locations.push_back(loc);
        }
    }

    for (size_t i = 0; _is_valid && i < exceps.size(); ++i) {
        Exception exc;
        bool wrong = false;
        exc.in_band = exceps[i]->hasAttribute(u"exception_major_channel_number") && exceps[i]->hasAttribute(u"exception_minor_channel_number");
        if (exc.in_band) {
            wrong = exceps[i]->hasAttribute(u"exception_OOB_source_ID");
            _is_valid =
                exceps[i]->getIntAttribute<uint16_t>(exc.exception_major_channel_number, u"exception_major_channel_number", true, 0, 0, 0x03FF) &&
                exceps[i]->getIntAttribute<uint16_t>(exc.exception_minor_channel_number, u"exception_minor_channel_number", true, 0, 0, 0x03FF);
        }
        else {
            wrong = exceps[i]->hasAttribute(u"exception_major_channel_number") || exceps[i]->hasAttribute(u"exception_minor_channel_number");
            _is_valid = exceps[i]->getIntAttribute<uint16_t>(exc.exception_OOB_source_ID, u"exception_OOB_source_ID", true);
        }
        if (wrong) {
            _is_valid = false;
            exceps[i]->report().error(u"invalid combination of attributes in <%s>, line %d", {exceps[i]->name(), exceps[i]->lineNumber()});
        }
        if (_is_valid) {
            exceptions.push_back(exc);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a CableEmergencyAlertTable section.
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    // Fixed past
    if (size < 7) {
        display.displayExtraData(data, size, indent);
        return;
    }

    // Fixed part
    const uint8_t protocol_version = data[0];
    //@@@@@@@@@

    strm << margin << UString::Format(u"Protocol version: 0x%X (%d)", {protocol_version, protocol_version}) << std::endl;
}
