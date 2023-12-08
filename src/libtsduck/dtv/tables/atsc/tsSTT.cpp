//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSTT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    PSIBuffer buf(duck, section.payload(), section.payloadSize());
    deserializePayload(buf, section);
    if (buf.error() || buf.remainingReadBytes() > 0) {
        invalidate();
    }
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::STT::tableIdExtension() const
{
    return 0x0000;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::STT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the STT section is limited to 1024 bytes in ATSC A/65.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
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

void ts::STT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    protocol_version = buf.getUInt8();
    system_time = buf.getUInt32();
    GPS_UTC_offset = buf.getUInt8();
    DS_status = buf.getBool();
    buf.skipBits(2);
    buf.getBits(DS_day_of_month, 5);
    DS_hour = buf.getUInt8();
    buf.getDescriptorList(descs);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::STT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // An STT is not allowed to use more than one section, see A/65, section 6.1.
    buf.putUInt8(protocol_version);
    buf.putUInt32(system_time);
    buf.putUInt8(GPS_UTC_offset);
    buf.putBit(DS_status);
    buf.putBits(0xFF, 2);
    buf.putBits(DS_day_of_month, 5);
    buf.putUInt8(DS_hour);
    buf.putPartialDescriptorList(descs);
}


//----------------------------------------------------------------------------
// A static method to display an STT section.
//----------------------------------------------------------------------------

void ts::STT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()}) << std::endl;
        const uint32_t time = buf.getUInt32();
        const uint8_t offset = buf.getUInt8();
        const Time utc(Time::UnixTimeToUTC(time + Time::UnixEpochToGPS - offset));
        disp << margin << UString::Format(u"System time: 0x%X (%<d), GPS-UTC offset: 0x%X (%<d)", {time, offset}) << std::endl;
        disp << margin << "Corresponding UTC time: " << (time == 0 ? u"none" : utc.format(Time::DATETIME)) << std::endl;
        disp << margin << "Daylight saving time: " << UString::YesNo(buf.getBool());
        buf.skipBits(2);
        disp << UString::Format(u", next switch day: %d", {buf.getBits<uint8_t>(5)});
        disp << UString::Format(u", hour: %d", {buf.getUInt8()}) << std::endl;
        disp.displayDescriptorList(section, buf, margin);
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
    return element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
           element->getIntAttribute(system_time, u"system_time", true) &&
           element->getIntAttribute(GPS_UTC_offset, u"GPS_UTC_offset", true) &&
           element->getBoolAttribute(DS_status, u"DS_status", true) &&
           element->getIntAttribute(DS_day_of_month, u"DS_day_of_month", false, 0, 0, 31) &&
           element->getIntAttribute(DS_hour, u"DS_hour", false, 0, 0, 23) &&
           descs.fromXML(duck, element);
}
