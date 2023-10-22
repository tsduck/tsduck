//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIBPDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"IBP_descriptor"
#define MY_CLASS ts::IBPDescriptor
#define MY_DID ts::DID_IBP
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IBPDescriptor::IBPDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::IBPDescriptor::IBPDescriptor(DuckContext& duck, const Descriptor& desc) :
    IBPDescriptor()
{
    deserialize(duck, desc);
}

void ts::IBPDescriptor::clearContent()
{
    closed_gop = false;
    identical_gop = false;
    max_gop_length = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IBPDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(closed_gop);
    buf.putBit(identical_gop);
    buf.putBits(max_gop_length, 14);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::IBPDescriptor::deserializePayload(PSIBuffer& buf)
{
    closed_gop = buf.getBool();
    identical_gop = buf.getBool();
    buf.getBits(max_gop_length, 14);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IBPDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Closed GOP: %s", {buf.getBool()});
        disp << UString::Format(u", identical GOP: %s", {buf.getBool()});
        disp << UString::Format(u", max GOP length: 0x%X (%<'d)", {buf.getBits<uint16_t>(14)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IBPDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"closed_gop", closed_gop);
    root->setBoolAttribute(u"identical_gop", identical_gop);
    root->setIntAttribute(u"max_gop_length", max_gop_length, false);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::IBPDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(closed_gop, u"closed_gop", true) &&
           element->getBoolAttribute(identical_gop, u"identical_gop", true) &&
           element->getIntAttribute(max_gop_length, u"max_gop_length", true, 0, 0x0001, 0x3FFF);
}
