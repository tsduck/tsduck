//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDCCArrivingRequestDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dcc_arriving_request_descriptor"
#define MY_CLASS ts::DCCArrivingRequestDescriptor
#define MY_DID ts::DID_ATSC_DCC_ARRIVING
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCArrivingRequestDescriptor::DCCArrivingRequestDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::DCCArrivingRequestDescriptor::clearContent()
{
    dcc_arriving_request_type = 0;
    dcc_arriving_request_text.clear();
}

ts::DCCArrivingRequestDescriptor::DCCArrivingRequestDescriptor(DuckContext& duck, const Descriptor& desc) :
    DCCArrivingRequestDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCArrivingRequestDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(dcc_arriving_request_type);
    buf.putMultipleStringWithLength(dcc_arriving_request_text);
}

void ts::DCCArrivingRequestDescriptor::deserializePayload(PSIBuffer& buf)
{
    dcc_arriving_request_type = buf.getUInt8();
    buf.getMultipleStringWithLength(dcc_arriving_request_text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DCCArrivingRequestDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"DCC arriving request type: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp.displayATSCMultipleString(buf, 1, margin, u"DCC arriving request text: ");
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCCArrivingRequestDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"dcc_arriving_request_type", dcc_arriving_request_type, true);
    dcc_arriving_request_text.toXML(duck, root, u"dcc_arriving_request_text", true);
}

bool ts::DCCArrivingRequestDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(dcc_arriving_request_type, u"dcc_arriving_request_type", true) &&
           dcc_arriving_request_text.fromXML(duck, element, u"dcc_arriving_request_text", false);
}
