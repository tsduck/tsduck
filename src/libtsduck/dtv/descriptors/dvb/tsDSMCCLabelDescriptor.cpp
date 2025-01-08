//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
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
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_DSMCC_LABEL, ts::Standards::DVB, ts::TID_DSMCC_UNM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCLabelDescriptor::DSMCCLabelDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DSMCCLabelDescriptor::DSMCCLabelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
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

void ts::DSMCCLabelDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
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
