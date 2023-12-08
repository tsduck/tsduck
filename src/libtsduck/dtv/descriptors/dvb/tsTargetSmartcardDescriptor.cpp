//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetSmartcardDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_smartcard_descriptor"
#define MY_CLASS ts::TargetSmartcardDescriptor
#define MY_DID ts::DID_INT_SMARTCARD
#define MY_STD ts::Standards::DVB

// Table-specific descriptor which is allowed in two distinct tables.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_INT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetSmartcardDescriptor::TargetSmartcardDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TargetSmartcardDescriptor::clearContent()
{
    super_CA_system_id = 0;
    private_data.clear();
}

ts::TargetSmartcardDescriptor::TargetSmartcardDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetSmartcardDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(super_CA_system_id);
    buf.putBytes(private_data);
}

void ts::TargetSmartcardDescriptor::deserializePayload(PSIBuffer& buf)
{
    super_CA_system_id = buf.getUInt32();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Super CAS Id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"super_CA_system_id", super_CA_system_id, true);
    root->addHexaText(private_data, true);
}

bool ts::TargetSmartcardDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(super_CA_system_id, u"super_CA_system_id", true) &&
           element->getHexaText(private_data, 0, MAX_DESCRIPTOR_SIZE - 6);
}
