//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsETT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ETT"
#define MY_CLASS ts::ETT
#define MY_TID ts::TID_ETT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ETT::ETT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true) // ETT is always "current"
{
}

ts::ETT::ETT(DuckContext& duck, const BinaryTable& table) :
    ETT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ETT::tableIdExtension() const
{
    return ETT_table_id_extension;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ETT::clearContent()
{
    ETT_table_id_extension = 0;
    protocol_version = 0;
    ETM_id = 0;
    extended_text_message.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ETT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    ETT_table_id_extension = section.tableIdExtension();
    protocol_version = buf.getUInt8();
    ETM_id = buf.getUInt32();
    buf.getMultipleString(extended_text_message);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ETT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Important: an ETT is not allowed to use more than one section, see A/65, section 6.6.
    buf.putUInt8(protocol_version);
    buf.putUInt32(ETM_id);
    buf.putMultipleString(extended_text_message);
}


//----------------------------------------------------------------------------
// A static method to display a ETT section.
//----------------------------------------------------------------------------

void ts::ETT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"ETT table id extension: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()});
        disp << UString::Format(u", ETM id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp.displayATSCMultipleString(buf, 0, margin, u"Extended text message: ");
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ETT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"ETT_table_id_extension", ETT_table_id_extension, true);
    root->setIntAttribute(u"ETM_id", ETM_id, true);
    extended_text_message.toXML(duck, root, u"extended_text_message", true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ETT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
           element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
           element->getIntAttribute(ETT_table_id_extension, u"ETT_table_id_extension", true) &&
           element->getIntAttribute(ETM_id, u"ETM_id", true) &&
           extended_text_message.fromXML(duck, element, u"extended_text_message", false);
}
