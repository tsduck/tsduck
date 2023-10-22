//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCableEmergencyAlertTable.h"
#include "tsBinaryTable.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"

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
    // Although not MPEG-defined, SCTE sections are "non private".
    return false;
}

uint16_t ts::CableEmergencyAlertTable::tableIdExtension() const
{
    // Specified as zero in this table.
    return 0;
}

size_t ts::CableEmergencyAlertTable::maxPayloadSize() const
{
    // Although declared as a "non-private section" in the MPEG sense, the
    // CableEmergencyAlertTable section can use up to 4096 bytes in SCTE 18.
    return MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
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

void ts::CableEmergencyAlertTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    protocol_version = buf.getUInt8();
    EAS_event_ID = buf.getUInt16();
    buf.getUTF8(EAS_originator_code, 3);
    buf.getUTF8WithLength(EAS_event_code);
    buf.getMultipleStringWithLength(nature_of_activation_text);
    alert_message_time_remaining = buf.getUInt8();
    const uint32_t start = buf.getUInt32();
    event_start_time = start == 0 ? Time::Epoch : Time::GPSSecondsToUTC(start);
    event_duration = buf.getUInt16();
    buf.skipBits(12);
    buf.getBits(alert_priority, 4);
    details_OOB_source_ID = buf.getUInt16();
    buf.skipBits(6);
    buf.getBits(details_major_channel_number, 10);
    buf.skipBits(6);
    buf.getBits(details_minor_channel_number, 10);
    audio_OOB_source_ID = buf.getUInt16();
    buf.getMultipleStringWithLength(alert_text, 2); // unusual 2-byte length field

    // List of locations.
    size_t count = buf.getUInt8();
    while (!buf.readError() && count-- > 0) {
        Location loc;
        loc.state_code = buf.getUInt8();
        buf.getBits(loc.county_subdivision, 4);
        buf.skipBits(2);
        buf.getBits(loc.county_code, 10);
        locations.push_back(loc);
    }

    // List of exceptions.
    count = buf.getUInt8();
    while (!buf.readError() && count-- > 0) {
        Exception exc;
        exc.in_band = buf.getBool();
        buf.skipBits(7);
        if (exc.in_band) {
            buf.skipBits(6);
            buf.getBits(exc.major_channel_number, 10);
            buf.skipBits(6);
            buf.getBits(exc.minor_channel_number, 10);
        }
        else {
            buf.skipBits(16);
            exc.OOB_source_ID = buf.getUInt16();
        }
        exceptions.push_back(exc);
    }

    // Descriptor list (with 10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CableEmergencyAlertTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // A cable_emergency_alert_table can have only one section.

    // Locations and exceptions cannot have more than 255 entries each (one-byte counter).
    if (locations.size() > 255 || exceptions.size() > 255) {
        buf.setUserError();
        return;
    }

    buf.putUInt8(protocol_version);
    buf.putUInt16(EAS_event_ID);
    buf.putFixedUTF8(EAS_originator_code, 3, ' ');
    buf.putUTF8WithLength(EAS_event_code);
    buf.putMultipleStringWithLength(nature_of_activation_text);
    buf.putUInt8(alert_message_time_remaining);
    buf.putUInt32(event_start_time == Time::Epoch ? 0 : uint32_t(event_start_time.toGPSSeconds()));
    buf.putUInt16(event_duration);
    buf.putBits(0xFFFF, 12);
    buf.putBits(alert_priority, 4);
    buf.putUInt16(details_OOB_source_ID);
    buf.putBits(0xFF, 6);
    buf.putBits(details_major_channel_number, 10);
    buf.putBits(0xFF, 6);
    buf.putBits(details_minor_channel_number, 10);
    buf.putUInt16(audio_OOB_source_ID);
    buf.putMultipleStringWithLength(alert_text, 2); // 2-byte length field

    // Serialize locations.
    buf.putUInt8(uint8_t(locations.size()));
    for (auto it = locations.begin(); !buf.writeError() && it != locations.end(); ++it) {
        buf.putUInt8(it->state_code);
        buf.putBits(it->county_subdivision, 4);
        buf.putBits(0xFF, 2);
        buf.putBits(it->county_code, 10);
    }

    // Serialize exceptions.
    buf.putUInt8(uint8_t(exceptions.size()));
    for (auto it = exceptions.begin(); !buf.writeError() && it != exceptions.end(); ++it) {
        buf.putBits(it->in_band, 1);
        buf.putBits(0xFF, 7);
        if (it->in_band) {
            buf.putBits(0xFF, 6);
            buf.putBits(it->major_channel_number, 10);
            buf.putBits(0xFF, 6);
            buf.putBits(it->minor_channel_number, 10);
        }
        else {
            buf.putUInt16(0xFFFF);
            buf.putUInt16(it->OOB_source_ID);
        }
    }

    // Insert descriptors (all or some, depending on the remaining space).
    buf.putPartialDescriptorListWithLength(descs, 0, NPOS, 10); // 10-bit length field
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
    for (const auto& it : locations) {
        xml::Element* e = root->addElement(u"location");
        e->setIntAttribute(u"state_code", it.state_code, false);
        e->setIntAttribute(u"county_subdivision", it.county_subdivision, false);
        e->setIntAttribute(u"county_code", it.county_code, false);
    }
    for (const auto& it : exceptions) {
        xml::Element* e = root->addElement(u"exception");
        if (it.in_band) {
            e->setIntAttribute(u"major_channel_number", it.major_channel_number, false);
            e->setIntAttribute(u"minor_channel_number", it.minor_channel_number, false);
        }
        else {
            e->setIntAttribute(u"OOB_source_ID", it.OOB_source_ID, true);
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
        element->getIntAttribute(version, u"sequence_number", true, 0, 0, 31) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(EAS_event_ID, u"EAS_event_ID", true) &&
        element->getAttribute(EAS_originator_code, u"EAS_originator_code", true, UString(), 3, 3) &&
        element->getAttribute(EAS_event_code, u"EAS_event_code", true, UString(), 0, 255) &&
        nature_of_activation_text.fromXML(duck, element, u"nature_of_activation_text", false) &&
        element->getIntAttribute(alert_message_time_remaining, u"alert_message_time_remaining", false, 0, 0, 120) &&
        element->getDateTimeAttribute(event_start_time, u"event_start_time", false, Time::Epoch) &&
        element->getIntAttribute(event_duration, u"event_duration", false, 0, 0, 6000) &&
        element->getIntAttribute(alert_priority, u"alert_priority", true, 0, 0, 15) &&
        element->getIntAttribute(details_OOB_source_ID, u"details_OOB_source_ID", false) &&
        element->getIntAttribute(details_major_channel_number, u"details_major_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute(details_minor_channel_number, u"details_minor_channel_number", false, 0, 0, 0x03FF) &&
        element->getIntAttribute(audio_OOB_source_ID, u"audio_OOB_source_ID", false) &&
        alert_text.fromXML(duck, element, u"alert_text", false) &&
        element->getChildren(locs, u"location", 1, 31) &&
        element->getChildren(exceps, u"exception", 0, 255) &&
        descs.fromXML(duck, others, element, u"location,exception,alert_text,nature_of_activation_text");

    for (size_t i = 0; ok && i < locs.size(); ++i) {
        Location loc;
        ok = locs[i]->getIntAttribute(loc.state_code, u"state_code", true, 0, 0, 99) &&
             locs[i]->getIntAttribute(loc.county_subdivision, u"county_subdivision", true, 0, 0, 9) &&
             locs[i]->getIntAttribute(loc.county_code, u"county_code", true, 0, 0, 909);
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
                exceps[i]->getIntAttribute(exc.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
                exceps[i]->getIntAttribute(exc.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF);
        }
        else {
            wrong = exceps[i]->hasAttribute(u"major_channel_number") || exceps[i]->hasAttribute(u"minor_channel_number");
            ok = exceps[i]->getIntAttribute(exc.OOB_source_ID, u"OOB_source_ID", true);
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

void ts::CableEmergencyAlertTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(7)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"EAS event id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << "Originator code: \"" << buf.getUTF8(3) << "\"";
        disp << ", event code: \"" << buf.getUTF8WithLength() << "\"" << std::endl;
    }

    disp.displayATSCMultipleString(buf, 1, margin, u"Nature of activation: ");

    if (buf.canReadBytes(17)) {
        disp << margin << UString::Format(u"Remaining: %d seconds", {buf.getUInt8()});
        const uint32_t start = buf.getUInt32();
        disp << ", start time: " << (start == 0 ? u"immediate" : Time::GPSSecondsToUTC(start).format(Time::DATETIME));
        disp << UString::Format(u", duration: %d minutes", {buf.getUInt16()}) << std::endl;
        buf.skipBits(12);
        disp << margin << UString::Format(u"Alert priority: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp << margin << UString::Format(u"Details: OOB id: 0x%X (%<d)", {buf.getUInt16()});
        buf.skipBits(6);
        disp << ", major.minor: " << buf.getBits<uint16_t>(10);
        buf.skipBits(6);
        disp << "." << buf.getBits<uint16_t>(10) << std::endl;
        disp << margin << UString::Format(u"Audio: OOB id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp.displayATSCMultipleString(buf, 2, margin, u"Alert text: ");
    }

    // Display locations.
    size_t count = buf.getUInt8();
    if (!buf.error()) {
        disp << margin << UString::Format(u"Number of locations: %d", {count}) << std::endl;
    }
    while (buf.canReadBytes(3) && count-- > 0) {
        const uint8_t state = buf.getUInt8();
        const uint8_t subd = buf.getBits<uint8_t>(4);
        buf.skipBits(2);
        const uint16_t county = buf.getBits<uint16_t>(10);
        disp << margin
             << UString::Format(u"  State code: %d, county: %d, subdivision: %s", {state, county, DataName(MY_XML_NAME, u"CountySubdivision", subd, NamesFlags::VALUE)})
             << std::endl;
    }

    // Display exceptions.
    count = buf.getUInt8();
    if (!buf.error()) {
        disp << margin << UString::Format(u"Number of exceptions: %d", {count}) << std::endl;
    }
    while (buf.canReadBytes(5) && count-- > 0) {
        const bool inband = buf.getBool();
        buf.skipBits(7);
        disp << margin << UString::Format(u"  In-band: %s", {inband});
        if (inband) {
            buf.skipBits(6);
            const uint16_t major = buf.getBits<uint16_t>(10);
            buf.skipBits(6);
            const uint16_t minor = buf.getBits<uint16_t>(10);
            disp << UString::Format(u", exception major.minor: %d.%d", {major, minor}) << std::endl;
        }
        else {
            buf.skipBits(16);
            disp << UString::Format(u", exception OOB id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }

    // Display descriptor list with 10-bit length field.
    disp.displayDescriptorListWithLength(section, buf, margin, UString(), UString(), 10);
}
