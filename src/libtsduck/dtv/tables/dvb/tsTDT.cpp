//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTDT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TDT"
#define MY_CLASS ts::TDT
#define MY_TID ts::TID_TDT
#define MY_PID ts::PID_TDT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TDT::TDT(const Time& utc_time_) :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    utc_time(utc_time_)
{
}

ts::TDT::TDT(DuckContext& duck, const BinaryTable& table) :
    TDT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::TDT::clearContent()
{
    utc_time.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TDT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get UTC time. The time reference is UTC as defined by DVB, but can be non-standard.
    utc_time = buf.getFullMJD() - buf.duck().timeReferenceOffset();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TDT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Encode the data in MJD in the payload. Defined as UTC by DVB, but can be non-standard.
    buf.putFullMJD(utc_time + buf.duck().timeReferenceOffset());
}


//----------------------------------------------------------------------------
// A static method to display a TDT section.
//----------------------------------------------------------------------------

void ts::TDT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(5)) {
        // The time reference is UTC as defined by DVB, but can be non-standard.
        const UString zone(disp.duck().timeReferenceName());
        const MilliSecond offset = disp.duck().timeReferenceOffset();
        const Time time(buf.getFullMJD());

        disp << margin << zone << " time: " << time.format(Time::DATETIME);
        if (offset != 0) {
            disp << " (UTC: " << (time - offset).format(Time::DATETIME) << ")";
        }
        disp << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TDT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setDateTimeAttribute(u"UTC_time", utc_time);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TDT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getDateTimeAttribute(utc_time, u"UTC_time", true);
}
