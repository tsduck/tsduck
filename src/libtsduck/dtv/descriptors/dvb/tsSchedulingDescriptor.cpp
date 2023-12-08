//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSchedulingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"

#define MY_XML_NAME u"scheduling_descriptor"
#define MY_CLASS ts::SchedulingDescriptor
#define MY_DID ts::DID_UNT_SCHEDULING
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

const ts::Enumeration ts::SchedulingDescriptor::SchedulingUnitNames({
    {u"second", 0},
    {u"minute", 1},
    {u"hour",   2},
    {u"day",    3},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SchedulingDescriptor::SchedulingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SchedulingDescriptor::clearContent()
{
    start_date_time.clear();
    end_date_time.clear();
    final_availability = false;
    periodicity = false;
    period_unit = 0;
    duration_unit = 0;
    estimated_cycle_time_unit = 0;
    period = 0;
    duration = 0;
    estimated_cycle_time = 0;
    private_data.clear();
}

ts::SchedulingDescriptor::SchedulingDescriptor(DuckContext& duck, const Descriptor& desc) :
    SchedulingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putMJD(start_date_time, MJD_SIZE);
    buf.putMJD(end_date_time, MJD_SIZE);
    buf.putBit(final_availability);
    buf.putBit(periodicity);
    buf.putBits(period_unit, 2);
    buf.putBits(duration_unit, 2);
    buf.putBits(estimated_cycle_time_unit, 2);
    buf.putUInt8(period);
    buf.putUInt8(duration);
    buf.putUInt8(estimated_cycle_time);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::deserializePayload(PSIBuffer& buf)
{
    start_date_time = buf.getMJD(MJD_SIZE);
    end_date_time = buf.getMJD(MJD_SIZE);
    final_availability = buf.getBool();
    periodicity = buf.getBool();
    buf.getBits(period_unit, 2);
    buf.getBits(duration_unit, 2);
    buf.getBits(estimated_cycle_time_unit, 2);
    period = buf.getUInt8();
    duration = buf.getUInt8();
    estimated_cycle_time = buf.getUInt8();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(14)) {
        disp << margin << "Start time: " << buf.getMJD(MJD_SIZE).format(Time::DATETIME) << std::endl;
        disp << margin << "End time:   " << buf.getMJD(MJD_SIZE).format(Time::DATETIME) << std::endl;
        disp << margin << UString::Format(u"Final availability: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Periodicity: %s", {buf.getBool()}) << std::endl;
        const uint8_t period_unit = buf.getBits<uint8_t>(2);
        const uint8_t duration_unit = buf.getBits<uint8_t>(2);
        const uint8_t cycle_unit = buf.getBits<uint8_t>(2);
        disp << margin << UString::Format(u"Period: %d %ss", {buf.getUInt8(), SchedulingUnitNames.name(period_unit)}) << std::endl;
        disp << margin << UString::Format(u"Duration: %d %ss", {buf.getUInt8(), SchedulingUnitNames.name(duration_unit)}) << std::endl;
        disp << margin << UString::Format(u"Estimated cycle time: %d %ss", {buf.getUInt8(), SchedulingUnitNames.name(cycle_unit)}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setDateTimeAttribute(u"start_date_time", start_date_time);
    root->setDateTimeAttribute(u"end_date_time", end_date_time);
    root->setBoolAttribute(u"final_availability", final_availability);
    root->setBoolAttribute(u"periodicity", periodicity);
    root->setIntEnumAttribute(SchedulingUnitNames, u"period_unit", period_unit);
    root->setIntEnumAttribute(SchedulingUnitNames, u"duration_unit", duration_unit);
    root->setIntEnumAttribute(SchedulingUnitNames, u"estimated_cycle_time_unit", estimated_cycle_time_unit);
    root->setIntAttribute(u"period", period);
    root->setIntAttribute(u"duration", duration);
    root->setIntAttribute(u"estimated_cycle_time", estimated_cycle_time);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SchedulingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getDateTimeAttribute(start_date_time, u"start_date_time", true) &&
            element->getDateTimeAttribute(end_date_time, u"end_date_time", true) &&
            element->getBoolAttribute(final_availability, u"final_availability", true) &&
            element->getBoolAttribute(periodicity, u"periodicity", true) &&
            element->getIntEnumAttribute(period_unit, SchedulingUnitNames, u"period_unit", true) &&
            element->getIntEnumAttribute(duration_unit, SchedulingUnitNames, u"duration_unit", true) &&
            element->getIntEnumAttribute(estimated_cycle_time_unit, SchedulingUnitNames, u"estimated_cycle_time_unit", true) &&
            element->getIntAttribute(period, u"period", true) &&
            element->getIntAttribute(duration, u"duration", true) &&
            element->getIntAttribute(estimated_cycle_time, u"estimated_cycle_time", true) &&
            element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 16);
}
