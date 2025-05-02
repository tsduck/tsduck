//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAstraVirtualServiceIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"astra_virtual_service_id_descriptor"
#define MY_CLASS    ts::AstraVirtualServiceIdDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_ASTRA_VIRTUAL_SERVICE_ID, ts::PDS_ASTRA)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AstraVirtualServiceIdDescriptor::AstraVirtualServiceIdDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::AstraVirtualServiceIdDescriptor::clearContent()
{
    virtual_service_id = 0;
    reserved.clear();
}

ts::AstraVirtualServiceIdDescriptor::AstraVirtualServiceIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    AstraVirtualServiceIdDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AstraVirtualServiceIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(virtual_service_id);
    buf.putBytes(reserved);
}

void ts::AstraVirtualServiceIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    virtual_service_id = buf.getUInt16();
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AstraVirtualServiceIdDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Virtual service id: %n", buf.getUInt16()) << std::endl;
        disp.displayPrivateData(u"Reserved", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AstraVirtualServiceIdDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"virtual_service_id", virtual_service_id, true);
    root->addHexaTextChild(u"reserved", reserved, true);
}

bool ts::AstraVirtualServiceIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(virtual_service_id, u"virtual_service_id", true) &&
           element->getHexaTextChild(reserved, u"reserved", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}
