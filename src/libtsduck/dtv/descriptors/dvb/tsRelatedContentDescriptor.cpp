//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRelatedContentDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"related_content_descriptor"
#define MY_CLASS    ts::RelatedContentDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_RELATED_CONTENT, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RelatedContentDescriptor::RelatedContentDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::RelatedContentDescriptor::RelatedContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    RelatedContentDescriptor()
{
    deserialize(duck, desc);
}

void ts::RelatedContentDescriptor::clearContent()
{
}


//----------------------------------------------------------------------------
// This descriptor is always empty.
//----------------------------------------------------------------------------

void ts::RelatedContentDescriptor::serializePayload(PSIBuffer& buf) const
{
}

void ts::RelatedContentDescriptor::deserializePayload(PSIBuffer& buf)
{
}

void ts::RelatedContentDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
}

void ts::RelatedContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
}

bool ts::RelatedContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return true;
}
