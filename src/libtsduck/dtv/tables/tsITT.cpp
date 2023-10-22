//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsITT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ITT"
#define MY_CLASS ts::ITT
#define MY_TID ts::TID_ITT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::ITT::ITT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    descs(this)
{
}

ts::ITT::ITT(const ITT& other) :
    AbstractLongTable(other),
    event_id(other.event_id),
    descs(this, other.descs)
{
}

ts::ITT::ITT(DuckContext& duck, const BinaryTable& table) :
    ITT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ITT::tableIdExtension() const
{
    return event_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ITT::clearContent()
{
    event_id = 0;
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ITT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    event_id = section.tableIdExtension();
    buf.getDescriptorListWithLength(descs);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ITT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    size_t start_index = 0;
    do {
        start_index = buf.putPartialDescriptorListWithLength(descs, start_index);
        addOneSection(table, buf);
    } while (start_index < descs.count());
}


//----------------------------------------------------------------------------
// A static method to display an ITT section.
//----------------------------------------------------------------------------

void ts::ITT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Event id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;
    disp.displayDescriptorListWithLength(section, buf, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ITT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"event_id", event_id, true);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ITT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(is_current, u"current", false, true) &&
           element->getIntAttribute(event_id, u"event_id", true) &&
           descs.fromXML(duck, element);
}
