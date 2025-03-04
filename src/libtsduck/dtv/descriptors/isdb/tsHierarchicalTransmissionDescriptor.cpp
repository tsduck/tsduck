//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHierarchicalTransmissionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"hierarchical_transmission_descriptor"
#define MY_CLASS    ts::HierarchicalTransmissionDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_HIERARCH_TRANS, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HierarchicalTransmissionDescriptor::HierarchicalTransmissionDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::HierarchicalTransmissionDescriptor::clearContent()
{
    high_quality = false;
    reference_PID = PID_NULL;
}

ts::HierarchicalTransmissionDescriptor::HierarchicalTransmissionDescriptor(DuckContext& duck, const Descriptor& desc) :
    HierarchicalTransmissionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 7);
    buf.putBit(high_quality);
    buf.putPID(reference_PID);
}

void ts::HierarchicalTransmissionDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(7);
    high_quality = buf.getBool();
    reference_PID = buf.getPID();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        buf.skipBits(7);
        disp << margin << "Quality level: " << (buf.getBool() ? u"high" : u"low") << std::endl;
        disp << margin << UString::Format(u"Reference PID: %n", buf.getPID()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HierarchicalTransmissionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"high_quality", high_quality);
    root->setIntAttribute(u"reference_PID", reference_PID, true);
}

bool ts::HierarchicalTransmissionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(high_quality, u"high_quality", true) &&
           element->getIntAttribute<PID>(reference_PID, u"reference_PID", true, 0, 0, 0x1FFF);
}
