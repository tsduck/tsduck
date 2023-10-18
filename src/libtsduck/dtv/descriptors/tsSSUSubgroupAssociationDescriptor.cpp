//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUSubgroupAssociationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_subgroup_association_descriptor"
#define MY_CLASS ts::SSUSubgroupAssociationDescriptor
#define MY_DID ts::DID_UNT_SUBGROUP_ASSOC
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUSubgroupAssociationDescriptor::SSUSubgroupAssociationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SSUSubgroupAssociationDescriptor::SSUSubgroupAssociationDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUSubgroupAssociationDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSUSubgroupAssociationDescriptor::clearContent()
{
    subgroup_tag = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUSubgroupAssociationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt40(subgroup_tag);
}

void ts::SSUSubgroupAssociationDescriptor::deserializePayload(PSIBuffer& buf)
{
    subgroup_tag = buf.getUInt40();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUSubgroupAssociationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBits(40)) {
        disp << margin << UString::Format(u"Subgroup tag: 0x%010X (%<d)", {buf.getUInt40()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUSubgroupAssociationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"subgroup_tag", subgroup_tag, true);
}

bool ts::SSUSubgroupAssociationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(subgroup_tag, u"subgroup_tag", true, 0, 0, 0x000000FFFFFFFFFF);
}
