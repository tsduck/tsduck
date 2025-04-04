//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDVB.h"

#define MY_XML_NAME u"service_descriptor"
#define MY_CLASS    ts::ServiceDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_SERVICE, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceDescriptor::ServiceDescriptor(uint8_t type, const UString& provider, const UString& name) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    service_type(type),
    provider_name(provider),
    service_name(name)
{
}

void ts::ServiceDescriptor::clearContent()
{
    service_type = 0;
    provider_name.clear();
    service_name.clear();
}

ts::ServiceDescriptor::ServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceDescriptor()
{
    deserialize(duck, desc);
}

ts::DescriptorDuplication ts::ServiceDescriptor::duplicationMode() const
{
    return DescriptorDuplication::REPLACE;
}


//----------------------------------------------------------------------------
// Binary serialization
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(service_type);
    buf.putStringWithByteLength(provider_name);
    buf.putStringWithByteLength(service_name);
}

void ts::ServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    service_type = buf.getUInt8();
    buf.getStringWithByteLength(provider_name);
    buf.getStringWithByteLength(service_name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Service type: " << ServiceTypeName(buf.getUInt8(), NamesFlags::VALUE_NAME) << std::endl;
        const UString provider(buf.getStringWithByteLength());
        const UString service(buf.getStringWithByteLength());
        disp << margin << "Service: \"" << service << "\", Provider: \"" << provider << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"service_type", service_type, true);
    root->setAttribute(u"service_provider_name", provider_name);
    root->setAttribute(u"service_name", service_name);
}

bool ts::ServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(service_type, u"service_type", true) &&
           element->getAttribute(provider_name, u"service_provider_name", true) &&
           element->getAttribute(service_name, u"service_name", true);
}
