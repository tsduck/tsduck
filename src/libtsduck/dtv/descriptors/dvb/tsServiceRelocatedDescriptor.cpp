//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceRelocatedDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"service_relocated_descriptor"
#define MY_CLASS    ts::ServiceRelocatedDescriptor
#define MY_EDID     ts::EDID::ExtensionDVB(ts::XDID_DVB_SERVICE_RELOCATED)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceRelocatedDescriptor::ServiceRelocatedDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ServiceRelocatedDescriptor::ServiceRelocatedDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceRelocatedDescriptor()
{
    deserialize(duck, desc);
}

void ts::ServiceRelocatedDescriptor::clearContent()
{
    old_original_network_id = 0;
    old_transport_stream_id = 0;
    old_service_id = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceRelocatedDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(old_original_network_id);
    buf.putUInt16(old_transport_stream_id);
    buf.putUInt16(old_service_id);
}

void ts::ServiceRelocatedDescriptor::deserializePayload(PSIBuffer& buf)
{
    old_original_network_id = buf.getUInt16();
    old_transport_stream_id = buf.getUInt16();
    old_service_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceRelocatedDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"Old original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Old transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Old service id: %n", buf.getUInt16()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceRelocatedDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"old_original_network_id", old_original_network_id, true);
    root->setIntAttribute(u"old_transport_stream_id", old_transport_stream_id, true);
    root->setIntAttribute(u"old_service_id", old_service_id, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceRelocatedDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(old_original_network_id, u"old_original_network_id", true) &&
           element->getIntAttribute(old_transport_stream_id, u"old_transport_stream_id", true) &&
           element->getIntAttribute(old_service_id, u"old_service_id", true);
}
