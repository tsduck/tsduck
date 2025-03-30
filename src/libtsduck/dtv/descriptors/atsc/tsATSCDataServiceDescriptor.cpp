//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCDataServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_data_service_descriptor"
#define MY_CLASS    ts::ATSCDataServiceDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_DATA_SERVICE, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCDataServiceDescriptor::ATSCDataServiceDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCDataServiceDescriptor::clearContent()
{
    data_service_profile = 0;
    data_service_level = 0;
    private_data.clear();
}

ts::ATSCDataServiceDescriptor::ATSCDataServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCDataServiceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCDataServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(data_service_profile);
    buf.putUInt8(data_service_level);
    buf.putUInt8(uint8_t(private_data.size()));
    buf.putBytes(private_data);
}

void ts::ATSCDataServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_service_profile = buf.getUInt8();
    data_service_level = buf.getUInt8();
    buf.getBytes(private_data, buf.getUInt8());
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCDataServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Data service profile: " << DataName(MY_XML_NAME, u"profile", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << "Data service level: " << DataName(MY_XML_NAME, u"level", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp.displayPrivateData(u"Private data", buf, buf.getUInt8(), margin);
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCDataServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_service_profile", data_service_profile, true);
    root->setIntAttribute(u"data_service_level", data_service_level, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::ATSCDataServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(data_service_profile, u"data_service_profile", true) &&
           element->getIntAttribute(data_service_level, u"data_service_level", true) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 5);
}
