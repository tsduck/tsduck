//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceAvailDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"splice_avail_descriptor"
#define MY_CLASS ts::SpliceAvailDescriptor
#define MY_DID ts::DID_SPLICE_AVAIL
#define MY_TID ts::TID_SCTE35_SIT
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceAvailDescriptor::SpliceAvailDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SpliceAvailDescriptor::clearContent()
{
    identifier = SPLICE_ID_CUEI;
    provider_avail_id = 0;
}

ts::SpliceAvailDescriptor::SpliceAvailDescriptor(DuckContext& duck, const Descriptor& desc) :
    SpliceAvailDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::SpliceAvailDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(identifier);
    buf.putUInt32(provider_avail_id);
}

void ts::SpliceAvailDescriptor::deserializePayload(PSIBuffer& buf)
{
    identifier = buf.getUInt32();
    provider_avail_id = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceAvailDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        // Sometimes, the identifiers are made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Identifier: 0x%08X", buf, 4, margin);
        disp.displayIntAndASCII(u"Provider id: 0x%08X", buf, 4, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::SpliceAvailDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"provider_avail_id", provider_avail_id, true);
}

bool ts::SpliceAvailDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
           element->getIntAttribute(provider_avail_id, u"provider_avail_id", true);
}
