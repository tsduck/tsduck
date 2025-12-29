//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRedistributionControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"redistribution_control_descriptor"
#define MY_CLASS    ts::RedistributionControlDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_REDIST_CONTROL, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RedistributionControlDescriptor::RedistributionControlDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::RedistributionControlDescriptor::RedistributionControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    RedistributionControlDescriptor()
{
    deserialize(duck, desc);
}

void ts::RedistributionControlDescriptor::clearContent()
{
    rc_information.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RedistributionControlDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(rc_information);
}

void ts::RedistributionControlDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(rc_information);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RedistributionControlDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayPrivateData(u"RC information", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RedistributionControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaTextChild(u"rc_information", rc_information, true);
}

bool ts::RedistributionControlDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(rc_information, u"rc_information", false, 0, 255);
}
