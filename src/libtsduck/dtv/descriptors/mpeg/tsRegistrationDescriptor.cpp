//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRegistrationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"registration_descriptor"
#define MY_CLASS ts::RegistrationDescriptor
#define MY_DID ts::DID_REGISTRATION
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RegistrationDescriptor::RegistrationDescriptor(uint32_t identifier, const ByteBlock& info) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    format_identifier(identifier),
    additional_identification_info(info)
{
}

void ts::RegistrationDescriptor::clearContent()
{
    format_identifier = 0;
    additional_identification_info.clear();
}

ts::RegistrationDescriptor::RegistrationDescriptor(DuckContext& duck, const Descriptor& desc) :
    RegistrationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(format_identifier);
    buf.putBytes(additional_identification_info);
}

void ts::RegistrationDescriptor::deserializePayload(PSIBuffer& buf)
{
    format_identifier = buf.getUInt32();
    buf.getBytes(additional_identification_info);

    // Keep track of last registration id.
    buf.duck().addRegistrationId(format_identifier);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        // Sometimes, the registration format identifier is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Format identifier: 0x%08X", buf, 4, margin);
        disp.displayPrivateData(u"Additional identification info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"format_identifier", format_identifier, true);
    root->addHexaTextChild(u"additional_identification_info", additional_identification_info, true);
}

bool ts::RegistrationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    const bool ok =
        element->getIntAttribute(format_identifier, u"format_identifier", true) &&
        element->getHexaTextChild(additional_identification_info, u"additional_identification_info", false, 0, MAX_DESCRIPTOR_SIZE - 6);

    // Keep track of last registration id.
    if (ok) {
        duck.addRegistrationId(format_identifier);
    }

    return ok;
}
