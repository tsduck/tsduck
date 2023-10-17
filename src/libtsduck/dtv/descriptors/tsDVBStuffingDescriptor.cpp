//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBStuffingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DVB_stuffing_descriptor"
#define MY_XML_NAME_LEGACY u"stuffing_descriptor"
#define MY_CLASS ts::DVBStuffingDescriptor
#define MY_DID ts::DID_STUFFING
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBStuffingDescriptor::DVBStuffingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY)
{
}

ts::DVBStuffingDescriptor::DVBStuffingDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBStuffingDescriptor()
{
    deserialize(duck, desc);
}

void ts::DVBStuffingDescriptor::clearContent()
{
    stuffing.clear();
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::DVBStuffingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(stuffing);
}

void ts::DVBStuffingDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(stuffing);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBStuffingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp.displayPrivateData(u"Stuffing data", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::DVBStuffingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaText(stuffing, true);
}

bool ts::DVBStuffingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaText(stuffing, 0, 255);
}
