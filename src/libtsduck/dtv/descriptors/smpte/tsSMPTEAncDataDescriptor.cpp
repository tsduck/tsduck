//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSMPTEAncDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"smpte_anc_data_descriptor"
#define MY_CLASS    ts::SMPTEAncDataDescriptor
#define MY_EDID     ts::EDID::PrivateMPEG(ts::DID_SMPTE_ANC_DATA, ts::REGID_VANC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SMPTEAncDataDescriptor::SMPTEAncDataDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::SMPTEAncDataDescriptor::SMPTEAncDataDescriptor(DuckContext& duck, const Descriptor& desc) :
    SMPTEAncDataDescriptor()
{
    deserialize(duck, desc);
}

void ts::SMPTEAncDataDescriptor::clearContent()
{
    descriptor.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SMPTEAncDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(descriptor);
}

void ts::SMPTEAncDataDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(descriptor);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SMPTEAncDataDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayPrivateData(u"descriptor", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SMPTEAncDataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaText(descriptor, true);
}

bool ts::SMPTEAncDataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaText(descriptor, 0, MAX_DESCRIPTOR_SIZE - 2);
}
