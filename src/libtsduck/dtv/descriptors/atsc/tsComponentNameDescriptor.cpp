//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsComponentNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"component_name_descriptor"
#define MY_CLASS    ts::ComponentNameDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_COMPONENT_NAME, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ComponentNameDescriptor::ComponentNameDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ComponentNameDescriptor::clearContent()
{
    component_name_string.clear();
}

ts::ComponentNameDescriptor::ComponentNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    ComponentNameDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putMultipleString(component_name_string);
}

void ts::ComponentNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getMultipleString(component_name_string);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayATSCMultipleString(buf, 0, margin, u"Component name: ");
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::ComponentNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    component_name_string.toXML(duck, root, u"component_name_string", true);
}

bool ts::ComponentNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return component_name_string.fromXML(duck, element, u"component_name_string", false);
}
