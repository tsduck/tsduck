//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDCCDepartingRequestDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dcc_departing_request_descriptor"
#define MY_CLASS    ts::DCCDepartingRequestDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_DCC_DEPARTING, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCDepartingRequestDescriptor::DCCDepartingRequestDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DCCDepartingRequestDescriptor::clearContent()
{
    dcc_departing_request_type = 0;
    dcc_departing_request_text.clear();
}

ts::DCCDepartingRequestDescriptor::DCCDepartingRequestDescriptor(DuckContext& duck, const Descriptor& desc) :
    DCCDepartingRequestDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(dcc_departing_request_type);
    buf.putMultipleStringWithLength(dcc_departing_request_text);
}

void ts::DCCDepartingRequestDescriptor::deserializePayload(PSIBuffer& buf)
{
    dcc_departing_request_type = buf.getUInt8();
    buf.getMultipleStringWithLength(dcc_departing_request_text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"DCC departing request type: %n", buf.getUInt8()) << std::endl;
        disp.displayATSCMultipleString(buf, 1, margin, u"DCC departing request text: ");
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCCDepartingRequestDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"dcc_departing_request_type", dcc_departing_request_type, true);
    dcc_departing_request_text.toXML(duck, root, u"dcc_departing_request_text", true);
}

bool ts::DCCDepartingRequestDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(dcc_departing_request_type, u"dcc_departing_request_type", true) &&
           dcc_departing_request_text.fromXML(duck, element, u"dcc_departing_request_text", false);
}
