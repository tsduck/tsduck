//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDiscontinuityInformationTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"discontinuity_information_table"
#define MY_CLASS ts::DiscontinuityInformationTable
#define MY_TID ts::TID_DIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DiscontinuityInformationTable::DiscontinuityInformationTable(bool tr) :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    transition(tr)
{
}

ts::DiscontinuityInformationTable::DiscontinuityInformationTable(DuckContext& duck, const BinaryTable& table) :
    DiscontinuityInformationTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::clearContent()
{
    transition = false;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    transition = buf.getBool();
    buf.skipReservedBits(7);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putBit(transition);
    buf.putBits(0xFF, 7);
}


//----------------------------------------------------------------------------
// A static method to display a DiscontinuityInformationTable section.
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Transition: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipReservedBits(7);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DiscontinuityInformationTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"transition", transition);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DiscontinuityInformationTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(transition, u"transition", true);
}
