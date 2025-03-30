//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCGroupLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_group_link_descriptor"
#define MY_CLASS    ts::ATSCGroupLinkDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_GROUP_LINK, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCGroupLinkDescriptor::ATSCGroupLinkDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCGroupLinkDescriptor::clearContent()
{
    position = 0;
    group_id = 0;
}

ts::ATSCGroupLinkDescriptor::ATSCGroupLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCGroupLinkDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCGroupLinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(position);
    buf.putUInt32(group_id);
}

void ts::ATSCGroupLinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    position = buf.getUInt8();
    group_id = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCGroupLinkDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        disp << margin << "Position: " << DataName(MY_XML_NAME, u"position", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Group id: %n", buf.getUInt32()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCGroupLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"position", position);
    root->setIntAttribute(u"group_id", group_id, true);
}

bool ts::ATSCGroupLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(position, u"position", true) &&
           element->getIntAttribute(group_id, u"group_id", true);
}
