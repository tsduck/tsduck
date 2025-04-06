//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCContentTypeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_content_type_descriptor"
#define MY_CLASS    ts::DSMCCContentTypeDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_DSMCC_CONTENT_TYPE, ts::Standards::DVB, ts::TID_DSMCC_UNM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCContentTypeDescriptor::DSMCCContentTypeDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DSMCCContentTypeDescriptor::DSMCCContentTypeDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCContentTypeDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCContentTypeDescriptor::clearContent()
{
    content_type.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCContentTypeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(content_type);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCContentTypeDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(content_type);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCContentTypeDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Content type: " << buf.getString() << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCContentTypeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"content_type", content_type);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCContentTypeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(content_type, u"content_type");
}
