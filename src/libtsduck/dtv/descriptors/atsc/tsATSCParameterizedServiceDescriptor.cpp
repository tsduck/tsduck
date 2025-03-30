//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCParameterizedServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_parameterized_service_descriptor"
#define MY_CLASS    ts::ATSCParameterizedServiceDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_PARAM_SERVICE, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCParameterizedServiceDescriptor::ATSCParameterizedServiceDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCParameterizedServiceDescriptor::clearContent()
{
    application_tag = 0;
    application_data.clear();
}

ts::ATSCParameterizedServiceDescriptor::ATSCParameterizedServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCParameterizedServiceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCParameterizedServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(application_tag);
    buf.putBytes(application_data);
}

void ts::ATSCParameterizedServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    application_tag = buf.getUInt8();
    buf.getBytes(application_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCParameterizedServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Application tag: %n", buf.getUInt8()) << std::endl;
        disp.displayPrivateData(u"Application data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCParameterizedServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"application_tag", application_tag, true);
    root->addHexaTextChild(u"application_data", application_data, true);
}

bool ts::ATSCParameterizedServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(application_tag, u"application_tag", true) &&
           element->getHexaTextChild(application_data, u"application_data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}
