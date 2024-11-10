//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCLabelDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_label_descriptor"
#define MY_CLASS    ts::DSMCCLabelDescriptor
#define MY_DID      ts::DID_DSMCC_LABEL
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCLabelDescriptor::DSMCCLabelDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCLabelDescriptor::DSMCCLabelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
    deserialize(duck, desc);
}

void ts::DSMCCLabelDescriptor::clearContent()
{
    label.clear();
}


//----------------------------------------------------------------------------
// Binary serialization
//----------------------------------------------------------------------------

void ts::DSMCCLabelDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(label);
}

void ts::DSMCCLabelDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(label);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCLabelDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Module label: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::DSMCCLabelDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"label", label);
}

bool ts::DSMCCLabelDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(label, u"label", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
