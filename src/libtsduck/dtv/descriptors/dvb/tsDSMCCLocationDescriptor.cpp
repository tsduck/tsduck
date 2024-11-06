//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_location_descriptor"
#define MY_CLASS    ts::DSMCCLocationDescriptor
#define MY_DID      ts::DID_DSMCC_LOCATION
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCLocationDescriptor::DSMCCLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCLocationDescriptor::DSMCCLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCLocationDescriptor::clearContent()
{
    location_tag = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Location Tag: %n", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(location_tag);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    location_tag = buf.getUInt8();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"location_tag", location_tag, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(location_tag, u"location_tag", true);
}
