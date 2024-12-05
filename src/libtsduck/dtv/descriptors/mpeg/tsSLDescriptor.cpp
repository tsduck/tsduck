//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSLDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SL_descriptor"
#define MY_CLASS    ts::SLDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_SL, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SLDescriptor::SLDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SLDescriptor::clearContent()
{
    ES_ID = 0;
}

ts::SLDescriptor::SLDescriptor(DuckContext& duck, const Descriptor& desc) :
    SLDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::SLDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(ES_ID);
}

void ts::SLDescriptor::deserializePayload(PSIBuffer& buf)
{
    ES_ID = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SLDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"ES id: %n", buf.getUInt16()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::SLDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"ES_ID", ES_ID, true);
}

bool ts::SLDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(ES_ID, u"ES_ID", true);
}
