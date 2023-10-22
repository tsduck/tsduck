//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSimpleApplicationLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"simple_application_location_descriptor"
#define MY_CLASS ts::SimpleApplicationLocationDescriptor
#define MY_DID ts::DID_AIT_APP_LOCATION
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimpleApplicationLocationDescriptor::SimpleApplicationLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SimpleApplicationLocationDescriptor::clearContent()
{
    initial_path.clear();
}

ts::SimpleApplicationLocationDescriptor::SimpleApplicationLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    SimpleApplicationLocationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(initial_path);
}

void ts::SimpleApplicationLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(initial_path);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Initial path: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"initial_path", initial_path);
}

bool ts::SimpleApplicationLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(initial_path, u"initial_path", true, UString(), 0, MAX_DESCRIPTOR_SIZE - 2);
}
