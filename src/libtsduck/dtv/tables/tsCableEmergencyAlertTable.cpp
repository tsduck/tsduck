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

#include "tsCableEmergencyAlertTable.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"cable_emergency_alert_table"
#define MY_CLASS ts::CableEmergencyAlertTable
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD (ts::Standards::SCTE | ts::Standards::ATSC)

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


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
    major_channel_number(0),
    minor_channel_number(0),
    OOB_source_ID(oob)
{
}

ts::CableEmergencyAlertTable::Exception::Exception(uint16_t major, uint16_t minor) :
    in_band(true),
    major_channel_number(major),
    minor_channel_number(minor),
    OOB_source_ID(0)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::CableEmergencyAlertTable::isPrivate() const
{
    // Although not MPEG-defined, SCTE section are "non private".
    return false;
}

uint16_t ts::CableEmergencyAlertTable::tableIdExtension() const
{
    // Specified as zero in this table.
    return 0;
}


//----------------------------------------------------------------------------
// Clear all fields.
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::clearContent()
{
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
    data += event_len; remain -= event_len;

    // Activation text
    if (!nature_of_activation_text.lengthDeserialize(duck, data, remain)) {
        return;
    }

    // A large portion of fixed fields
    if (remain < 19) {
        return;
    }
    alert_message_time_remaining = data[0];
    const uint32_t start = GetUInt32(data + 1);
    event_start_time = start == 0 ? Time::Epoch : Time::GPSSecondsToUTC(start);
    event_duration = GetUInt16(data + 5);
    alert_priority = data[8] & 0x0F;
    details_OOB_source_ID = GetUInt16(data + 9);
    details_major_channel_number = GetUInt16(data + 11) & 0x03FF;
    details_minor_channel_number = GetUInt16(data + 13) & 0x03FF;
    audio_OOB_source_ID = GetUInt16(data + 15);
    const size_t alert_len = GetUInt16(data + 17);
    data += 19; remain -= 19;

    // Alert text.
    if (!alert_text.deserialize(duck, data, remain, alert_len, true)) {
        return;
    }

    // List of locations.
    if (remain < 1 || remain < 1 + 3 * size_t(*data)) {
        return;
    }
    size_t count = *data++;
    remain--;

    while (count-- > 0) {
        locations.push_back(Location(data[0], data[1] >> 4, GetUInt16(data + 1) & 0x03FF));
        data += 3; remain -= 3;
    }

    // List of exceptions.
    if (remain < 1 || remain < 1 + 5 * size_t(*data)) {
        return;
    }
    count = *data++;
    remain--;

    while (count-- > 0) {
        if ((data[0] & 0x80) != 0) {
            // In-band reference
            exceptions.push_back(Exception(GetUInt16(data + 1) & 0x03FF, GetUInt16(data + 3) & 0x03FF));
        }
        else {
            exceptions.push_back(Exception(GetUInt16(data + 3)));
        }
        data += 5; remain -= 5;
    }

    // Descriptor list.
    if (remain < 2) {
        return;
    }
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
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Fixed part of the section.
    PutUInt8(data, protocol_version);
    PutUInt16(data + 1, EAS_event_ID);

    // Encode exactly 3 characters for EAS_originator_code.
    for (size_t i = 0; i < 3; ++i) {
        data[3 + i] = uint8_t(i < EAS_originator_code.size() ? EAS_originator_code[i] : SPACE);
    }
    data += 6; remain -= 6;

    // Where to store the length of EAS_event_code.
    size_t len = std::min<size_t>(EAS_event_code.size(), 255);
    *data++ = uint8_t(len);
    remain--;
    for (size_t i = 0; i < len; ++i) {
        *data++ = uint8_t(EAS_event_code[i]);
        remain--;
    }

    nature_of_activation_text.lengthSerialize(duck, data, remain);

    // A large portion of fixed fields
    if (remain < 19) {
        return;
    }
    PutUInt8(data, alert_message_time_remaining);
    PutUInt32(data + 1, event_start_time == Time::Epoch ? 0 : uint32_t(event_start_time.toGPSSeconds()));
    PutUInt16(data + 5, event_duration);
    PutUInt16(data + 7, 0xFFF0 | alert_priority);
    PutUInt16(data + 9, details_OOB_source_ID);
    PutUInt16(data + 11, 0xFC00 | details_major_channel_number);
    PutUInt16(data + 13, 0xFC00 | details_minor_channel_number);
    PutUInt16(data + 15, audio_OOB_source_ID);
    uint8_t* len_addr = data + 17; // place-holder for alert_text length;
    data += 19; remain -= 19;

    PutUInt16(len_addr, uint16_t(alert_text.serialize(duck, data, remain, NPOS, true)));

    // Serialize locations.
    if (remain < 1) {
        return;
    }
    len_addr = data; // address of location_code_count
    *data++ = 0;
    remain--;
    for (auto it = locations.begin(); remain >= 3 && it != locations.end(); ++it) {
        PutUInt8(data, it->state_code);
        PutUInt16(data + 1, uint16_t((uint16_t(it->county_subdivision) << 12) | 0x0C00 | (it->county_code & 0x03FF)));
        data += 3; remain -= 3;
        (*len_addr)++; // increment number of serialized locations
    }

    // Serialize exceptions.
    if (remain < 1) {
        return;
    }
    len_addr = data; // address of exception_count
    *data++ = 0;
    remain--;
    for (auto it = exceptions.begin(); remain >= 5 && it != exceptions.end(); ++it) {
        PutUInt8(data, it->in_band ? 0xFF : 0x7F);
        if (it->in_band) {
            PutUInt16(data + 1, 0xFC00 | it->major_channel_number);
            PutUInt16(data + 3, 0xFC00 | it->minor_channel_number);
        }
        else {
            PutUInt16(data + 1, 0xFFFF);
            PutUInt16(data + 3, it->OOB_source_ID);
        }
        data += 5; remain -= 5;
        (*len_addr)++; // increment number of serialized exceptions
    }

    // Insert descriptors (all or some, depending on the remaining space).
    if (remain < 2) {
        return;
    }
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
    if (event_start_time != Time::Epoch) {
        root->setDateTimeAttribute(u"event_start_time", event_start_time);
    }
    if (event_duration != 0) {
        root->setIntAttribute(u"event_duration", event_duration, false);
    }
    root->setIntAttribute(u"alert_priority", alert_priority, false);
    if (details_OOB_source_ID != 0) {
        root->setIntAttribute(u"details_OOB_source_ID", details_OOB_source_ID, true);
    }
    if (details_major_channel_number != 0) {
        root->setIntAttribute(u"details_major_channel_number", details_major_channel_number, false);
    }
    if (details_minor_channel_number != 0) {
        root->setIntAttribute(u"details_minor_channel_number", details_minor_channel_number, false);
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
            e->setIntAttribute(u"major_channel_number", it->major_channel_number, false);
            e->setIntAttribute(u"minor_channel_number", it->minor_channel_number, false);
        }
        else {
            e->setIntAttribute(u"OOB_source_ID", it->OOB_source_ID, true);
        }
    }
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CableEmergencyAlertTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector others;
    xml::ElementVector locs;
    xml::ElementVector exceps;

    bool ok =
        element->getIntAttribute<uint8_t>(version, u"sequence_number", true, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint16_t>(EAS_event_ID, u"EAS_event_ID", true) &&
        element->getAttribute(EAS_originator_code, u"EAS_originator_code", true, UString(), 3, 3) &&
        element->getAttribute(EAS_event_code, u"EAS_event_code", true, UString(), 0, 255) &&
        nature_of_activation_text.fromXML(duck, element, u"nature_of_activation_text", false) &&
        element->getIntAttribute<uint8_t>(alert_message_time_remaining, u"alert_message_time_remaining", false, 0, 0, 120) &&
        element->getDateTimeAttribute(event_start_time, u"event_start_time", false, Time::Epoch) &&
        element->getIntAttribute<uint16_t>(event_duration, u"event_duration", false, 0, 0, 6000) &&
        element->getIntAttribute<uint8_t>(alert_priority, u"alert_priority", true, 0, 0, 15) &&
        element->getIntAttribute<uint16_t>(details_OOB_source_ID, u"details_OOB_source_ID", false) &&
        element->getIntAttribute<uint16_t>(details_major_channel_number, u"details_major_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute<uint16_t>(details_minor_channel_number, u"details_minor_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute<uint16_t>(audio_OOB_source_ID, u"audio_OOB_source_ID", false) &&
        alert_text.fromXML(duck, element, u"alert_text", false) &&
        element->getChildren(locs, u"location", 1, 31) &&
        element->getChildren(exceps, u"exception", 0, 255) &&
        descs.fromXML(duck, others, element, u"location,exception,alert_text,nature_of_activation_text");

    for (size_t i = 0; ok && i < locs.size(); ++i) {
        Location loc;
        ok = locs[i]->getIntAttribute<uint8_t>(loc.state_code, u"state_code", true, 0, 0, 99) &&
             locs[i]->getIntAttribute<uint8_t>(loc.county_subdivision, u"county_subdivision", true, 0, 0, 9) &&
             locs[i]->getIntAttribute<uint16_t>(loc.county_code, u"county_code", true, 0, 0, 909);
        if (ok) {
            locations.push_back(loc);
        }
    }

    for (size_t i = 0; ok && i < exceps.size(); ++i) {
        Exception exc;
        bool wrong = false;
        exc.in_band = exceps[i]->hasAttribute(u"major_channel_number") && exceps[i]->hasAttribute(u"minor_channel_number");
        if (exc.in_band) {
            wrong = exceps[i]->hasAttribute(u"OOB_source_ID");
            ok =
                exceps[i]->getIntAttribute<uint16_t>(exc.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
                exceps[i]->getIntAttribute<uint16_t>(exc.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF);
        }
        else {
            wrong = exceps[i]->hasAttribute(u"major_channel_number") || exceps[i]->hasAttribute(u"minor_channel_number");
            ok = exceps[i]->getIntAttribute<uint16_t>(exc.OOB_source_ID, u"OOB_source_ID", true);
        }
        if (wrong) {
            ok = false;
            exceps[i]->report().error(u"invalid combination of attributes in <%s>, line %d", {exceps[i]->name(), exceps[i]->lineNumber()});
        }
        if (ok) {
            exceptions.push_back(exc);
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// A static method to display a CableEmergencyAlertTable section.
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    bool ok = size >= 7;

    if (ok) {
        const uint8_t protocol_version = data[0];
        const uint16_t event_id = GetUInt16(data + 1);
        const UString orig_code(UString::FromUTF8(reinterpret_cast<const char*>(data + 3), 3));
        size_t len = data[6];
        data += 7; size -= 7;
        if (len > size) {
            len = size;
        }
        const UString event_code(UString::FromUTF8(reinterpret_cast<const char*>(data), len));
        data += len; size -= len;

        strm << margin << UString::Format(u"Protocol version: 0x%X (%d)", {protocol_version, protocol_version}) << std::endl
             << margin << UString::Format(u"EAS event id: 0x%X (%d)", {event_id, event_id}) << std::endl
             << margin << UString::Format(u"Originator code: \"%s\", event code: \"%s\"", {orig_code, event_code}) << std::endl;

        if (size > 0) {
            len = *data++;
            size--;
            ATSCMultipleString::Display(display, u"Nature of activation: ", indent, data, size, len);
        }

        ok = size >= 17;

        if (ok) {
            const uint8_t remaining = data[0];
            const uint32_t start_value = GetUInt32(data + 1);
            const Time start_time(Time::GPSSecondsToUTC(start_value));
            const uint16_t duration = GetUInt16(data + 5);
            const uint8_t priority = data[8] & 0x0F;
            const uint16_t details_oob = GetUInt16(data + 9);
            const uint16_t details_major = GetUInt16(data + 11) & 0x03FF;
            const uint16_t details_minor = GetUInt16(data + 13) & 0x03FF;
            const uint16_t audio_oob = GetUInt16(data + 15);
            data += 17; size -= 17;

            strm << margin
                 << UString::Format(u"Remaining: %d seconds, start time: %s, duration: %d minutes", {remaining, start_value == 0 ? u"immediate" : start_time.format(Time::DATETIME), duration})
                 << std::endl
                 << margin
                 << UString::Format(u"Alert priority: %d", {priority})
                 << std::endl
                 << margin
                 << UString::Format(u"Details: OOB id: 0x%X (%d), major.minor: %d.%d", {details_oob, details_oob, details_major, details_minor})
                 << std::endl
                 << margin
                 << UString::Format(u"Audio: OOB id: 0x%X (%d)", {audio_oob, audio_oob})
                 << std::endl;

            ok = size >= 2;
        }

        if (ok) {
            len = GetUInt16(data);
            data += 2; size -= 2;
            ATSCMultipleString::Display(display, u"Alert text: ", indent, data, size, len);
            ok = size >= 1;
        }

        if (ok) {
            // Display locations.
            len = *data++;
            size--;
            strm << margin << UString::Format(u"Number of locations: %d", {len}) << std::endl;
            while (ok && len-- > 0) {
                ok = size >= 3;
                if (ok) {
                    const uint8_t state = data[0];
                    const uint8_t subd = data[1] >> 4;
                    const uint16_t county = GetUInt16(data + 1) & 0x03FF;
                    strm << margin
                         << UString::Format(u"  State code: %d, county: %d, subdivision: %s", {state, county, NameFromSection(u"EASCountySubdivision", subd, names::VALUE)})
                         << std::endl;

                }
                data += 3; size -= 3;
            }
            ok = ok && size >= 1;
        }

        if (ok) {
            // Display exceptions.
            len = *data++;
            size--;
            strm << margin << UString::Format(u"Number of exceptions: %d", {len}) << std::endl;
            while (ok && len-- > 0) {
                ok = size >= 5;
                if (ok) {
                    const bool inband = (data[0] & 0x80) != 0;
                    strm << margin << UString::Format(u"  In-band: %s", {inband});
                    if (inband) {
                        strm << UString::Format(u", exception major.minor: %d.%d", {GetUInt16(data + 1) & 0x03FF, GetUInt16(data + 3) & 0x03FF}) << std::endl;
                    }
                    else {
                        strm << UString::Format(u", exception OOB id: 0x%X (%d)", {GetUInt16(data + 3), GetUInt16(data + 3)}) << std::endl;
                    }
                }
                data += 5; size -= 5;
            }
            ok = ok && size >= 2;
        }

        if (ok) {
            len = GetUInt16(data) & 0x03FF;
            data += 2; size -= 2;
            if (len > size) {
                len = size;
            }
            display.displayDescriptorList(section, data, len, indent);
            data += len; size -= len;
        }
    }

    display.displayExtraData(data, size, indent);
}
