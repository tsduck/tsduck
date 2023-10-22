//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#define MY_CLASS ts::ComponentNameDescriptor
#define MY_DID ts::DID_ATSC_COMPONENT_NAME
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ComponentNameDescriptor::ComponentNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ComponentNameDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
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
