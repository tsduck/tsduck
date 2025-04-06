//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCGroupLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_group_link_descriptor"
#define MY_CLASS    ts::DSMCCGroupLinkDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_DSMCC_GROUP_LINK, ts::Standards::DVB, ts::TID_DSMCC_UNM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCGroupLinkDescriptor::DSMCCGroupLinkDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DSMCCGroupLinkDescriptor::DSMCCGroupLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCGroupLinkDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCGroupLinkDescriptor::clearContent()
{
    position = 0;
    group_id = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCGroupLinkDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        const uint8_t  position = buf.getUInt8();
        const uint32_t group_id = buf.getUInt32();
        disp << margin << "Position: " << DataName(MY_XML_NAME, u"position", position, NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Group Id: %n", group_id) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCGroupLinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(position);
    buf.putUInt32(group_id);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCGroupLinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    position = buf.getUInt8();
    group_id = buf.getUInt32();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCGroupLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"position", position, true);
    root->setIntAttribute(u"group_id", group_id, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCGroupLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(position, u"position", true) &&
           element->getIntAttribute(group_id, u"group_id", true);
}
