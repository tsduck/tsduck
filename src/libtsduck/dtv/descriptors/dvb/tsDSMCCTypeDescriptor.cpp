//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCTypeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_type_descriptor"
#define MY_CLASS    ts::DSMCCTypeDescriptor
#define MY_DID      ts::DID_DSMCC_TYPE
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCTypeDescriptor::DSMCCTypeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCTypeDescriptor::DSMCCTypeDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
    deserialize(duck, desc);
}

void ts::DSMCCTypeDescriptor::clearContent()
{
    type.clear();
}


//----------------------------------------------------------------------------
// Binary serialization
//----------------------------------------------------------------------------

void ts::DSMCCTypeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(type);
}

void ts::DSMCCTypeDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(type);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCTypeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Module or Group Type: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::DSMCCTypeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"type", type);
}

bool ts::DSMCCTypeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(type, u"type", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
