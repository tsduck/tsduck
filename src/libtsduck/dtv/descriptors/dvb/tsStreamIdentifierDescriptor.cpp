//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStreamIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"stream_identifier_descriptor"
#define MY_CLASS    ts::StreamIdentifierDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_STREAM_ID, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StreamIdentifierDescriptor::StreamIdentifierDescriptor(uint8_t ctag) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    component_tag(ctag)
{
}

void ts::StreamIdentifierDescriptor::clearContent()
{
    component_tag = 0;
}

ts::StreamIdentifierDescriptor::StreamIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    component_tag(0)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(component_tag);
}

void ts::StreamIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    component_tag = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::StreamIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"component_tag", component_tag, true);
}

bool ts::StreamIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(component_tag, u"component_tag", true);
}
