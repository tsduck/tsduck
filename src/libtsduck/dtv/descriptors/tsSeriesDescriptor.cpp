//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSeriesDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"

#define MY_XML_NAME u"series_descriptor"
#define MY_CLASS ts::SeriesDescriptor
#define MY_DID ts::DID_ISDB_SERIES
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SeriesDescriptor::SeriesDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SeriesDescriptor::SeriesDescriptor(DuckContext& duck, const Descriptor& desc) :
    SeriesDescriptor()
{
    deserialize(duck, desc);
}

void ts::SeriesDescriptor::clearContent()
{
    series_id = 0;
    repeat_label = 0;
    program_pattern = 0;
    expire_date.reset();
    episode_number = 0;
    last_episode_number = 0;
    series_name.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(series_id);
    buf.putBits(repeat_label, 4);
    buf.putBits(program_pattern, 3);
    buf.putBit(expire_date.has_value());
    if (expire_date.has_value()) {
        buf.putMJD(expire_date.value(), 2);  // 2 bytes, date only
    }
    else {
        buf.putUInt16(0xFFFF);
    }
    buf.putBits(episode_number, 12);
    buf.putBits(last_episode_number, 12);
    buf.putString(series_name);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::deserializePayload(PSIBuffer& buf)
{
    series_id = buf.getUInt16();
    buf.getBits(repeat_label, 4);
    buf.getBits(program_pattern, 3);
    if (buf.getBool()) {
        expire_date = buf.getMJD(2);  // 2 bytes, date only
    }
    else {
        buf.skipBits(16);
    }
    buf.getBits(episode_number, 12);
    buf.getBits(last_episode_number, 12);
    buf.getString(series_name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"Series id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Repeat label: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp << margin << "Program pattern: " << DataName(MY_XML_NAME, u"ProgramPattern", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
        const bool date_valid = buf.getBool();
        const Time exp(buf.getMJD(2));
        disp << margin << "Expire date: " << (date_valid ? exp.format(Time::DATE) : u"unspecified") << std::endl;
        disp << margin << UString::Format(u"Episode: %d", {buf.getBits<uint16_t>(12)});
        disp << UString::Format(u"/%d", {buf.getBits<uint16_t>(12)}) << std::endl;
        disp << margin << "Series name: \"" << buf.getString() << u"\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SeriesDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"series_id", series_id, true);
    root->setIntAttribute(u"repeat_label", repeat_label);
    root->setIntAttribute(u"program_pattern", program_pattern);
    if (expire_date.has_value()) {
        root->setDateAttribute(u"expire_date", expire_date.value());
    }
    root->setIntAttribute(u"episode_number", episode_number);
    root->setIntAttribute(u"last_episode_number", last_episode_number);
    root->setAttribute(u"series_name", series_name, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SeriesDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(series_id, u"series_id", true) &&
        element->getIntAttribute(repeat_label, u"repeat_label", true, 0, 0, 15) &&
        element->getIntAttribute(program_pattern, u"program_pattern", true, 0, 0, 7) &&
        element->getIntAttribute(episode_number, u"episode_number", true, 0, 0, 0x0FFF) &&
        element->getIntAttribute(last_episode_number, u"last_episode_number", true, 0, 0, 0x0FFF) &&
        element->getAttribute(series_name, u"series_name");

    if (ok && element->hasAttribute(u"expire_date")) {
        Time date;
        ok = element->getDateAttribute(date, u"expire_date", true);
        expire_date = date;
    }
    return ok;
}
