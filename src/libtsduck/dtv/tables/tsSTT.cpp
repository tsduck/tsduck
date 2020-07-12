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

#include "tsSTT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"STT"
#define MY_CLASS ts::STT
#define MY_TID ts::TID_STT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::STT::STT() :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, 0, true),
    protocol_version(0),
    system_time(0),
    GPS_UTC_offset(0),
    DS_status(false),
    DS_day_of_month(0),
    DS_hour(0),
    descs(this)
{
}

ts::STT::STT(const STT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    system_time(other.system_time),
    GPS_UTC_offset(other.GPS_UTC_offset),
    DS_status(other.DS_status),
    DS_day_of_month(other.DS_day_of_month),
    DS_hour(other.DS_hour),
    descs(this, other.descs)
{
}

ts::STT::STT(DuckContext& duck, const BinaryTable& table) :
    STT()
{
    deserialize(duck, table);
}

ts::STT::STT(DuckContext& duck, const Section& section) :
    STT()
{
    deserializeSection(duck, section);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::STT::tableIdExtension() const
{
    return 0x0000;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::STT::clearContent()
{
    protocol_version = 0;
    system_time = 0;
    GPS_UTC_offset = 0;
    DS_status = 0;
    DS_day_of_month = 0;
    DS_hour = 0;
    descs.clear();
}


//----------------------------------------------------------------------------
// Convert the GPS system time in this object in a UTC time.
//----------------------------------------------------------------------------

ts::Time ts::STT::utcTime() const
{
    if (system_time == 0) {
        // Time is unset.
        return Time::Epoch;
    }
    else {
        // Add difference between 1970 and 180 to convert from GPS to UTC.
        // Then substract GPS-UTC offset (see ATSC A/65 section 6.1).
        return Time::UnixTimeToUTC(system_time + Time::UnixEpochToGPS - GPS_UTC_offset);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::STT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Deserialize first section.
    if (table.sectionCount() > 0 && !table.sectionAt(0).isNull()) {
        deserializeSection(duck, *table.sectionAt(0));
    }
}


//----------------------------------------------------------------------------
// Deserialize one section.
//----------------------------------------------------------------------------

void ts::STT::deserializeSection(DuckContext& duck, const Section& section)
{
    // Clear table content
    _is_valid = false;
    protocol_version = 0;
    descs.clear();

    // Get fixed properties (should be identical in all STT)
    version = section.version();       // should be zero
    is_current = section.isCurrent();  // should be true

    // Section payload:
    const uint8_t* data = section.payload();
    size_t remain = section.payloadSize();

    // Giveup if not a valid STT section.
    if (!section.isValid() || section.tableId() != TID_STT || remain < 8) {
        return;
    }

    // Get fixed fields.
    protocol_version = data[0];
    system_time = GetUInt32(data + 1);
    GPS_UTC_offset = data[5];
    DS_status = (data[6] & 0x80) != 0;
    DS_day_of_month = data[6] & 0x1F;
    DS_hour = data[7];
    data += 8; remain -= 8;

    // Add descriptors.
    descs.add(data, remain);
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::STT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that a STT is not allowed to use more than one section, see A/65, section 6.1.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add fixed fields.
    data[0] = protocol_version;
    PutUInt32(data + 1, system_time);
    data[5] = GPS_UTC_offset;
    data[6] = (DS_status ? 0xE0 : 0x60) | (DS_day_of_month & 0x1F);
    data[7] = DS_hour;
    data += 8; remain -= 8;

    // Insert descriptor list (without leading length field)
    descs.serialize(data, remain);

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 0,                // tid_ext is always zero in STT
                                 version,          // Should be zero
                                 is_current,       // should be true
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display an STT section.
//----------------------------------------------------------------------------

void ts::STT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 8) {
        // Fixed part.
        const uint32_t time = GetUInt32(data + 1);
        const uint8_t offset = data[5];
        const Time utc(Time::UnixTimeToUTC(time + Time::UnixEpochToGPS - offset));

        strm << margin << UString::Format(u"Protocol version: %d", {data[0]}) << std::endl
             << margin << UString::Format(u"System time: 0x%X (%d), GPS-UTC offset: 0x%X (%d)", {time, time, offset, offset}) << std::endl
             << margin << "Corresponding UTC time: " << (time == 0 ? u"none" : utc.format(Time::DATE | Time::TIME)) << std::endl
             << margin << UString::Format(u"Daylight saving time: %s, next switch day: %d, hour: %d", {UString::YesNo((data[6] & 0x80) != 0), data[6] & 0x1F, data[7]}) << std::endl;

        // Display descriptors. Use fake PDS for ATSC.
        display.displayDescriptorList(section, data + 8, size - 8, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::STT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"system_time", system_time);
    root->setIntAttribute(u"GPS_UTC_offset", GPS_UTC_offset);
    root->setBoolAttribute(u"DS_status", DS_status);
    if (DS_day_of_month > 0) {
        root->setIntAttribute(u"DS_day_of_month", DS_day_of_month & 0x1F);
    }
    if (DS_day_of_month > 0 || DS_hour > 0) {
        root->setIntAttribute(u"DS_hour", DS_hour);
    }
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::STT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
           element->getIntAttribute<uint32_t>(system_time, u"system_time", true) &&
           element->getIntAttribute<uint8_t>(GPS_UTC_offset, u"GPS_UTC_offset", true) &&
           element->getBoolAttribute(DS_status, u"DS_status", true) &&
           element->getIntAttribute<uint8_t>(DS_day_of_month, u"DS_day_of_month", false, 0, 0, 31) &&
           element->getIntAttribute<uint8_t>(DS_hour, u"DS_hour", false, 0, 0, 23) &&
           descs.fromXML(duck, element);
}
