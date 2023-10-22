//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNPTEndpointDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NPT_endpoint_descriptor"
#define MY_CLASS ts::NPTEndpointDescriptor
#define MY_DID ts::DID_NPT_ENDPOINT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NPTEndpointDescriptor::NPTEndpointDescriptor(uint64_t start, uint64_t stop) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    start_NPT(start),
    stop_NPT(stop)
{
}

ts::NPTEndpointDescriptor::NPTEndpointDescriptor(DuckContext& duck, const Descriptor& desc) :
    NPTEndpointDescriptor()
{
    deserialize(duck, desc);
}

void ts::NPTEndpointDescriptor::clearContent()
{
    start_NPT = 0;
    stop_NPT = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFFFF, 15);
    buf.putBits(start_NPT, 33);
    buf.putBits(0xFFFFFFFF, 31);
    buf.putBits(stop_NPT, 33);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(15);
    buf.getBits(start_NPT, 33);
    buf.skipBits(31);
    buf.getBits(stop_NPT, 33);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(14)) {
        buf.skipBits(15);
        disp << margin << UString::Format(u"Start NPT: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
        buf.skipBits(31);
        disp << margin << UString::Format(u"Stop NPT:  0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"start_NPT", start_NPT, true);
    root->setIntAttribute(u"stop_NPT", stop_NPT, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NPTEndpointDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(start_NPT, u"start_NPT", true, 0, 0, 0x00000001FFFFFFFF) &&
           element->getIntAttribute(stop_NPT, u"stop_NPT", true, 0, 0, 0x00000001FFFFFFFF);
}
