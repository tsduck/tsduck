//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCStuffingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_stuffing_descriptor"
#define MY_CLASS    ts::ATSCStuffingDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_STUFFING, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCStuffingDescriptor::ATSCStuffingDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCStuffingDescriptor::clearContent()
{
    stuffing.clear();
}

ts::ATSCStuffingDescriptor::ATSCStuffingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCStuffingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(stuffing);
}

void ts::ATSCStuffingDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(stuffing);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayPrivateData(u"Stuffing data", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaText(stuffing, true);
}

bool ts::ATSCStuffingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaText(stuffing, 0, 255);
}
