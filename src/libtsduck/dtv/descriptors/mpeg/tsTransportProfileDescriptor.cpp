//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTransportProfileDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"transport_profile_descriptor"
#define MY_CLASS    ts::TransportProfileDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_TRANSPORT_PROFILE, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TransportProfileDescriptor::TransportProfileDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::TransportProfileDescriptor::TransportProfileDescriptor(DuckContext& duck, const Descriptor& desc) :
    TransportProfileDescriptor()
{
    deserialize(duck, desc);
}

void ts::TransportProfileDescriptor::clearContent()
{
    transport_profile = 0;
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TransportProfileDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(transport_profile);
    buf.putBytes(private_data);
}

void ts::TransportProfileDescriptor::deserializePayload(PSIBuffer& buf)
{
    transport_profile = buf.getUInt8();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TransportProfileDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Transport profile: " << DataName(MY_XML_NAME, u"Profile", buf.getUInt8(), NamesFlags::HEXA_FIRST) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TransportProfileDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_profile", transport_profile, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::TransportProfileDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(transport_profile, u"transport_profile", true) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}
