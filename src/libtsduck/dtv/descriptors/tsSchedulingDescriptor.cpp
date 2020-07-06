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

#include "tsSchedulingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    start_date_time(),
    end_date_time(),
    final_availability(false),
    periodicity(false),
    period_unit(0),
    duration_unit(0),
    estimated_cycle_time_unit(0),
    period(0),
    duration(0),
    estimated_cycle_time(0),
    private_data()
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

void ts::SchedulingDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    EncodeMJD(start_date_time, bbp->enlarge(MJD_SIZE), MJD_SIZE);
    EncodeMJD(end_date_time, bbp->enlarge(MJD_SIZE), MJD_SIZE);
    bbp->appendUInt8((final_availability ? 0x80 : 0x00) |
                     (periodicity ? 0x40 : 0x00) |
                     uint8_t((period_unit & 0x03) << 4) |
                     uint8_t((duration_unit & 0x03) << 2) |
                     (estimated_cycle_time_unit & 0x03));
    bbp->appendUInt8(period);
    bbp->appendUInt8(duration);
    bbp->appendUInt8(estimated_cycle_time);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 14;
    private_data.clear();

    if (_is_valid) {
        DecodeMJD(data, 5, start_date_time);
        DecodeMJD(data + 5, 5, end_date_time);
        final_availability = (data[10] & 0x80) != 0;
        periodicity = (data[10] & 0x40) != 0;
        period_unit = (data[10] >> 4) & 0x03;
        duration_unit = (data[10] >> 2) & 0x03;
        estimated_cycle_time_unit = data[10] & 0x03;
        period = data[11];
        duration = data[12];
        estimated_cycle_time = data[13];
        private_data.copy(data + 14, size - 14);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SchedulingDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 14) {
        Time start, end;
        DecodeMJD(data, 5, start);
        DecodeMJD(data + 5, 5, end);
        strm << margin << "Start time: " << start.format(Time::DATETIME) << std::endl
             << margin << "End time:   " << end.format(Time::DATETIME) << std::endl
             << margin << UString::Format(u"Final availability: %s", {(data[10] & 0x80) != 0}) << std::endl
             << margin << UString::Format(u"Periodicity: %s", {(data[10] & 0x40) != 0}) << std::endl
             << margin << UString::Format(u"Period: %d %ss", {data[11], SchedulingUnitNames.name((data[10] >> 4) & 0x03)}) << std::endl
             << margin << UString::Format(u"Duration: %d %ss", {data[12], SchedulingUnitNames.name((data[10] >> 2) & 0x03)}) << std::endl
             << margin << UString::Format(u"Estimated cycle time: %d %ss", {data[13], SchedulingUnitNames.name(data[10] & 0x03)}) << std::endl;
        display.displayPrivateData(u"Private data", data + 14, size - 14, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
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
            element->getIntAttribute<uint8_t>(period, u"period", true) &&
            element->getIntAttribute<uint8_t>(duration, u"duration", true) &&
            element->getIntAttribute<uint8_t>(estimated_cycle_time, u"estimated_cycle_time", true) &&
            element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 16);
}
