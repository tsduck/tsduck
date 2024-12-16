//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsXAITLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"xait_location_descriptor"
#define MY_CLASS    ts::XAITLocationDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_XAIT_LOCATION, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::XAITLocationDescriptor::XAITLocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::XAITLocationDescriptor::XAITLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    XAITLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::XAITLocationDescriptor::clearContent()
{
    xait_original_network_id = 0;
    xait_service_id = 0;
    xait_version_number = 0;
    xait_update_policy = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::XAITLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(xait_original_network_id);
    buf.putUInt16(xait_service_id);
    buf.putBits(xait_version_number, 5);
    buf.putBits(xait_update_policy, 3);
}

void ts::XAITLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    xait_original_network_id = buf.getUInt16();
    xait_service_id = buf.getUInt16();
    buf.getBits(xait_version_number, 5);
    buf.getBits(xait_update_policy, 3);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::XAITLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"Original network id: %n", buf.getUInt16());
        disp << UString::Format(u", service id: %n", buf.getUInt16()) << std::endl;
        disp << margin << "Version number: " << buf.getBits<uint16_t>(5);
        disp << u", update policy: " << DataName(MY_XML_NAME, u"update_policy", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::XAITLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"xait_original_network_id", xait_original_network_id, true);
    root->setIntAttribute(u"xait_service_id", xait_service_id, true);
    root->setIntAttribute(u"xait_version_number", xait_version_number);
    root->setIntAttribute(u"xait_update_policy", xait_update_policy);
}

bool ts::XAITLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(xait_original_network_id, u"xait_original_network_id", true) &&
           element->getIntAttribute(xait_service_id, u"xait_service_id", true) &&
           element->getIntAttribute(xait_version_number, u"xait_version_number", true, 0, 0, 0x1F) &&
           element->getIntAttribute(xait_update_policy, u"xait_update_policy", true, 0, 0, 0x07);
}
