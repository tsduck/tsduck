//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceMoveDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"service_move_descriptor"
#define MY_CLASS    ts::ServiceMoveDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_SERVICE_MOVE, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceMoveDescriptor::ServiceMoveDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ServiceMoveDescriptor::ServiceMoveDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceMoveDescriptor()
{
    deserialize(duck, desc);
}

void ts::ServiceMoveDescriptor::clearContent()
{
    new_original_network_id = 0;
    new_transport_stream_id = 0;
    new_service_id = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceMoveDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(new_original_network_id);
    buf.putUInt16(new_transport_stream_id);
    buf.putUInt16(new_service_id);
}


void ts::ServiceMoveDescriptor::deserializePayload(PSIBuffer& buf)
{
    new_original_network_id = buf.getUInt16();
    new_transport_stream_id = buf.getUInt16();
    new_service_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceMoveDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"New original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"New transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"New service id: %n", buf.getUInt16()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceMoveDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"new_original_network_id", new_original_network_id, true);
    root->setIntAttribute(u"new_transport_stream_id", new_transport_stream_id, true);
    root->setIntAttribute(u"new_service_id", new_service_id, true);
}

bool ts::ServiceMoveDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(new_original_network_id, u"new_original_network_id", true) &&
           element->getIntAttribute(new_transport_stream_id, u"new_transport_stream_id", true) &&
           element->getIntAttribute(new_service_id, u"new_service_id", true);
}
