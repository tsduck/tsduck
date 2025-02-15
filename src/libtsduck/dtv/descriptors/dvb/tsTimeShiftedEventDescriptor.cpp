//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeShiftedEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"time_shifted_event_descriptor"
#define MY_CLASS    ts::TimeShiftedEventDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_TIME_SHIFT_EVENT, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TimeShiftedEventDescriptor::TimeShiftedEventDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::TimeShiftedEventDescriptor::TimeShiftedEventDescriptor(DuckContext& duck, const Descriptor& desc) :
    TimeShiftedEventDescriptor()
{
    deserialize(duck, desc);
}

void ts::TimeShiftedEventDescriptor::clearContent()
{
    reference_service_id = 0;
    reference_event_id = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TimeShiftedEventDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(reference_service_id);
    buf.putUInt16(reference_event_id);
}

void ts::TimeShiftedEventDescriptor::deserializePayload(PSIBuffer& buf)
{
    reference_service_id = buf.getUInt16();
    reference_event_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TimeShiftedEventDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Reference service id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Reference event id: %n", buf.getUInt16()) << std::endl;
    }

}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TimeShiftedEventDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"reference_service_id", reference_service_id, true);
    root->setIntAttribute(u"reference_event_id", reference_event_id, true);
}

bool ts::TimeShiftedEventDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(reference_service_id, u"reference_service_id", true) &&
           element->getIntAttribute(reference_event_id, u"reference_event_id", true);
}
