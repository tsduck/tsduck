//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGShortServiceNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dtg_short_service_name_descriptor"
#define MY_CLASS    ts::DTGShortServiceNameDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_OFCOM_SHORT_SRV_NAM, ts::PDS_OFCOM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGShortServiceNameDescriptor::DTGShortServiceNameDescriptor(const UString& name_) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    name(name_)
{
}

ts::DTGShortServiceNameDescriptor::DTGShortServiceNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTGShortServiceNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::DTGShortServiceNameDescriptor::clearContent()
{
    name.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTGShortServiceNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(name);
}

void ts::DTGShortServiceNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTGShortServiceNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Name: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTGShortServiceNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"name", name);
}

bool ts::DTGShortServiceNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(name, u"name", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
