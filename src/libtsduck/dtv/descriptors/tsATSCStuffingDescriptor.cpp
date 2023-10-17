//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCStuffingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_stuffing_descriptor"
#define MY_CLASS ts::ATSCStuffingDescriptor
#define MY_DID ts::DID_ATSC_STUFFING
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCStuffingDescriptor::ATSCStuffingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ATSCStuffingDescriptor::clearContent()
{
    stuffing.clear();
}

ts::ATSCStuffingDescriptor::ATSCStuffingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCStuffingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(stuffing);
}

void ts::ATSCStuffingDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(stuffing);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp.displayPrivateData(u"Stuffing data", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCStuffingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaText(stuffing, true);
}

bool ts::ATSCStuffingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaText(stuffing, 0, 255);
}
