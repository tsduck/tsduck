//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCSubgroupAssociationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_subgroup_association_descriptor"
#define MY_CLASS    ts::DSMCCSubgroupAssociationDescriptor
#define MY_DID      ts::DID_DSMCC_SUBGROUP_ASSOCIATION
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCSubgroupAssociationDescriptor::DSMCCSubgroupAssociationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCSubgroupAssociationDescriptor::DSMCCSubgroupAssociationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCSubgroupAssociationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCSubgroupAssociationDescriptor::clearContent()
{
    subgroup_tag = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCSubgroupAssociationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt40(subgroup_tag);
}

void ts::DSMCCSubgroupAssociationDescriptor::deserializePayload(PSIBuffer& buf)
{
    subgroup_tag = buf.getUInt40();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCSubgroupAssociationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBits(40)) {
        disp << margin << UString::Format(u"Subgroup tag: %n", buf.getUInt40()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCSubgroupAssociationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"subgroup_tag", subgroup_tag, true);
}

bool ts::DSMCCSubgroupAssociationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(subgroup_tag, u"subgroup_tag", true, 0, 0, 0x000000FFFFFFFFFF);
}
