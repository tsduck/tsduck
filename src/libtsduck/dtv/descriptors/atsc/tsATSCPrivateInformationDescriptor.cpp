//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCPrivateInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_private_information_descriptor"
#define MY_CLASS    ts::ATSCPrivateInformationDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_PRIVATE_INFO, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCPrivateInformationDescriptor::ATSCPrivateInformationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCPrivateInformationDescriptor::clearContent()
{
    format_identifier = 0;
    private_data.clear();
}

ts::ATSCPrivateInformationDescriptor::ATSCPrivateInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCPrivateInformationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCPrivateInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(format_identifier);
    buf.putBytes(private_data);
}

void ts::ATSCPrivateInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    format_identifier = buf.getUInt32();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCPrivateInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        // Sometimes, the format identifier is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Format identifier: 0x%08X", buf, 4, margin);
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCPrivateInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"format_identifier", format_identifier, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::ATSCPrivateInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(format_identifier, u"format_identifier", true) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}
