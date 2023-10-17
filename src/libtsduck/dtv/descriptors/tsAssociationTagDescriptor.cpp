//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAssociationTagDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"association_tag_descriptor"
#define MY_CLASS ts::AssociationTagDescriptor
#define MY_DID ts::DID_ASSOCIATION_TAG
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AssociationTagDescriptor::AssociationTagDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::AssociationTagDescriptor::clearContent()
{
    association_tag = 0;
    use = 0;
    selector_bytes.clear();
    private_data.clear();
}

ts::AssociationTagDescriptor::AssociationTagDescriptor(DuckContext& duck, const Descriptor& desc) :
    AssociationTagDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(association_tag);
    buf.putUInt16(use);
    buf.putUInt8(uint8_t(selector_bytes.size()));
    buf.putBytes(selector_bytes);
    buf.putBytes(private_data);
}

void ts::AssociationTagDescriptor::deserializePayload(PSIBuffer& buf)
{
    association_tag = buf.getUInt16();
    use = buf.getUInt16();
    buf.getBytes(selector_bytes, buf.getUInt8());
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"Association tag: 0x%X (%<d)", {buf.getUInt16()});
        disp << UString::Format(u", use: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp.displayPrivateData(u"Selector bytes", buf, buf.getUInt8(), margin);
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"association_tag", association_tag, true);
    root->setIntAttribute(u"use", use, true);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::AssociationTagDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(association_tag, u"association_tag", true) &&
           element->getIntAttribute(use, u"use", true) &&
           element->getHexaTextChild(selector_bytes, u"selector_bytes", false) &&
           element->getHexaTextChild(private_data, u"private_data", false);
}
